/*!
 * =========== H A I P -  M O D E M =============
 */

/*=============  I N C L U D E S   =============*/

#include "haip_modem.h"
/* Managed drivers and/or services include */
#include "system/adi_initialize.h"
#include <filter.h>
#include <fract2float_conv.h>

#include <blackfin.h>
#include <stdio.h>
#include <services\int\adi_int.h>
#include <drivers\uart\adi_uart.h>
#include <services\pwr\adi_pwr.h>
#include <string.h>

#include "haip_commons.h"
#include "haip_modulator.h"
#include "haip_demodulator.h"

#include "haip_tx_rx.h"
#include "haip_16QAM_mapping.h"

/* ADI initialization header */
#include "system/adi_initialize.h"

/*=============  L O C A L    F U N C T I O N S  =============*/

/* Opens and configures AD1871 ADC driver */
static uint32_t init_1871_adc(void);
/* Opens and configures AD1854 ADC driver */
static uint32_t init_1854_dac(void);
/*Initializes the system, UART communication and ADC/DAC devices*/
bool initialize_peripherals(void);
/* Sends a test message through UART */
void test_uart(unsigned char* buffer);
/* Closes peripherals */
void finalize_peripherals(void);

/*==========  D E V I C E S  ==========*/
/* UART Handle */
static ADI_UART_HANDLE h_uart_device;
/* Handle to the AD1871 device instance */
static ADI_AD1871_HANDLE h_adi_1871_adc_device;
/* Handle to the AD1854 device instance */
static ADI_AD1854_HANDLE h_adi_1854_dac_device;

/*=============  D A T A  =============*/

section ("sdram0") unsigned char entrada_test[BUFFER_SIZE];

/* Audio data buffers */
#pragma align 4
section ("sdram0") uint8_t analog_rx_buffer_1[HAIP_ANALOG_BUFFER_SIZE];
section ("sdram0") uint8_t analog_rx_buffer_2[HAIP_ANALOG_BUFFER_SIZE];
section ("sdram0") uint8_t analog_tx_buffer_1[HAIP_ANALOG_BUFFER_SIZE];
section ("sdram0") uint8_t analog_tx_buffer_2[HAIP_ANALOG_BUFFER_SIZE];

/* Rx and Tx buffers */
char buffer_tx_1[BUFFER_SIZE];
char buffer_tx_2[BUFFER_SIZE];
char buffer_rx_1[BUFFER_SIZE];
char buffer_rx_2[BUFFER_SIZE];

/* Memory required to handle an AD1871 device instance */
static uint8_t reserved_1871_adc_memory[ADI_AD1871_MEMORY_SIZE];
/* Memory required to handle an AD1854 device instance */
static uint8_t reserved_1854_dac_memory[ADI_AD1854_MEMORY_SIZE];
/* Memory required for operating UART in dma mode */
static uint8_t reserved_uart_memory[ADI_UART_BIDIR_DMA_MEMORY_SIZE];

/*=============  C O D E  =============*/

//#########################
// ======= M A I N ========
//#########################
void main(void) {
	fract32 modulated_out[HAIP_TX_PACKET_LENGTH];
	unsigned char modulated_in[5];
	unsigned char demodulated_out[HAIP_TX_PACKET_LENGTH];
	haip_sync_t sync;
	int length = 0;

	/* Flag which indicates whether to stop the program */
	bool stop_flag = false;
	haip_init_const();
	memcpy(modulated_in,"MIKE2",5);
	memcpy(modulated_out,modulate_frame(modulated_in,5),HAIP_TX_PACKET_LENGTH);
	sync = haip_demodulate_head(modulated_out, demodulated_out);
	length = demodulated_out[0] & 0xE0 >> 5;
	haip_demodulate_payload(modulated_out,length,sync,demodulated_out);
	printf("ss");
	/*bool result = initialize_peripherals();

	memcpy(entrada_test, "MIKE", 5);
	test_uart(entrada_test);

	/* IF (Success) */
	/*if (result == 0) {
		haiptxrx_init_devices(h_uart_device, h_adi_1854_dac_device,
				h_adi_1871_adc_device);
		while (!stop_flag) {
			haiptxrx_iterate();
		}
	}

	finalize_peripherals();*/

}
//#########################
// ======= M A I N ========
//#########################

/* Sends a test message through UART */
void test_uart(unsigned char* buffer) {
	char* UBufferBidali;
	bool enviar = 0;

	while (!enviar) {
		adi_uart_IsTxBufferAvailable(h_uart_device, &enviar);
	}
	adi_uart_GetTxBuffer(h_uart_device, (void**) &UBufferBidali);
	memcpy(UBufferBidali, buffer, BUFFER_SIZE);
	adi_uart_SubmitTxBuffer(h_uart_device, UBufferBidali, BUFFER_SIZE);
}

