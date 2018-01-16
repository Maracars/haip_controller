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

#define DIGITAL_INPUT_TIMEOUT 0.1
static const int MODULATE = true;
//Local variables
static ADI_UART_HANDLE uart_dev;
static ADI_AD1871_HANDLE dac_dev;
static ADI_AD1871_HANDLE adc_dev;

void *uart_rx_buffer, *adc_buffer, *dac_buffer, *uart_tx_buffer;

section ("sdram0") unsigned char digital_input_buffer[HAIP_UART_BUFFER_SIZE * 2];
section ("sdram0") unsigned char adc_entered[HAIP_ANALOG_BUFFER_SIZE];

section ("sdram0") unsigned char frame_buffer_list[HAIP_MAX_FRAMES][HAIP_FRAME_BUFFER_SIZE];

section ("sdram0") fract32 modulated_frame[HAIP_FRAME_SAMPLES_W_COEFFS];
section ("sdram0") unsigned char demodulated_out[HAIP_FRAME_MAX_LEN];

section ("sdram0") fract32 adc_channel_left[HAIP_ANALOG_BUFFER_SIZE / 4];
section ("sdram0") fract32 adc_channel_right[HAIP_ANALOG_BUFFER_SIZE / 4];

section ("sdram0") fract32 dac_tmp_fr32[HAIP_ANALOG_BUFFER_SIZE / 4];
section ("sdram0") fract32 adc_tmp_fr32[HAIP_ANALOG_BUFFER_SIZE / 4];

double last_digital_input_t = 0;
bool reception_on_course = false;
int alreadyCopiedOffset = 0;
int expected_byte_cnt = 0;
int send_list_offset = 0;
int frame_count = 0;

//Declarations
void output_analog(void);
void output_digital(int frame_length);
void read_analog_input(void);
void read_digital_input(void);
void process_digital_input(unsigned char* buffer, int size);
bool dac_is_free(void);
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

	bool uart_tx_free = MODULATE;
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
		adi_uart_SubmitRxBuffer(uart_dev, uart_rx_buffer, HAIP_UART_BUFFER_SIZE);

		process_digital_input(digital_input_buffer, HAIP_UART_BUFFER_SIZE);
	}
}

void process_digital_input(unsigned char* buffer, int size) {
	int length = 0;
	//check timeout to avoid unwanted bytes
	if(check_timeout(last_digital_input_t, DIGITAL_INPUT_TIMEOUT, &last_digital_input_t)){
		reception_on_course = false;
		expected_byte_cnt = 0;
		alreadyCopiedOffset = 0;
	}

	//Check if there is already a part of a frame received or if it is the first one
	if (!reception_on_course) {
		length = ((haip_header_t*) &digital_input_buffer[0])->len;
		length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;
	} else {
		length = expected_byte_cnt;
	}

	//TODO: send_list_offset + frame_count > HAIP_MAX_FRAME
	memcpy(&(frame_buffer_list[send_list_offset + frame_count][alreadyCopiedOffset]), &(digital_input_buffer[0]), size);

	expected_byte_cnt = length - size;
	alreadyCopiedOffset += size;

	//check if the complete frame has been received
	if (expected_byte_cnt != 0) {
		reception_on_course = MODULATE;
	} else {
		frame_count++;
		reception_on_course = false;
		alreadyCopiedOffset = 0;
	}
}

bool dac_is_free(void) {
	bool available;
	adi_ad1854_IsTxBufAvailable(dac_dev, &available);
	return available;
}

void output_analog() {
	bool modulate;

	if (dac_is_free()) {
		modulate = frame_count >= 1;
		if (modulate) {
			frame_count--;
		}
		send_dac(modulate);
	}
}

