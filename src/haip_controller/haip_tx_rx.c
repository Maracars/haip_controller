/*
 * HaipTxRx.c
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */
#include "haip_tx_rx.h"
#include "haip_commons.h"
#include "haip_modulator.h"
#include "haip_demodulator.h"

//Local variables
section ("sdram0") unsigned char digital_input_buffer[HAIP_UART_BUFFER_SIZE];
section ("sdram0") unsigned char analog_input_buffer[HAIP_ANALOG_BUFFER_SIZE];
double last_digital_input_t = 0;
section ("sdram0") unsigned char frame_buffer[HAIP_FRAME_BUFFER_SIZE][HAIP_FRAME_LENGTH_MAX];
section ("sdram0") unsigned char demod_buffer[HAIP_UART_BUFFER_SIZE];
section ("sdram0") fract32* output_buffer;
unsigned char *uart_rx_buffer, *adc_buffer, *dac_buffer, *uart_tx_buffer;
int frame_count = 0;

static ADI_UART_HANDLE uart_dev;
static ADI_AD1871_HANDLE dac_dev;
static ADI_AD1871_HANDLE adc_dev;

section ("sdram0") unsigned char uart_test[HAIP_UART_BUFFER_SIZE];

unsigned char *tmp_buffer;

int contar = 0;
bool uart_has_buffer_ready;
bool adcAvailable = true;

//Declarations
bool has_tx_frame_ready(void);
void output_analog(void);
void output_digital(void);
void read_analog_input(void);
void read_digital_input(void);
void check_adc_available(void);
void process_digital_input(unsigned char* buffer, int size);
bool dac_is_free(void);
void send_dac(bool do_send);

//GLOBAL FUNCTIONS

void haiptxrx_init_devices(ADI_UART_HANDLE uart, ADI_AD1871_HANDLE dac, ADI_AD1871_HANDLE adc) {
	uart_dev = uart;
	dac_dev = dac;
	adc_dev = adc;
}

bool haiptxrx_iterate() {

	read_digital_input(); // Reads the UART data
	check_adc_available(); // If available fetches the ADC buffer pointer
	output_analog(); // Outputs if ready frames received from the UART after modulation
	read_analog_input(); // Reads and processes the buffer from the ADC

	return false;
}

void check_adc_available() {
	ADI_AD1871_RESULT Result;
	Result = adi_ad1871_IsRxBufAvailable(adc_dev, &adcAvailable);

	/* IF (Failure) */

	/* IF (AD1871 Buffer available) */
	if (adcAvailable) {
		/* Get AD1871 processed buffer address */
		Result = adi_ad1871_GetRxBuffer(adc_dev, (void**) &adc_buffer);

		/* IF (Failure) */

	}

}

//LOCAL FUNCTIONS

void read_digital_input(void) {
	ADI_UART_RESULT res;
	res = adi_uart_IsRxBufferAvailable(uart_dev, &uart_has_buffer_ready);
	if (uart_has_buffer_ready) {
		adi_uart_GetRxBuffer(uart_dev, (void**) &uart_rx_buffer);
		int next_frame_size = ((haip_header_t*) &uart_rx_buffer[0])->len
				+ HAIP_FRAME_CRC_LEN + HAIP_HEADER_AND_ADDR_LEN;
		memcpy(digital_input_buffer, uart_rx_buffer, next_frame_size);
		adi_uart_SubmitRxBuffer(uart_dev, uart_rx_buffer, next_frame_size);
		process_digital_input(digital_input_buffer, next_frame_size);
	}
}

void process_digital_input(unsigned char* buffer, int size) {
	memcpy(frame_buffer[frame_count], buffer, size);
	frame_count++;
}

bool has_tx_frame_ready() {
	return frame_count > 0;
}

void output_analog() {

	if (dac_is_free()) {
		if (has_tx_frame_ready()) {
			for (int i = 0; i < frame_count; i++) {
				int frame_size = ((haip_header_t*) &frame_buffer[i][0])->len + HAIP_FRAME_CRC_LEN + HAIP_HEADER_AND_ADDR_LEN;
				output_buffer = modulate_frame(frame_buffer[i], frame_size);
				send_dac(true);
			}
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
	result = adi_ad1854_GetTxBuffer(dac_dev, (void**) &dac_buffer);
	//fract32* fr32_buffer = (fract32*) dac_buffer;
	if (do_send) {
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE * 2; i++) {
			if (i >= HAIP_UART_BUFFER_SIZE && i % HAIP_UART_BUFFER_SIZE == 0) {
				j = 0;
			}
			tmp_buffer[i] = digital_input_buffer[j];
			j++;
		}
		//memcpy(dac_buffer, digital_input_buffer, AUDIO_BUFFER_SIZE);
	} else {
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE * 2; i++) {
			tmp_buffer[i] = 0;
		}
	}
	memcpy(dac_buffer, tmp_buffer, HAIP_ANALOG_BUFFER_SIZE);
	adi_ad1854_SubmitTxBuffer(dac_dev, dac_buffer, HAIP_ANALOG_BUFFER_SIZE);
}

void output_digital(void) {
	bool uart_tx_free = 0;

	adi_uart_IsTxBufferAvailable(uart_dev, &uart_tx_free);
	if (uart_tx_free) {
		adi_uart_GetTxBuffer(uart_dev, (void**) &uart_tx_buffer);
		memcpy(uart_tx_buffer, demod_buffer, HAIP_UART_BUFFER_SIZE);
		adi_uart_SubmitTxBuffer(uart_dev, uart_tx_buffer,
				HAIP_UART_BUFFER_SIZE);
	}
}

void read_analog_input(void) {
	if (adc_buffer != NULL) {
		memcpy(analog_input_buffer, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);
		adi_ad1871_SubmitRxBuffer(adc_dev, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);
		int index = 0;
		while(index < HAIP_ANALOG_BUFFER_SIZE - INIT_SAMPLES){
			if(analog_input_buffer[index] < DETECTION_THRESHOLD){
				index++;
			}else {
				haip_sync_t sync = haip_demodulate_head(&analog_input_buffer[index], demod_buffer);
				int len = ((haip_header_t*) &demod_buffer[0])->len + HAIP_FRAME_CRC_LEN;
				index += INIT_SAMPLES;
				int remaining_samples = len * HAIP_SYMBOLS_PER_BYTE * HAIP_OVERSAMPLING_FACTOR;
				if(index > HAIP_ANALOG_BUFFER_SIZE - remaining_samples)	break;
				haip_demodulate_payload(&analog_input_buffer[index], remaining_samples, sync, &demod_buffer[HAIP_HEADER_AND_ADDR_LEN]);
				index += remaining_samples;
				output_digital();
			}
		}
		adc_buffer = NULL;
	}
}