/* Closes peripherals */
void finalize_peripherals() {
	bool result;
	/* IF (Success) */
	/* Disable AD1854 DAC dataflow */
	result = adi_ad1854_Enable(h_adi_1854_dac_device, false);

	/* IF (Failure) */
	if (result) {
		DEBUG_MSG2("Failed to disable AD1854 DAC dataflow", result);
	}

	/* IF (Success) */
	if (result == 0) {
		/* Disable AD1871 ADC dataflow */
		result = adi_ad1871_Enable(h_adi_1871_adc_device, false);

		/* IF (Failure) */
		if (result) {
			DEBUG_MSG2("Failed to disable AD1871 ADC dataflow", result);
		}
	}

	/* IF (Success) */
	if (result == 0) {
		/* Close AD1854 DAC instance */
		result = adi_ad1854_Close(h_adi_1854_dac_device);

		/* IF (Failure) */
		if (result) {
			DEBUG_MSG2("Failed to close AD1854 DAC instance", result);
		}
	}

	/* IF (Success) */
	if (result == 0) {
		/* Close AD1871 ADC instance */
		result = adi_ad1871_Close(h_adi_1871_adc_device);

		/* IF (Failure) */
		if (result) {
			DEBUG_MSG2("Failed to close AD1871 ADC instance", result);
		}
	}

	/* IF (Success) */
	if (result == 0) {
		printf("All Done\n");
	}
	/* ELSE (Failure) */
	else {
		printf("Failed\n");
	}

}

/*Initializes the system, UART communication and ADC/DAC devices*/
bool initialize_peripherals() {
	bool result;
	unsigned int i;
	ADI_UART_RESULT eResult;

	/* Initialize managed drivers and/or services */
	adi_initComponents();

	/* Clear audio input and output buffers */
	memset(analog_rx_buffer_1, 0, HAIP_ANALOG_BUFFER_SIZE);
	memset(analog_rx_buffer_2, 0, HAIP_ANALOG_BUFFER_SIZE);
	memset(analog_tx_buffer_1, 0, HAIP_ANALOG_BUFFER_SIZE);
	memset(analog_tx_buffer_2, 0, HAIP_ANALOG_BUFFER_SIZE);

	/* Initialize power service */
	result = (uint32_t) adi_pwr_Init(PROC_CLOCK_IN, PROC_MAX_CORE_CLOCK,
	PROC_MAX_SYS_CLOCK, PROC_MIN_VCO_CLOCK);

	/* IF (Failure) */
	if (result) {
		DEBUG_MSG1("Failed to initialize Power service\n");
	}

	/* IF (Success) */
	if (result == 0) {
		/* Set the required core clock and system clock */
		result = (uint32_t) adi_pwr_SetFreq(PROC_REQ_CORE_CLOCK,
		PROC_REQ_SYS_CLOCK);

		/* IF (Failure) */
		if (result) {
			DEBUG_MSG1("Failed to initialize Power service\n");
		}
	}

	//UART initialization
	/* Open UART driver */
	eResult = adi_uart_Open(HAIP_UART_DEV_NUM, ADI_UART_DIR_BIDIRECTION,
			reserved_uart_memory, ADI_UART_BIDIR_DMA_MEMORY_SIZE,
			&h_uart_device);
	if (eResult != ADI_UART_SUCCESS) {
		printf("failure\n");
	}

	/* Set UART Baud Rate */
	eResult = adi_uart_SetBaudRate(h_uart_device, HAIP_BAUD_RATE);
	if (eResult != ADI_UART_SUCCESS) {
		printf("failure\n");
	}

	/* Configure  UART device with NO-PARITY, ONE STOP BIT and 8bit word length. */
	eResult = adi_uart_SetConfiguration(h_uart_device, ADI_UART_NO_PARITY,
			ADI_UART_ONE_STOPBIT, ADI_UART_WORDLEN_8BITS);
	if (eResult != ADI_UART_SUCCESS) {
		printf("failure\n");
	}

	/* Enable the DMA associated with UART if UART is expeced to work with DMA mode */
	eResult = adi_uart_EnableDMAMode(h_uart_device, true);
	if (eResult != ADI_UART_SUCCESS) {
		printf("failure\n");
	}

	//printf("Setup terminal on PC as described in Readme file. \n\n");

	adi_uart_SubmitTxBuffer(h_uart_device, buffer_tx_1, BUFFER_SIZE);
	adi_uart_SubmitRxBuffer(h_uart_device, buffer_tx_2, BUFFER_SIZE);
	adi_uart_SubmitTxBuffer(h_uart_device, buffer_rx_1, BUFFER_SIZE);
	adi_uart_SubmitRxBuffer(h_uart_device, buffer_rx_2, BUFFER_SIZE);

	eResult = adi_uart_EnableTx(h_uart_device, true);
	eResult = adi_uart_EnableRx(h_uart_device, true);

	/* IF (Success) */
	if (result == 0) {
		/* Open and configure AD1871 ADC device instance */
		result = init_1871_adc();
	} else {
		printf("Error initing uart");
	}

	/* IF (Success) */
	if (result == 0) {
		/* Open and configure AD1854 DAC device instance */
		result = init_1854_dac();
	} else {
		printf("Error initing ADC");
	}

	// IF (Success)
	if (result == 0) {
		// Enable AD1854 DAC dataflow
		result = adi_ad1854_Enable(h_adi_1854_dac_device, true);

		// IF (Failure)
		if (result) {
			DEBUG_MSG2("Failed to enable AD1854 DAC dataflow", result);
		}
	} else {
		printf(" error initing DAC");
	}

	// IF (Success)
	if (result == 0) {
		// Enable AD1871 ADC dataflow
		result = adi_ad1871_Enable(h_adi_1871_adc_device, true);

		// IF (Failure)
		if (result) {
			DEBUG_MSG2("Failed to enable AD1871 ADC dataflow", result);
		}
	} else {
		printf("error enabling DAC");
	}

	if (result != 0)
		printf("Error enabling ADC");
	return result;

}

