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
#include "haip_data_detector.h"

#include "fract2float_conv.h"

//Local variables
section ("sdram0") unsigned char digital_input_buffer[HAIP_UART_BUFFER_SIZE * 2];
double last_digital_input_t = 0;
section ("sdram0") unsigned char frame_buffer[HAIP_MAX_FRAMES][HAIP_FRAME_BUFFER_SIZE];
section ("sdram0") fract32 adc_channel_left[HAIP_ANALOG_BUFFER_SIZE / 4];
section ("sdram0") fract32 adc_channel_right[HAIP_ANALOG_BUFFER_SIZE / 4];

//section ("sdram0") fract32 output_buffer[HAIP_DAC_BUFFER_SIZE];
void *uart_rx_buffer, *adc_buffer, *dac_buffer, *uart_tx_buffer;

int frame_count = 0;
int kontadorea = 0;

static ADI_UART_HANDLE uart_dev;
static ADI_AD1871_HANDLE dac_dev;
static ADI_AD1871_HANDLE adc_dev;

section ("sdram0") unsigned char uart_test[HAIP_UART_BUFFER_SIZE];

section ("sdram0") fract32 modulated_frame[HAIP_FRAME_SAMPLES_W_COEFFS];
section ("sdram0") unsigned char adc_entered[HAIP_ANALOG_BUFFER_SIZE];
section ("sdram0") unsigned char demodulated_out[HAIP_UART_BUFFER_SIZE];

bool probetako = false;
bool halfPacket = false;
int alreadyCopiedOffset = 0;
int halfPacketLeft = 0;
int send_list_offset = 0;

section ("sdram0") fract32 ptr_32[HAIP_ANALOG_BUFFER_SIZE / 4];

//Declarations
bool has_tx_frame_ready(void);
bool has_rx_frame_ready(void);
void output_analog(void);
void output_digital(int frame_length);
void read_analog_input(void);
void read_digital_input(void);
void process_digital_input(unsigned char* buffer, int size);
int get_next_frame_legth(char* buffer, int* flen);
bool dac_is_free(void);
int get_frames(char* buffer, char** frames, int curr_len);
void send_dac(bool do_send);
void process_adc_input(void);

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
	output_analog();
	read_analog_input();

	return false;
}

//LOCAL FUNCTIONS

void read_digital_input(void) {
	ADI_UART_RESULT res;
	bool uart_has_buffer_ready;
	int length = 0;
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
	int length = 0;
	int processed_bytes = 0;
	while (processed_bytes < size) {
		if (!halfPacket) {
			length =
					((haip_header_t*) &digital_input_buffer[processed_bytes])->len;
			length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;
		} else {
			length = halfPacketLeft;
		}

		processed_bytes += length;

		memcpy(
				&(frame_buffer[send_list_offset + frame_count][alreadyCopiedOffset]),
				&(digital_input_buffer[processed_bytes - length]), length);

		if (processed_bytes > HAIP_UART_BUFFER_SIZE) {
			halfPacket = true;
			halfPacketLeft = processed_bytes - HAIP_UART_BUFFER_SIZE;
			alreadyCopiedOffset = length - halfPacketLeft;
		} else {
			frame_count++;
			halfPacket = false;
			alreadyCopiedOffset = 0;
			halfPacketLeft = 0;
		}

	}

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

bool dac_is_free(void) {
	bool available;
	adi_ad1854_IsTxBufAvailable(dac_dev, &available);
	return available;
}

void output_analog() {

	if (dac_is_free()) {
		if (frame_count >= 1) {
			send_dac(true);
			frame_count--;
		} else {
			send_dac(false);
		}
	}
}

void send_dac(bool do_send) {
	ADI_AD1854_RESULT result;
	int i = 0;
	int length = 0;
	int modulated_length = 0;

	result = adi_ad1854_GetTxBuffer(dac_dev, &dac_buffer);

	if (do_send) {
		length = ((haip_header_t*) &frame_buffer[send_list_offset][0])->len;
		length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;
		modulated_length = ((length * HAIP_SYMBOLS_PER_BYTE * HAIP_CODING_RATE)
				+ HAIP_PREAMBLE_SYMBOLS) * HAIP_OVERSAMPLING_FACTOR
				+ HAIP_SRCOS_COEFF_NUM;
		fract32 kk = haip_modulate_frame(frame_buffer[send_list_offset], length,
				modulated_frame);

		if(send_list_offset == (HAIP_MAX_FRAMES-1)){
			send_list_offset = 0;
		} else {
			send_list_offset++;
		}

		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE / 4; i = i + 2) {
			if ((i / 2) < modulated_length) {
				ptr_32[i] = (modulated_frame[i / 2] >> 8); //left channel
				ptr_32[i + 1] = (modulated_frame[i / 2] >> 8); //right channel
			} else {
				ptr_32[i] = 0;
				ptr_32[i + 1] = 0;
			}

		}
		probetako = true; //TODO: kendu, debugeetako bakarrik da
	} else {
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE / 4; i++) {
			ptr_32[i] = float_to_fr32(0) >> 8;
		}
	}
	memcpy(dac_buffer, ptr_32, HAIP_ANALOG_BUFFER_SIZE);
	if (probetako) {
		i = 0;
	}
	adi_ad1854_SubmitTxBuffer(dac_dev, dac_buffer, HAIP_ANALOG_BUFFER_SIZE);
	dac_buffer = NULL;

}

