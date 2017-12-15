/*********************************************************************************

 Copyright(c) 2012 Analog Devices, Inc. All Rights Reserved.

 This software is proprietary and confidential.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/
/*!
 * @file      AudioLoopback.c
 * @brief     Audio loopback example using AD1871 ADC and AD1854 DAC.
 * @version:  $Revision: 9599 $
 * @date:     $Date: 2012-06-26 02:46:24 -0400 (Tue, 26 Jun 2012) $
 *
 * @details
 *            This is the primary source file for audio loopback example using
 *            AD1871 Stereo Audio ADC and AD1854 Stereo Audio DAC drivers.
 *
 */

/*=============  I N C L U D E S   =============*/

/* Audio loopback example includes */
#include "AudioLoopback.h"
/* Managed drivers and/or services include */
#include <filter.h>
#include <fract2float_conv.h>

#include <blackfin.h>
#include <stdio.h>
#include <services\int\adi_int.h>
#include <drivers\uart\adi_uart.h>
#include <services\pwr\adi_pwr.h>
#include <string.h>

#include "HaipCommons.h"
#include "HaipModulator.h"
#include "HaipTxRx.h"

/* ADI initialization header */
#include "system/adi_initialize.h"

/*Functions*/
bool initialize_peripherals(void);
bool disconnect_peripherals(void);
bool is_error(uint32_t result);

/* Device Handles */
/* UART Handle */
static ADI_UART_HANDLE hDevice;
/* Handle to the AD1871 device instance */
static ADI_AD1871_HANDLE hAd1871Adc;
/* Handle to the AD1854 device instance */
static ADI_AD1854_HANDLE hAd1854Dac;

/*=============  D A T A  =============*/
#pragma align 2
/* Memory required to handle an AD1871 device instance */
static uint8_t Ad1871AdcMemory[ADI_AD1871_MEMORY_SIZE];
/* Memory required to handle an AD1854 device instance */
static uint8_t Ad1854DacMemory[ADI_AD1854_MEMORY_SIZE];

/*=============  L O C A L    F U N C T I O N S  =============*/

/* Opens and configures AD1871 ADC driver */
static uint32_t InitAd1871Adc(void);
/* Opens and configures AD1854 ADC driver */
static uint32_t InitAd1854Dac(void);

/* Audio data buffers */
#pragma align 4
section ("sdram0") uint8_t RxAudioBuf1[HAIP_AUDIO_BUFFER_SIZE];
section ("sdram0") uint8_t RxAudioBuf2[HAIP_AUDIO_BUFFER_SIZE];
section ("sdram0") uint8_t TxAudioBuf1[HAIP_AUDIO_BUFFER_SIZE];
section ("sdram0") uint8_t TxAudioBuf2[HAIP_AUDIO_BUFFER_SIZE];

/* Rx and Tx buffers */
char BufferTx1[BUFFER_SIZE];
char BufferTx2[BUFFER_SIZE];
char BufferRx1[BUFFER_SIZE];
char BufferRx2[BUFFER_SIZE];

/*=============  C O D E  =============*/

/* Memory required for operating UART in dma mode */
static uint8_t gUARTMemory[ADI_UART_BIDIR_DMA_MEMORY_SIZE];

/*********************************************************************
 *
 *   Function:   main
 *
 *********************************************************************/
void main(void) {

	bool error = initialize_peripherals();
	/* IF (Success) */
	if (!error) {
		haiptxrx_init_devices(hDevice, hAd1854Dac, hAd1871Adc);
		while (!haiptxrx_iterate());
	}
	disconnect_peripherals();

}

