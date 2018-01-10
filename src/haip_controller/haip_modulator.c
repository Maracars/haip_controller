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
#include <fract2float_conv.h>
#include <math.h>

//Need to be normalized
segment ("sdram0") static float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") static float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };

segment ("sdram0") static float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071,
		0, -0.7071, -1, -0.7071 };
segment ("sdram0") static float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071,
		-1, -0.7071, 0, 0.7071 };

segment ("sdram0") static fract32 frame_symbols_real[50];
segment ("sdram0") static fract32 frame_symbols_imag[50];

segment ("sdram0") static fract32 frame_symbols_real_upsample[(10 * 2 + 8) * 8
		+ 100];
segment ("sdram0") static fract32 frame_symbols_imag_upsample[(10 * 2 + 8) * 8
		+ 100];
segment ("sdram0") static fract32 modulated_synchronization[(10 * 2 + 8) * 8
		+ 100];

segment ("sdram0") static fract32 filtered_real_symbols[(10 * 2 + 8) * 8 + 100];
segment ("sdram0") static fract32 filtered_imag_symbols[(10 * 2 + 8) * 8 + 100];

fir_state_fr32 state_real;
fir_state_fr32 state_imag;

fir_state_fr32 nstate_real;
fir_state_fr32 nstate_imag;

#pragma section("L1_data_b")
fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

fract32 ndelay_real[HAIP_SRCOS_COEFF_NUM];
fract32 ndelay_imag[HAIP_SRCOS_COEFF_NUM];

segment ("sdram0") static unsigned char frame_code[25];

segment ("sdram0") char prueba;

bool packetReceivedADC;

fract32 modulate_frame(unsigned char* frame_buffer, int frame_length,
		fract32* modulated_signal) {
	int frame_symbols = frame_length * HAIP_SYMBOLS_PER_BYTE;
	float kk, a1, a2;
	unsigned char a = frame_buffer[0];
	haip_hamming_7_4_ext_code(frame_buffer, frame_code, frame_symbols);
	addPreamble();
	mapper(frame_code, frame_symbols * HAIP_CODING_RATE);
	upsample(frame_symbols * HAIP_CODING_RATE + HAIP_PREAMBLE_SYMBOLS);

	filter();
	oscilate(modulated_signal);

	kk = fr32_to_float(frame_symbols_real_upsample[8 * 8]);
	a1 = fr32_to_float(filtered_real_symbols[8 * 8 + 24]);
	a2 = fr32_to_float(modulated_signal[8 * 8]);

	a = 0;
	return modulated_signal[0];
}

void addPreamble() {
	int i = 0;
	float re[9];
	for (i = 0; i < HAIP_PREAMBLE_SYMBOLS; i++) {
		frame_symbols_real[i] = float_to_fr32(preamble_real[i] / SQRT_2);
		frame_symbols_imag[i] = float_to_fr32(preamble_imag[i] / SQRT_2);
		re[i] = (preamble_real[i]);
	}
}

void mapper(unsigned char* frame_buffer, int frame_length) {
	int i = 0;
	int numDecimal = 0;
	float re[30];

	for (i = 0; i < frame_length; i++) {
		if (i % 2 != 0)
			numDecimal = frame_buffer[i / 2] & 0xF;
		else
			numDecimal = ((frame_buffer[i / 2] & 0xF0) >> 4);

		frame_symbols_real[i + HAIP_PREAMBLE_SYMBOLS] =
				haip_const[numDecimal][0];
		frame_symbols_imag[i + HAIP_PREAMBLE_SYMBOLS] =
				haip_const[numDecimal][1];
		re[i] = fr32_to_float(frame_symbols_real[i]);

	}
}

void upsample(int frame_length) {
	int i = 0;
	int oversampled_length = (frame_length) * HAIP_OVERSAMPLING_FACTOR;

	for (i = 0; i < oversampled_length; i++) {

		if (!(i % HAIP_OVERSAMPLING_FACTOR)) {

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

	for (i = (10 * 2 + 8) * 8; i < (10 * 2 + 8) * 8 + 49; i++) {
		frame_symbols_real_upsample[i] = 0;
		frame_symbols_imag_upsample[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	//Filters the signal
	fir_fr32(frame_symbols_real_upsample, filtered_real_symbols,
			(10 * 2 + 8) * 8 + HAIP_SRCOS_COEFF_NUM, &state_real);
	fir_fr32(frame_symbols_imag_upsample, filtered_imag_symbols,
			(10 * 2 + 8) * 8 + HAIP_SRCOS_COEFF_NUM, &state_imag);

}
void oscilate(fract32* modulated_signal) {
	int i = 0;
	fract32 aaas;
	float re[(10 * 2 + 8) * 8 + 49];
	float oo[(10 * 2 + 8) * 8 + 49];
	for (i = 0; i < (10 * 2 + 8) * 8 + 49; ++i) {
		modulated_signal[i] = (filtered_real_symbols[i + 24]
				* cos_modulator_6KHz[i % 8]
				- filtered_imag_symbols[i + 24] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
		aaas = (filtered_real_symbols[i + 24] * cos_modulator_6KHz[i % 8]
				- filtered_imag_symbols[i + 24] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
		re[i] = fr32_to_float(modulated_signal[i]);
		oo[i] = fr32_to_float(filtered_real_symbols[i + 24]);
	}

}
