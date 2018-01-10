/*
 * HaipTxRx.c
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */
#include "haip_tx_rx.h"
#include "haip_commons.h"
#include "haip_modulator.h"
#include "fract2float_conv.h"

//Local variables
section ("sdram0") unsigned char digital_input_buffer[HAIP_UART_BUFFER_SIZE];
double last_digital_input_t = 0;
section ("sdram0") unsigned char frame_buffer[HAIP_FRAME_BUFFER_SIZE][HAIP_FRAME_LENGTH_MAX];
//section ("sdram0") fract32 output_buffer[HAIP_DAC_BUFFER_SIZE];
void *uart_rx_buffer, *adc_buffer, *dac_buffer, *uart_tx_buffer;

int frame_count = 0;
int contadoreMierdosoa = 0;

static ADI_UART_HANDLE uart_dev;
static ADI_AD1871_HANDLE dac_dev;
static ADI_AD1871_HANDLE adc_dev;

section ("sdram0") unsigned char uart_test[HAIP_UART_BUFFER_SIZE];

section ("sdram0") fract32 tmp_buffer[40 * 8 + 49];
section ("sdram0") unsigned char adc_entered[HAIP_ANALOG_BUFFER_SIZE];

int contar = 0;
bool uart_has_buffer_ready;
bool adcAvailable = true;
bool probetako = false;

section ("sdram0") unsigned char ptr_32[HAIP_ANALOG_BUFFER_SIZE];

//Declarations
bool has_tx_frame_ready(void);
bool has_rx_frame_ready(void);
void output_analog(void);
void output_digital(void);
void read_analog_input(void);
void read_digital_input(void);
void check_adc_available(void);
void process_digital_input(unsigned char* buffer, int size);
int get_next_frame_legth(char* buffer, int* flen);
bool dac_is_free(void);
int get_frames(char* buffer, char** frames, int curr_len);
void send_dac(bool do_send);

//GLOBAL FUNCTIONS

void haiptxrx_init_devices(ADI_UART_HANDLE uart, ADI_AD1871_HANDLE dac,
		ADI_AD1871_HANDLE adc) {
	uart_dev = uart;
	dac_dev = dac;
	adc_dev = adc;
}

bool haiptxrx_iterate() {

	bool uart_tx_free = true;
	read_digital_input();
	check_adc_available();
	output_analog();
	read_analog_input();

	return false;
}

void check_adc_available() {
	ADI_AD1871_RESULT Result;
	Result = adi_ad1871_IsRxBufAvailable(adc_dev, &adcAvailable);

	/* IF (Failure) */

	/* IF (AD1871 Buffer available) */
	if (adcAvailable) {
		/* Get AD1871 processed buffer address */
		Result = adi_ad1871_GetRxBuffer(adc_dev, &adc_buffer);

		/* IF (Failure) */

	}

}

//LOCAL FUNCTIONS

void read_digital_input(void) {
	ADI_UART_RESULT res;
	res = adi_uart_IsRxBufferAvailable(uart_dev, &uart_has_buffer_ready);
	if (uart_has_buffer_ready) {
		adi_uart_GetRxBuffer(uart_dev, &uart_rx_buffer);
		memcpy(digital_input_buffer, uart_rx_buffer, HAIP_UART_BUFFER_SIZE);
		adi_uart_SubmitRxBuffer(uart_dev, uart_rx_buffer,
		HAIP_UART_BUFFER_SIZE);
		process_digital_input(digital_input_buffer, HAIP_UART_BUFFER_SIZE);
	}
}

void process_digital_input(unsigned char* buffer, int size) {

	/*if (check_timeout(last_digital_input_t, DIGITAL_INPUT_TIMEOUT,
	 &last_digital_input_t)) {
	 //strcpy(digital_input_buffer, buffer);
	 //printf("strcpy(digital_input_buffer, buffer); \n\n\n");

	 } else {
	 if (strlen(digital_input_buffer)
	 + strlen(buffer) >= DIGITAL_INPUT_BUFFER_SIZE) {
	 //printf("ERROR: Overflow detected on uart input buffer, emptying buffer");
	 digital_input_buffer[0] = '\0';
	 }
	 //strcat(digital_input_buffer, buffer);
	 //printf("strcat(digital_input_buffer, buffer); \n\n\n");

	 }

	 //frame_count += get_frames(digital_input_buffer, frame_buffer, frame_count);
	 */
	frame_count++;

}

