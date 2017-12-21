/*
 * haip_modulator.c
 *
 *  Created on: 19/12/2017
 *      Author: joseb
 */
#include "haip_commons.h"
#include "haip_modulator.h"
#include "haip_modem.h"
#include "haip_srcos_filter_fr32.h"
#include "haip_16QAM_mapping.h"
#include "haip_hamming_7_4_ext_coding.h"
#include <filter.h>

//Need to be normalized
segment ("sdram0") float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };

segment ("sdram0") float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071, 0,
		-0.7071, -1, -0.7071 };
segment ("sdram0") float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071, -1,
		-0.7071, 0, 0.7071 };

segment ("sdram0") unsigned char frame_code[HAIP_PACKET_LENGTH];

segment ("sdram0") fract32 frame_symbols_real[HAIP_PACKET_LENGTH];
segment ("sdram0") fract32 frame_symbols_imag[HAIP_PACKET_LENGTH];

segment ("sdram0") fract32 frame_symbols_real_upsample[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") fract32 frame_symbols_imag_upsample[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") fract32 modulated_synchronization[HAIP_TX_PACKET_LENGTH];

segment ("sdram0") fract32 filtered_real_symbols[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") fract32 filtered_imag_symbols[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") fract32 modulated_signal[HAIP_TX_PACKET_LENGTH];


fir_state_fr32 state_real;
fir_state_fr32 state_imag;

#pragma section("L1_data_b")
fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
#pragma section("L1_data_b")
fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

fract32* modulate_frame(unsigned char* frame_buffer, int frame_length) {
	int frame_symbols = frame_length * HAIP_SYMBOLS_PER_BYTE;
	//haip_hamming_7_4_ext_code(frame_buffer,frame_code, frame_symbols);
	addPreamble();
	mapper(frame_code, frame_symbols);
	upsample(frame_symbols);
	filter();
	oscilate();
	return modulated_signal;
}

void addPreamble() {
	int i = 0;

	for (i = 0; i < HAIP_PREAMBLE_SYMBOLS; i++) {
		frame_symbols_real[i] = float_to_fr32(preamble_real[i]/SQRT_2);
		frame_symbols_imag[i] = float_to_fr32(preamble_imag[i]/SQRT_2);
	}
}

void mapper(unsigned char* frame_buffer, int frame_length) {
	int i = 0;
	int numDecimal = 0;

	for (i = HAIP_PREAMBLE_SYMBOLS; i < frame_length; i++) {
		if (i % 2 != 0)
			numDecimal = frame_buffer[i / 2] & 0xF;
		else
			numDecimal = ((frame_buffer[i / 2] >> 4) & 0xF);

		frame_symbols_real[i] = haip_ideal_const[numDecimal][0];
		frame_symbols_imag[i] = haip_ideal_const[numDecimal][1];

	}
}

void upsample(int frame_length) {
	int i = 0;
	int oversampled_length = (frame_length + HAIP_PREAMBLE_SYMBOLS)
			* HAIP_OVERSAMPLING_FACTOR;
	for (i = 0; i < HAIP_TX_PACKET_LENGTH; i++) {

		if (i % HAIP_OVERSAMPLING_FACTOR == 0 && i < oversampled_length) {

			/*Mikeleri galdetu behar dan edo ez*/
			if (i == 0) {
				modulated_synchronization[i] = 0x80000000;
			} else {
				modulated_synchronization[i] = 0x7FFFFFFF;
			}
			/***************************************/

			frame_symbols_imag_upsample[i] = frame_symbols_imag[i
					/ HAIP_OVERSAMPLING_FACTOR];
			frame_symbols_real_upsample[i] = frame_symbols_real[i
					/ HAIP_OVERSAMPLING_FACTOR];
		} else {
			frame_symbols_imag_upsample[i] = 0;
			frame_symbols_real_upsample[i] = 0;
			//Mikeleri preguntau behar dan
			modulated_synchronization[i] = 0;
		}
	}
}

void filter() {
	int i = 0;

	for (i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	//Filters the signal
	fir_fr32(frame_symbols_real_upsample, filtered_real_symbols,
	HAIP_TX_PACKET_LENGTH, &state_real);
	fir_fr32(frame_symbols_imag_upsample, filtered_imag_symbols,
	HAIP_TX_PACKET_LENGTH, &state_imag);

}
void oscilate() {
	int i = 0;
	float real = 0;
	float imaginary = 0;

	for (i = 0; i < HAIP_TX_PACKET_LENGTH; ++i) {
		real = filtered_real_symbols[i]
				* cos_modulator_6KHz[i % HAIP_OVERSAMPLING_FACTOR];
		imaginary = filtered_imag_symbols[i]
				* sin_modulator_6KHz[i % HAIP_OVERSAMPLING_FACTOR];
		modulated_signal[i] = (real - imaginary) * SQRT_2;
	}
}