/*
 * Opens and initializes AD1854 DAC device instance.
 *
 * Parameters
 *  None
 *
 * Returns
 *  0 if success, other values for error
 *
 */
static uint32_t init_1854_dac(void) {
	/* Return code */
	ADI_AD1854_RESULT eResult;

	/* Open AD1854 instance */
	eResult = adi_ad1854_Open(AD1854_DEV_NUM, &reserved_1854_dac_memory,
	ADI_AD1854_MEMORY_SIZE, &h_adi_1854_dac_device);

	/* IF (Failed) */
	if (eResult != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to open AD1854 DAC instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Reset AD1854 */
	eResult = adi_ad1854_HwReset(h_adi_1854_dac_device, AD1854_RESET_PORT,
	AD1854_RESET_PIN);

	/* IF (Failed) */
	if (eResult != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to reset AD1854 DAC instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Set SPORT device number, External clock source (SPORT as Slave) */
	eResult = adi_ad1854_SetSportDevice(h_adi_1854_dac_device,
			AD1854_SPORT_DEV_NUM, true);

	/* IF (Failed) */
	if (eResult != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to set AD1854 DAC SPORT device instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Submit Audio buffer 1 to AD1854 DAC */
	eResult = adi_ad1854_SubmitTxBuffer(h_adi_1854_dac_device,
			&analog_tx_buffer_1,
			HAIP_ANALOG_BUFFER_SIZE);

	/* IF (Failed) */
	if (eResult != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1854 DAC", eResult);
		return ((uint32_t) eResult);
	}

	/* Submit Audio buffer 2 to AD1854 DAC */
	eResult = adi_ad1854_SubmitTxBuffer(h_adi_1854_dac_device,
			&analog_tx_buffer_2,
			HAIP_ANALOG_BUFFER_SIZE);

	/* IF (Failed) */
	if (eResult != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1854 DAC", eResult);
	}

	/* Return */
	return ((uint32_t) eResult);
}

/*
 * Opens and initializes AD1871 ADC device instance.
 *
 * Parameters
 *  None
 *
 * Returns
 *  0 if success, other values for error
 *
 */
static uint32_t init_1871_adc(void) {
	/* Return code */
	ADI_AD1871_RESULT eResult;

	/* Open AD1871 instance */
	eResult = adi_ad1871_Open(AD1871_DEV_NUM, &reserved_1871_adc_memory,
	ADI_AD1871_MEMORY_SIZE, &h_adi_1871_adc_device);

	/* IF (Failed) */
	if (eResult != ADI_AD1871_SUCCESS) {
		DEBUG_MSG2("Failed to open AD1871 ADC instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Reset AD1871 */
	eResult = adi_ad1871_HwReset(h_adi_1871_adc_device, AD1871_RESET_PORT,
	AD1871_RESET_PIN);

	/* IF (Failed) */
	if (eResult != ADI_AD1871_SUCCESS) {
		DEBUG_MSG2("Failed to reset AD1871 ADC instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Set SPORT device number, AD1871 as Master (SPORT as Slave) */
	eResult = adi_ad1871_SetSportDevice(h_adi_1871_adc_device,
			AD1871_SPORT_DEV_NUM, true);

	/* IF (Failed) */
	if (eResult != ADI_AD1871_SUCCESS) {
		DEBUG_MSG2("Failed to set AD1871 ADC SPORT device instance", eResult);
		return ((uint32_t) eResult);
	}

	/* Submit Audio buffer 1 to AD1871 ADC */
	eResult = adi_ad1871_SubmitRxBuffer(h_adi_1871_adc_device,
			&analog_rx_buffer_1,
			HAIP_ANALOG_BUFFER_SIZE);

	/* IF (Failed) */
	if (eResult != ADI_AD1871_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1871 ADC", eResult);
		return ((uint32_t) eResult);
	}

	/* Submit Audio buffer 2 to AD1871 ADC */
	eResult = adi_ad1871_SubmitRxBuffer(h_adi_1871_adc_device,
			&analog_rx_buffer_2,
			HAIP_ANALOG_BUFFER_SIZE);

	/* IF (Failed) */
	if (eResult != ADI_AD1871_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1871 ADC", eResult);
	}

	/* Return */
	return ((uint32_t) eResult);
}

/*
 ** EOF
 */
