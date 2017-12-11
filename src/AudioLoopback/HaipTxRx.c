/*
 * HaipTxRx.c
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */
#include "HaipTxRx.h"
#include "HaipCommons.h"

//definitions

#define DIGITAL_INPUT_TIMEOUT 0.02
#define DIGITAL_INPUT_BUFFER_SIZE 500
#define FRAME_BUFFER_SIZE 10
#define AUDIO_BUFFER_SIZE 65536*2
#define DAC_BUFFER_SIZE 900

//Local variables
char digital_input_buffer[DIGITAL_INPUT_BUFFER_SIZE] = "";
double last_digital_input_t = 0;
char frame_buffer[FRAME_BUFFER_SIZE][HAIP_FRAME_LENGTH_MAX];
int frame_count = 0;

//Declarations
void has_tx_frame_ready(void);
void has_rx_frame_ready(void);
void output_analog(void);
void output_digital(void);
void read_analog_input(void);
void read_digital_input(void);
void process_digital_input(char* buffer, int size);
int get_next_frame_legth(char* buffer);
bool dac_is_free(void);
int get_frames(char* buffer, char** frames, int curr_len);
void send_dac(fract32* output_buffer);

//GLOBAL FUNCTIONS

void haiptxrx_iterate() {

	read_digital_input(); //Get bytes from serial (care timeouts)
	output_analog(); //Transmit it

	read_analog_input();
	if (has_rx_frame_ready()) { //If full frame received
		output_digital(); //Send through serial
	}

}

//LOCAL FUNCTIONS

void read_digital_input(void) {
	bool uart_is_free;
	ADI_UART_RESULT uart_result;
	unsigned char* uart_rx_buffer;
	char* digital_input_buffer;

	uart_result = adi_uart_IsRxBufferAvailable(uart_device, &uart_is_free);
	if (uart_is_free) {
		uart_result = adi_uart_GetRxBuffer(uart_device, &uart_rx_buffer);
		memcpy(uart_rx_buffer, digital_input_buffer, UART_BUFFER_SIZE);
		process_digital_input(digital_input_buffer, UART_BUFFER_SIZE);
	}
}

void process_digital_input(char* buffer, int size) {

	if (check_timeout(last_digital_input_t, DIGITAL_INPUT_TIMEOUT,
			&last_digital_input_t)) {
		strcpy(digital_input_buffer, buffer);
	} else {
		if (strlen(digital_input_buffer)
				+ strlen(buffer) >= DIGITAL_INPUT_BUFFER_SIZE) {
			printf(
					"ERROR: Overflow detected on uart input buffer, emptying buffer");
			digital_input_buffer[0] = '\0';
		}
		strcat(digital_input_buffer, buffer);
	}

	frame_count += get_frames(digital_input_buffer, frame_buffer, frame_count);

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
	if (dac_is_free()) {
		fract32 output_buffer[DAC_BUFFER_SIZE];
		if (has_tx_frame_ready()) {
			modulate_frame(frame_buffer[0], output_buffer);
			send_dac(output_buffer, true);
		} else {
			send_dac(output_buffer, false);
		}
	}
}

bool dac_is_free(void) {
	ADI_AD1854_RESULT result;
	bool available;
	result = adi_ad1854_IsTxBufAvailable(dac_device, &available);
	return available;
}

void send_dac(fract32* output_buffer, bool do_send) {
	ADI_AD1854_RESULT result;
	void* buffer;
	result = adi_ad1854_GetTxBuffer(dac_device, &buffer);
	fract32* fr32_buffer = (fract32*) buffer;
	if(do_send){
		memcpy(fr32_buffer, output_buffer, DAC_BUFFER_SIZE);
	}
	else{
		int i;
		for(i = 0; i < DAC_BUFFER_SIZE; i++){
			fr32_buffer[i] = 0;
		}
	}
	adi_ad1854_SubmitTxBuffer(dac_device, buffer, AUDIO_BUFFER_SIZE)
}