/*Initializes the system, UART communication and ADC/DAC devices*/
bool initialize_peripherals() {

	uint32_t result;

	/* Initialize managed drivers and/or services */
	result = adi_initComponents();
	if (is_error(result))
		return true;

	/* Clear audio input and output buffers */
	memset(RxAudioBuf1, 0, HAIP_AUDIO_BUFFER_SIZE);
	memset(RxAudioBuf2, 0, HAIP_AUDIO_BUFFER_SIZE);
	memset(TxAudioBuf1, 0, HAIP_AUDIO_BUFFER_SIZE);
	memset(TxAudioBuf2, 0, HAIP_AUDIO_BUFFER_SIZE);

	/* Initialize power service */
	result = (uint32_t) adi_pwr_Init(PROC_CLOCK_IN, PROC_MAX_CORE_CLOCK,
	PROC_MAX_SYS_CLOCK, PROC_MIN_VCO_CLOCK);

	/* IF (Failure) */
	if (is_error(result)) {
		DEBUG_MSG1("Failed to initialize Power service\n");
		return true;
	}

	/* IF (Success) */
	if (!is_error(result)) {
		/* Set the required core clock and system clock */
		result = (uint32_t) adi_pwr_SetFreq(PROC_REQ_CORE_CLOCK,
		PROC_REQ_SYS_CLOCK);

		/* IF (Failure) */
		if (is_error(result)) {
			DEBUG_MSG1("Failed to initialize Power service\n");
			return true;
		}
	}

	//UART initialization
	/* Open UART driver */
	result = adi_uart_Open(UART_DEV_NUM, ADI_UART_DIR_BIDIRECTION, gUARTMemory,
	ADI_UART_BIDIR_DMA_MEMORY_SIZE, &hDevice);
	if (is_error(result)) {
		DEBUG_MSG1("Failed to initialize UART\n");
		return true;
	}

	/* Set UART Baud Rate */
	result = adi_uart_SetBaudRate(hDevice, HAIP_BAUD_RATE);
	if (is_error(result)) {
		DEBUG_MSG1("Failed to initialize UART\n");
		return true;
	}

	/* Configure  UART device with NO-PARITY, ONE STOP BIT and 8bit word length. */
	result = adi_uart_SetConfiguration(hDevice, ADI_UART_NO_PARITY,
			ADI_UART_ONE_STOPBIT, ADI_UART_WORDLEN_8BITS);
	if (is_error(result)) {
		DEBUG_MSG1("Failed to initialize UART\n");
		return true;
	}

	/* Enable the DMA associated with UART if UART is expeced to work with DMA mode */
	result = adi_uart_EnableDMAMode(hDevice, true);
	if (is_error(result)) {
		DEBUG_MSG1("Failed to initialize UART\n");
		return true;
	}

	adi_uart_SubmitTxBuffer(hDevice, BufferTx1, BUFFER_SIZE);
	adi_uart_SubmitRxBuffer(hDevice, BufferTx2, BUFFER_SIZE);
	adi_uart_SubmitTxBuffer(hDevice, BufferRx1, BUFFER_SIZE);
	adi_uart_SubmitRxBuffer(hDevice, BufferRx2, BUFFER_SIZE);

	result = adi_uart_EnableTx(hDevice, true);
	result = adi_uart_EnableRx(hDevice, true);

	/* IF (Success) */
	if (!is_error(result)) {
		/* Open and configure AD1871 ADC device instance */
		result = InitAd1871Adc();
	} else {
		printf("Error initializing ADC\n");
		return true;
	}

	/* IF (Success) */
	if (!is_error(result)) {
		/* Open and configure AD1854 DAC device instance */
		result = InitAd1854Dac();
	} else {
		printf("Error initializing DAC\n");
		return true;
	}

	// IF (Success)
	if (!is_error(result)) {
		// Enable AD1854 DAC dataflow
		result = adi_ad1854_Enable(hAd1854Dac, true);

		// IF (Failure)
		if (is_error(result)) {
			DEBUG_MSG2("Failed to enable AD1854 DAC dataflow", result);
		}
	} else {
		printf("Error initializing DAC\n");
		return true;
	}

	// IF (Success)
	if (!is_error(result)) {
		// Enable AD1871 ADC dataflow
		result = adi_ad1871_Enable(hAd1871Adc, true);

		// IF (Failure)
		if (is_error(result)) {
			DEBUG_MSG2("Failed to enable AD1871 ADC dataflow", result);
			return true;
		}
	} else {
		printf("error enabling DAC\n");
		return true;
	}

	if (!is_error(result)) {
		printf("Error enabling ADC");
		return true;
	}

	return false;

}