int get_frames(char* buffer, char** frames, int curr_len) {
	int frame_len;
	int frame_cnt = curr_len;
	while (get_next_frame_legth(buffer, &frame_len)
			|| frame_count < HAIP_FRAME_BUFFER_SIZE) {
		memcpy(frames[frame_cnt], buffer, frame_len);
		frames[frame_cnt][frame_len] = '\0';
		strcpy(buffer, &buffer[frame_len]);
		frame_cnt++;
	}
	return frame_cnt - curr_len;
}

int get_next_frame_legth(char* buffer, int* flen) {
	int len = strlen(buffer);
	if (len >= HAIP_FRAME_LENGTH_MIN) {
		int data_len = buffer[HAIP_FRAME_DATA_LEN_INDEX];
		int frame_len = data_len + HAIP_FRAME_LENGTH_MIN;
		if (len > frame_len) {
			*flen = frame_len;
			return frame_len;
		}
	}
	*flen = 0;
	return 0;
}

bool has_tx_frame_ready() {
	return frame_count > 0;
}

void output_analog() {

	if (dac_is_free()) {
		if (frame_count >= 1) {
			//modulate_frame(frame_buffer[0], output_buffer);
			send_dac(true);
			frame_count = 0;
		} else {
			send_dac(false);
		}
	}
}

bool dac_is_free(void) {
	bool available;
	adi_ad1854_IsTxBufAvailable(dac_dev, &available);
	return available;
}

void send_dac(bool do_send) {
	ADI_AD1854_RESULT result;
	int i = 0;
	int j = 0;
	//unsigned char prueba [HAIP_ANALOG_BUFFER_SIZE];
	result = adi_ad1854_GetTxBuffer(dac_dev, &dac_buffer);
	//fract32* fr32_buffer = (fract32*) dac_buffer;
	//ptr_32 = (unsigned char *) dac_buffer;
	if (do_send) {
		fract32 kk = modulate_frame(digital_input_buffer, 5, tmp_buffer);
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE/4; i++) {
			if (i >= ((5 * 2 * 2 + 8) * 8 + 49)
					&& i % ((5 * 2 * 2 + 8) * 8 + 49) == 0) {
				j = 0;
			}
			//ptr_32[i] = (tmp_buffer[j]>>8);
			j++;
		}
		probetako = true;
		//memcpy(dac_buffer, digital_input_buffer, AUDIO_BUFFER_SIZE);
	} else {

		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE; i=i+4) {
			ptr_32[i] = 0x6E;
			ptr_32[i+1] = 0x34;
			ptr_32[i+2] = 0x80;
			ptr_32[i+3] = 0xFF;
			//ptr_32[i] = float_to_fr32(0.0039); //0x92cb7f00
			//ptr_32[i+1] = float_to_fr32(-0.0039); //0x6e3480ff
		}
		//memcpy(empty_buffer, dac_buffer, HAIP_ANALOG_BUFFER_SIZE);

	}
	memcpy(dac_buffer, ptr_32, HAIP_ANALOG_BUFFER_SIZE);
	adi_ad1854_SubmitTxBuffer(dac_dev, dac_buffer, HAIP_ANALOG_BUFFER_SIZE);
}

bool has_rx_frame_ready(void) {
	return 0;
}
void output_digital(void) {
	bool uart_tx_free = 0;
	float prueba[HAIP_UART_BUFFER_SIZE];
	float threshold;
	adi_uart_IsTxBufferAvailable(uart_dev, &uart_tx_free);
	if (uart_tx_free) {
		//ptr_32 = (fract32*) adc_entered;  IPINI BARRIRO!!!!!
		adi_uart_GetTxBuffer(uart_dev, (void**) &uart_tx_buffer);
		for (int i = 0; i < HAIP_UART_BUFFER_SIZE; ++i) {
			prueba[i] = fr32_to_float(adc_entered[i]<<8);
			threshold = prueba[i] * 10;
			if (threshold > 1.0 || threshold < -1.0) {
				threshold = 0;
			} else if (probetako) {
				threshold = 0;
			} else {
				threshold = 0;

			}
		}
		if (probetako) {
			threshold = 0;
		}
		memcpy(uart_tx_buffer, prueba, HAIP_UART_BUFFER_SIZE);
		adi_uart_SubmitTxBuffer(uart_dev, uart_tx_buffer,
		HAIP_UART_BUFFER_SIZE);
	}
}

void read_analog_input(void) {
	if (adc_buffer != NULL) {
		memcpy(adc_entered,adc_buffer,HAIP_ANALOG_BUFFER_SIZE);
		contadoreMierdosoa++;
		if(contadoreMierdosoa > 5){
			contadoreMierdosoa = 0;
		}
		adi_ad1871_SubmitRxBuffer(adc_dev, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);
		//output_digital();
		adc_buffer = NULL;
	}
}