void send_dac(bool do_send) {
	ADI_AD1854_RESULT result;
	int i = 0;
	int length = 0;
	int modulated_length = 0;

	result = adi_ad1854_GetTxBuffer(dac_dev, &dac_buffer);

	if (do_send) {

		memset(modulated_frame, 0, HAIP_FRAME_SAMPLES_W_COEFFS);

		//get length of frame
		length = ((haip_header_t*) &(frame_buffer_list[send_list_offset][0]))->len;
		length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;

		//calculate the length of the modulated signal
		modulated_length = ((length * HAIP_SYMBOLS_PER_BYTE * HAIP_CODING_RATE)
				+ HAIP_PREAMBLE_SYMBOLS) * HAIP_OVERSAMPLING_FACTOR
				+ HAIP_SRCOS_COEFF_NUM;

		//modulate
		fract32 kk = haip_modulate_frame(
				&(frame_buffer_list[send_list_offset][0]), length,
				modulated_frame);

		//shift the offset of the list of frames to send
		if (send_list_offset == (HAIP_MAX_FRAMES - 1)) {
			send_list_offset = 0;
		} else {
			send_list_offset++;
		}

		//copy modulated frame into a temporal buffer
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE / 4; i = i + 2) {
			if ((i / 2) < modulated_length) {
				dac_tmp_fr32[i] = (modulated_frame[i / 2] >> 8); //left channel
				dac_tmp_fr32[i + 1] = (modulated_frame[i / 2] >> 8); //right channel
			} else {
				dac_tmp_fr32[i] = 0;
				dac_tmp_fr32[i + 1] = 0;
			}

		}
	} else {
		//fill temporal buffer with 0
		for (i = 0; i < HAIP_ANALOG_BUFFER_SIZE / 4; i++) {
			dac_tmp_fr32[i] = float_to_fr32(0) >> 8;
		}
	}

	//copy temporal buffer into the DAC buffer
	memcpy(dac_buffer, dac_tmp_fr32, HAIP_ANALOG_BUFFER_SIZE);

	//send DAC
	adi_ad1854_SubmitTxBuffer(dac_dev, dac_buffer, HAIP_ANALOG_BUFFER_SIZE);
	dac_buffer = NULL;

}

void read_analog_input(void) {
	bool adcAvailable;
	ADI_AD1871_RESULT Result;
	Result = adi_ad1871_IsRxBufAvailable(adc_dev, &adcAvailable);

	/* IF (AD1871 Buffer available) */
	if (adcAvailable) {
		/* Get AD1871 processed buffer address */
		Result = adi_ad1871_GetRxBuffer(adc_dev, &adc_buffer);

		memcpy(adc_tmp_fr32, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);

		adi_ad1871_SubmitRxBuffer(adc_dev, adc_buffer, HAIP_ANALOG_BUFFER_SIZE);

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
			//Check for a voltage peak in the right channel
			holder_ptr = &(adc_tmp_fr32[1 + offset + sample_length * 2]);
			offset = haip_next_data(holder_ptr, HAIP_ANALOG_BUFFER_SIZE / 4 - (1 + offset + sample_length * 2));

			//When a peak is detected, demodulate the frame
			if(offset != -1){

				if(offset + HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM > (HAIP_ANALOG_BUFFER_SIZE / 4))
					break;

				//Remove base noise (calculate the average noise and shift it to 0 level)
				for (i = 0; i < HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM * 2; i++) {
					adc_channel_right[i] = (adc_tmp_fr32[1 + offset + 2*i] << 8) - haip_get_noise_lvl_fr32();
				}

				memset(demodulated_out,0,HAIP_FRAME_MAX_LEN);

				//demodulate header and get the sync values (phase deviation, attenuation, sample index)
				sync = haip_demodulate_head(adc_channel_right, demodulated_out);


				//get frame length
				length = ((haip_header_t*) &demodulated_out[0])->len;
				length += HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN;

				//calculate modulated signal total length
				sample_length = length * HAIP_CODING_RATE * HAIP_SYMBOLS_PER_BYTE * HAIP_OVERSAMPLING_FACTOR;
				sample_length += HAIP_SRCOS_COEFF_NUM*2 + (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR);

				if(offset + sample_length > (HAIP_ANALOG_BUFFER_SIZE / 4))
					break;

				//Remove base noise (calculate the average noise and shift it to 0 level)
				for (; i < sample_length; i++) {
					adc_channel_right[i] = (adc_tmp_fr32[1 + offset + 2*i] << 8) - haip_get_noise_lvl_fr32();
				}

				//demodulate the rest of the frame (data + crc)
				haip_demodulate_payload(adc_channel_right, length, sync, demodulated_out);

				//send output through UART
				output_digital(length);
			}
		}
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
