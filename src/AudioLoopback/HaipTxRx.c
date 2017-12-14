/*
 * HaipTxRx.c
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */
#include "HaipTxRx.h"
#include "HaipCommons.h"
#include "declaraciones.h"
#include "HaipModulator.h"

//definitions

#define DIGITAL_INPUT_TIMEOUT 0.02
#define DIGITAL_INPUT_BUFFER_SIZE 500
#define FRAME_BUFFER_SIZE 10
#define AUDIO_BUFFER_SIZE 65536*2
#define DAC_BUFFER_SIZE 900

//Local variables
section ("sdram0") unsigned char digital_input_buffer[UART_BUFFER_SIZE];
double last_digital_input_t = 0;
section ("sdram0") unsigned char frame_buffer[FRAME_BUFFER_SIZE][HAIP_FRAME_LENGTH_MAX];
section ("sdram0") fract32 output_buffer[DAC_BUFFER_SIZE];
unsigned char *uart_rx_buffer, *adc_buffer, *dac_buffer;
int frame_count = 0;
static ADI_UART_HANDLE uart_dev;
static ADI_AD1871_HANDLE dac_dev;
static ADI_AD1871_HANDLE adc_dev;

section ("sdram0") unsigned char uart_test[UART_BUFFER_SIZE];

int contar = 0;
bool uart_is_free;
bool available;
bool adcAvailable = false;

//Declarations
bool has_tx_frame_ready(void);
bool has_rx_frame_ready(void);
void output_analog(void);
void output_digital(void);
void read_analog_input(void);
void read_digital_input(void);
void check_adc_available();
void process_digital_input(unsigned char* buffer, int size);
int get_next_frame_legth(char* buffer, int* flen);
void dac_is_free(void);
int get_frames(char* buffer, char** frames, int curr_len);
void send_dac(bool do_send);

//GLOBAL FUNCTIONS

void haiptxrx_init_devices(ADI_UART_HANDLE uart, ADI_AD1871_HANDLE dac,
		ADI_AD1871_HANDLE adc) {
	uart_dev = uart;
	dac_dev = dac;
	adc_dev = adc;
	//printf("Devices initialized\n\n\n");
}

void haiptxrx_iterate() {

	read_digital_input();
	check_adc_available();
	output_analog(); //output_analog(); //printf("Something readed \n\n\n");
	read_analog_input();
	//read_digital_input(); //Get bytes from serial (care timeouts)
	contar++;

	//dac_is_free();
	//Transmit it
	//printf("output_analog(); \n\n\n");

	/*
	 read_analog_input();
	 if (has_rx_frame_ready()) { //If full frame received
	 output_digital(); //Send through serial
	 }*/

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
	res = adi_uart_IsRxBufferAvailable(uart_dev, &uart_is_free);
	if (uart_is_free) {
		adi_uart_GetRxBuffer(uart_dev, &uart_rx_buffer);
		memcpy(digital_input_buffer, uart_rx_buffer, UART_BUFFER_SIZE);
		adi_uart_SubmitRxBuffer(uart_dev, uart_rx_buffer, UART_BUFFER_SIZE);
		frame_count++;
		//memcpy(digital_input_buffer, uart_rx_buffer, UART_BUFFER_SIZE);
		//process_digital_input(digital_input_buffer, UART_BUFFER_SIZE);
		//printf("Something processed \n\n\n");

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
	 /*Temporal*/

	frame_count++;
	//printf("get_frames(); \n\n\n");

}

int get_frames(char* buffer, char** frames, int curr_len) {
	int frame_len;
	int frame_cnt = curr_len;
	while (get_next_frame_legth(buffer, &frame_len)
			|| frame_count < FRAME_BUFFER_SIZE) {
		memcpy(frames[frame_cnt], buffer, frame_len);
		frames[frame_cnt][frame_len] = '\0';
		strcpy(buffer, buffer[frame_len]);
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
	dac_is_free();

	if (available) {

		//printf("frame_ready \n\n\n");
		//modulate_frame(frame_buffer[0], output_buffer);
		//printf("frame modulated \n\n\n");
		send_dac(false);
	} else {
		//printf("dac_is_free FALSE \n\n\n");
	}
}

void dac_is_free(void) {

	adi_ad1854_IsTxBufAvailable(dac_dev, &available);
	/*	if(available && contar > 1){
	 printf("s");
	 }*/
	//return available;
}

void send_dac(bool do_send) {
	ADI_AD1854_RESULT result;
	result = adi_ad1854_GetTxBuffer(dac_dev, &dac_buffer);
	fract32* fr32_buffer = (fract32*) dac_buffer;
	if (do_send) {
		memcpy(fr32_buffer, entrada_dac, DAC_BUFFER_SIZE);
	} else {
		int i;
		for (i = 0; i < AUDIO_BUFFER_SIZE / 4; i++) {
			fr32_buffer[i] = 0;
		}
	}
	adi_ad1854_SubmitTxBuffer(dac_dev, dac_buffer, AUDIO_BUFFER_SIZE);
}

bool has_rx_frame_ready(void) {
	return 0;
}
void output_digital(void) {
}

void read_analog_input(void) {
	if (adc_buffer != NULL) {
		adi_ad1871_SubmitRxBuffer(adc_dev, adc_buffer, AUDIO_BUFFER_SIZE);
		adc_buffer = NULL;
	}
}