bool has_rx_frame_ready(void) {
	return 0;
}

void read_analog_input(void) {
	bool adcAvailable;
	ADI_AD1871_RESULT Result;
	Result = adi_ad1871_IsRxBufAvailable(adc_dev, &adcAvailable);

	/* IF (AD1871 Buffer available) */
	if (adcAvailable) {
		/* Get AD1871 processed buffer address */
		Result = adi_ad1871_GetRxBuffer(adc_dev, &adc_buffer);

		memcpy(ptr_32, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);
		//TODO kendo, debugeetako bakarrik da
		kontadorea++;
		if (kontadorea > 20) {
			kontadorea = 25;
		}
		adi_ad1871_SubmitRxBuffer(adc_dev, adc_buffer,
		HAIP_ANALOG_BUFFER_SIZE);
		process_adc_input();
		adc_buffer = NULL;

	}
}

void process_adc_input(void) {
	int if_frame_received = 0;
	haip_sync_t sync;
	int length = 0, sample_length = 0;
	int offset = 0;
	int i = 0;
	fract32* holder_ptr;

	while(offset != -1){
		holder_ptr = &(ptr_32[1+offset + sample_length * 2]);
		offset = haip_next_data(holder_ptr, HAIP_ANALOG_BUFFER_SIZE / 4 - (offset + sample_length));
		if(offset != -1){
			if(offset + HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM > HAIP_ANALOG_BUFFER_SIZE / 4)
				break;
			for (i = 0; i < HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM * 2; i++) {
				adc_channel_left[i] = (ptr_32[1+offset + 2*i] << 8) - haip_get_noise_lvl_fr32();
			}
			sync = haip_demodulate_head(adc_channel_left, demodulated_out);
			length = ((haip_header_t*) &demodulated_out[0])->len;
			length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;
			sample_length = length * HAIP_CODING_RATE * HAIP_SYMBOLS_PER_BYTE * HAIP_OVERSAMPLING_FACTOR;
			sample_length += HAIP_SRCOS_COEFF_NUM*2 + (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR);
			if(offset + sample_length > HAIP_ANALOG_BUFFER_SIZE / 4)
				break;
			for (; i < sample_length; i++) {
				adc_channel_left[i] = (ptr_32[1+offset + 2*i] << 8) - haip_get_noise_lvl_fr32();
			}
			haip_demodulate_payload(adc_channel_left, length, sync, demodulated_out);
			output_digital(length);
		}
	}

	/*
	for (i = 0; i < ; i++) {

		if (i % 2) {
			adc_channel_right[i / 2] = (ptr_32[i]) << 8;
		} else {
			adc_channel_left[i / 2] = (ptr_32[i]) << 8;
		}

		if ((fr32_to_float(adc_channel_left[i / 2]) > 0.2)
				|| (fr32_to_float(adc_channel_left[i / 2]) < -0.2)) {
			if (!offset)
				offset = i / 2;
			if_frame_received++;
		}

	}

	if (if_frame_received) {
		sync = haip_demodulate_head(&adc_channel_left[offset], demodulated_out);
		length = ((haip_header_t*) &demodulated_out[0])->len;
		length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;
		haip_demodulate_payload(&adc_channel_left[offset], length, sync,
				demodulated_out);
		output_digital(length);
		if_frame_received = 0;
	}
	*/
}

void output_digital(int frame_length) {
	bool uart_tx_free = 0;
	int i = 0;
	adi_uart_IsTxBufferAvailable(uart_dev, &uart_tx_free);
	if (uart_tx_free) {
		adi_uart_GetTxBuffer(uart_dev, (void**) &uart_tx_buffer);
		memcpy(uart_tx_buffer, demodulated_out, frame_length);
		adi_uart_SubmitTxBuffer(uart_dev, uart_tx_buffer, frame_length);
	}

}
