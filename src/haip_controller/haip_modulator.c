/*
 * haip_modulator.c
 *
 *  Created on: 19/12/2017
 *      Author: joseb
 */

#include "haip_modulator.h"
#include "haip_modem.h"
#include "haip_srcos_filter_fr32.h"
#include "haip_16QAM_mapping.h"
#include "haip_hamming_7_4_ext_coding.h"
#include <filter.h>
#include <fract2float_conv.h>
#include <math.h>
#include "haip_commons.h"

//FUNCTIONS
void addPreamble(void);
void mapper(unsigned char* frame_buffer, int frame_symbols);
void upsample(int frame_symbols);
void filter(int length);
void oscilate(fract32* modulated_signal, int length);

//VARS
segment ("sdram0") static float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") static float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };

segment ("sdram0") static float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071,
		0, -0.7071, -1, -0.7071 };
segment ("sdram0") static float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071,
		-1, -0.7071, 0, 0.7071 };

segment ("sdram0") static fract32 frame_symbols_real[HAIP_FRAME_MAX_SYMBOLS];
segment ("sdram0") static fract32 frame_symbols_imag[HAIP_FRAME_MAX_SYMBOLS];

segment ("sdram0") static fract32 frame_symbols_real_upsample[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static fract32 frame_symbols_imag_upsample[HAIP_FRAME_SAMPLES_W_COEFFS];

segment ("sdram0") static fract32 filtered_real_symbols[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static fract32 filtered_imag_symbols[HAIP_FRAME_SAMPLES_W_COEFFS];

fir_state_fr32 state_real;
fir_state_fr32 state_imag;

#pragma section("L1_data_b")
fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

segment ("sdram0") static unsigned char frame_code[HAIP_FRAME_MAX_LEN
		* HAIP_CODING_RATE];
int kont = 0;

int haip_modulate_frame(unsigned char* frame_buffer, int frame_length,
		fract32* modulated_signal) {
	int frame_symbols = frame_length * HAIP_SYMBOLS_PER_BYTE;
	kont++;
	if (kont == 2) {
		kont = 0;
	}
	haip_hamming_7_4_ext_code(frame_buffer, frame_code, frame_symbols);

	addPreamble();
	mapper(frame_code, frame_symbols * HAIP_CODING_RATE);
	upsample(frame_symbols * HAIP_CODING_RATE + HAIP_PREAMBLE_SYMBOLS);
	filter(frame_symbols * HAIP_CODING_RATE + HAIP_PREAMBLE_SYMBOLS);
	oscilate(modulated_signal,
			frame_symbols * HAIP_CODING_RATE + HAIP_PREAMBLE_SYMBOLS);

	return 0;
}

void addPreamble() {
	int i = 0;

	for (i = 0; i < HAIP_PREAMBLE_SYMBOLS; i++) {
		frame_symbols_real[i] = float_to_fr32(preamble_real[i] / SQRT_2);
		frame_symbols_imag[i] = float_to_fr32(preamble_imag[i] / SQRT_2);
	}
}

void mapper(unsigned char* frame_buffer, int frame_length) {
	int i = 0;
	int numDecimal = 0;

	for (i = 0; i < frame_length; i++) {
		if (i % 2 != 0)
			numDecimal = frame_buffer[i / 2] & 0xF;
		else
			numDecimal = ((frame_buffer[i / 2] & 0xF0) >> 4);

		frame_symbols_real[i + HAIP_PREAMBLE_SYMBOLS] =
				haip_const[numDecimal][0];
		frame_symbols_imag[i + HAIP_PREAMBLE_SYMBOLS] =
				haip_const[numDecimal][1];
	}
}

void upsample(int frame_length) {
	int i = 0;
	int oversampled_length = frame_length * HAIP_OVERSAMPLING_FACTOR;

	for (i = 0; i < oversampled_length; i++) {
		if (i % HAIP_OVERSAMPLING_FACTOR == 0) {
			frame_symbols_imag_upsample[i] = frame_symbols_imag[i
					/ HAIP_OVERSAMPLING_FACTOR];
			frame_symbols_real_upsample[i] = frame_symbols_real[i
					/ HAIP_OVERSAMPLING_FACTOR];
		} else {
			frame_symbols_imag_upsample[i] = 0;
			frame_symbols_real_upsample[i] = 0;
		}
	}
}

void filter(int length) {
	int i = 0;

	for (i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	for (i = 0; i < length * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM; i++) {
		filtered_real_symbols[i] = 0;
		filtered_imag_symbols[i] = 0;
	}

	for (i = length * HAIP_OVERSAMPLING_FACTOR;
			i < length * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM; i++) {
		frame_symbols_real_upsample[i] = 0;
		frame_symbols_imag_upsample[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	//Filters the signal
	fir_fr32(frame_symbols_real_upsample, filtered_real_symbols,
			length * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM,
			&state_real);
	fir_fr32(frame_symbols_imag_upsample, filtered_imag_symbols,
			length * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM,
			&state_imag);

}

void oscilate(fract32* modulated_signal, int length) {
	int i = 0;

	for (i = 0; i < length * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM;
			++i) {
		modulated_signal[i] =
				(filtered_real_symbols[i + HAIP_SRCOS_FILTER_DELAY]
						* cos_modulator_6KHz[i % HAIP_OVERSAMPLING_FACTOR]
						- filtered_imag_symbols[i + HAIP_SRCOS_FILTER_DELAY]
								* sin_modulator_6KHz[i
										% HAIP_OVERSAMPLING_FACTOR]) * SQRT_2;
	}
}