/*Disconnects the DAC/ADC/UART */
bool disconnect_peripherals(void) {

	uint32_t result;
	bool has_had_error = false;

	/* Disable AD1854 DAC dataflow */
	result = adi_ad1854_Enable(hAd1854Dac, false);

	/* IF (Failure) */
	if (result) {
		DEBUG_MSG2("Failed to disable AD1854 DAC dataflow", result);
		has_had_error = true;
	}

	/* IF (Success) */
	if (result == 0) {
		/* Disable AD1871 ADC dataflow */
		result = adi_ad1871_Enable(hAd1871Adc, false);

		/* IF (Failure) */
		if (result) {
			DEBUG_MSG2("Failed to disable AD1871 ADC dataflow", result);
			has_had_error = true;
		}
	}

	/* IF (Success) */
	if (!is_error(result)) {
		/* Close AD1854 DAC instance */
		result = adi_ad1854_Close(hAd1854Dac);

		/* IF (Failure) */
		if (is_error(result)) {
			DEBUG_MSG2("Failed to close AD1854 DAC instance", result);
			has_had_error = true;
		}
	}

	/* IF (Success) */
	if (!is_error(result)) {
		/* Close AD1871 ADC instance */
		result = adi_ad1871_Close(hAd1871Adc);

		/* IF (Failure) */
		if (is_error(result)) {
			DEBUG_MSG2("Failed to close AD1871 ADC instance", result);
			has_had_error = true;
		}
	}

	return has_had_error;
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
static uint32_t InitAd1854Dac(void) {
	/* Return code */
	uint32_t result;

	/* Open AD1854 instance */
	result = adi_ad1854_Open(AD1854_DEV_NUM, &Ad1854DacMemory,
	ADI_AD1854_MEMORY_SIZE, &hAd1854Dac);

	/* IF (Failed) */
	if (result != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to open AD1854 DAC instance", result);
		return ((uint32_t) result);
	}

	/* Reset AD1854 */
	result = adi_ad1854_HwReset(hAd1854Dac, AD1854_RESET_PORT,
	AD1854_RESET_PIN);

	/* IF (Failed) */
	if (result != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to reset AD1854 DAC instance", result);
		return ((uint32_t) result);
	}

	/* Set SPORT device number, External clock source (SPORT as Slave) */
	result = adi_ad1854_SetSportDevice(hAd1854Dac, AD1854_SPORT_DEV_NUM, true);

	/* IF (Failed) */
	if (result != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to set AD1854 DAC SPORT device instance", result);
		return ((uint32_t) result);
	}

	/* Submit Audio buffer 1 to AD1854 DAC */
	result = adi_ad1854_SubmitTxBuffer(hAd1854Dac, &TxAudioBuf1,
	HAIP_AUDIO_BUFFER_SIZE);

	/* IF (Failed) */
	if (result != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1854 DAC", result);
		return ((uint32_t) result);
	}

	/* Submit Audio buffer 2 to AD1854 DAC */
	result = adi_ad1854_SubmitTxBuffer(hAd1854Dac, &TxAudioBuf2,
	HAIP_AUDIO_BUFFER_SIZE);

	/* IF (Failed) */
	if (result != ADI_AD1854_SUCCESS) {
		DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1854 DAC", result);
	}

	/* Return */
	return ((uint32_t) result);
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
static uint32_t InitAd1871Adc(void) {
	/* Return code */
	uint32_t result;

	/* Open AD1871 instance */
	result = adi_ad1871_Open(AD1871_DEV_NUM, &Ad1871AdcMemory,
	ADI_AD1871_MEMORY_SIZE, &hAd1871Adc);

	/* IF (Failed) */
	if (is_error(result)) {
		DEBUG_MSG2("Failed to open AD1871 ADC instance", result);
		return ((uint32_t) result);
	}

	/* Reset AD1871 */
	result = adi_ad1871_HwReset(hAd1871Adc, AD1871_RESET_PORT,
	AD1871_RESET_PIN);

	/* IF (Failed) */
	if (is_error(result)) {
		DEBUG_MSG2("Failed to reset AD1871 ADC instance", result);
		return ((uint32_t) result);
	}

	/* Set SPORT device number, AD1871 as Master (SPORT as Slave) */
	result = adi_ad1871_SetSportDevice(hAd1871Adc, AD1871_SPORT_DEV_NUM, true);

	/* IF (Failed) */
	if (is_error(result)) {
		DEBUG_MSG2("Failed to set AD1871 ADC SPORT device instance", result);
		return ((uint32_t) result);
	}

	/* Submit Audio buffer 1 to AD1871 ADC */
	result = adi_ad1871_SubmitRxBuffer(hAd1871Adc, &RxAudioBuf1,
	HAIP_AUDIO_BUFFER_SIZE);

	/* IF (Failed) */
	if (is_error(result)) {
		DEBUG_MSG2("Failed to submit Audio buffer 1 to AD1871 ADC", result);
		return ((uint32_t) result);
	}

	/* Submit Audio buffer 2 to AD1871 ADC */
	result = adi_ad1871_SubmitRxBuffer(hAd1871Adc, &RxAudioBuf2,
	HAIP_AUDIO_BUFFER_SIZE);

	/* IF (Failed) */
	if (is_error(result)) {
		DEBUG_MSG2("Failed to submit Audio buffer 2 to AD1871 ADC", result);
	}

	/* Return */
	return ((uint32_t) result);
}

static bool is_error(uint32_t result) {
	return (bool) result;
}

/*****/

/*
 ** EOF
 */
