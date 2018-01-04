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

segment ("sdram0") static fract32 frame_symbols_real[10];
segment ("sdram0") static fract32 frame_symbols_imag[10];

segment ("sdram0") static fract32 frame_symbols_real_upsample[10 * 8];
segment ("sdram0") static fract32 frame_symbols_imag_upsample[10 * 8];
segment ("sdram0") static fract32 modulated_synchronization[HAIP_TX_PACKET_LENGTH];

segment ("sdram0") static fract32 filtered_real_symbols[HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 filtered_imag_symbols[HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 modulated_signal[HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM];

fir_state_fr32 state_real;
fir_state_fr32 state_imag;

fir_state_fr32 nstate_real;
fir_state_fr32 nstate_imag;

#pragma section("L1_data_b")
fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

fract32 ndelay_real[HAIP_SRCOS_COEFF_NUM];
fract32 ndelay_imag[HAIP_SRCOS_COEFF_NUM];

segment ("sdram0") static fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 raw_samples_r[10 * HAIP_OVERSAMPLING_FACTOR+49*2];
segment ("sdram0") static fract32 raw_samples_i[10 * HAIP_OVERSAMPLING_FACTOR+49*2];
segment ("sdram0") static fract32 subsample_r[10 * HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 subsample_i[10 * HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 filtered_samples_r[10
		* HAIP_OVERSAMPLING_FACTOR + 49 * 2];
segment ("sdram0") static fract32 filtered_samples_i[10
		* HAIP_OVERSAMPLING_FACTOR + 49 * 2];
segment ("sdram0") static unsigned char ppp[10];
segment ("sdram0") static unsigned char frame_code[HAIP_FRAME_MAX_LEN*HAIP_CODING_RATE];
segment ("sdram0") static unsigned char frame_dem[HAIP_FRAME_MAX_LEN];


segment ("sdram0") char prueba;

bool packetReceivedADC;

fract32* modulate_frame(unsigned char* frame_buffer, int frame_length) {
	int frame_symbols = frame_length * HAIP_SYMBOLS_PER_BYTE;
	unsigned char t , r;
	int after_code[frame_length*HAIP_CODING_RATE];

	haip_hamming_7_4_ext_code(frame_buffer, frame_code, frame_symbols);
	addPreamble();
	for (int j = 0; j < frame_symbols*HAIP_CODING_RATE / HAIP_SYMBOLS_PER_BYTE; ++j) {
		t = frame_code[j];
	}
	mapper(frame_code, frame_symbols*HAIP_CODING_RATE);
	upsample(frame_symbols*HAIP_CODING_RATE + HAIP_PREAMBLE_SYMBOLS);
	filter();
	oscilate();

	get_quadrature_inphase_test(frame_symbols * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM);
	filter_sqrcosine_test(frame_symbols * HAIP_OVERSAMPLING_FACTOR);
	subsample_test(HAIP_SRCOS_FILTER_DELAY + HAIP_PREAMBLE_SYMBOLS*8, HAIP_OVERSAMPLING_FACTOR, frame_symbols * HAIP_OVERSAMPLING_FACTOR);
	demap_16QAM_test(frame_symbols * HAIP_CODING_RATE, 0, frame_code);
	haip_hamming_7_4_ext_decode(frame_code, frame_dem, frame_symbols * HAIP_CODING_RATE / 2);
	for(int i = 0; i<frame_length; i++){
		t = frame_buffer[i];
		r = frame_dem[i];
	}


	return modulated_signal;
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

	float re[40];
	float im[40];

	for (i = 0; i < frame_length; i++) {
		if (i % 2 != 0)
			numDecimal = frame_buffer[i / 2] & 0xF;
		else
			numDecimal = ((frame_buffer[i / 2] & 0xF0) >> 4);

		frame_symbols_real[i + HAIP_PREAMBLE_SYMBOLS] = haip_const[numDecimal][0];
		frame_symbols_imag[i + HAIP_PREAMBLE_SYMBOLS] = haip_const[numDecimal][1];

		re[i] = haip_ideal_const[numDecimal][0];
		im[i] = haip_ideal_const[numDecimal][1];

	}
}

void upsample(int frame_length) {
	int i = 0;
	int oversampled_length = (frame_length + HAIP_PREAMBLE_SYMBOLS) * HAIP_OVERSAMPLING_FACTOR;
	float re[HAIP_TX_PACKET_LENGTH];
	float im[HAIP_TX_PACKET_LENGTH];

	for (i = 0; i < oversampled_length; i++) {

		if (i % HAIP_OVERSAMPLING_FACTOR == 0) {

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
			re[i] = fr32_to_float(
					frame_symbols_real[i / HAIP_OVERSAMPLING_FACTOR]);
			im[i] = fr32_to_float(
					frame_symbols_imag[i / HAIP_OVERSAMPLING_FACTOR]);
		} else {
			re[i] = 0;
			im[i] = 0;
			frame_symbols_imag_upsample[i] = 0;
			frame_symbols_real_upsample[i] = 0;
			//Mikeleri preguntau behar dan
			modulated_synchronization[i] = 0;
		}
	}
}

void filter() {
	int i = 0;
	float re[HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM];
	float im[HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM];

	for (i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real, HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag, HAIP_SRCOS_COEFF_NUM, 0);

	//Filters the signal
	fir_fr32(frame_symbols_real_upsample, filtered_real_symbols, HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM, &state_real);
	fir_fr32(frame_symbols_imag_upsample, filtered_imag_symbols, HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM, &state_imag);

	for (i = 0; i < HAIP_TX_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM; i++) {
		re[i] = fr32_to_float(filtered_real_symbols[i]);
		im[i] = fr32_to_float(filtered_imag_symbols[i]);
	}

}
void oscilate() {
	int i = 0;
	float mod[149];

	for (i = 0; i < HAIP_PACKET_LENGTH + HAIP_SRCOS_COEFF_NUM; ++i) {
		modulated_signal[i] = (filtered_real_symbols[i + HAIP_SRCOS_FILTER_DELAY] * cos_modulator_6KHz[i % 8]
				- filtered_imag_symbols[i + HAIP_SRCOS_FILTER_DELAY] * sin_modulator_6KHz[i % 8]) * SQRT_2;
	}
	for (i = 0; i < 149; ++i) {
		mod[i] = fr32_to_float(modulated_signal[i]);
	}
}

void demap_16QAM_test(int len, double phase_off, unsigned char* frame_buffer) {
	int i = 0;
	float re_dist = 0;
	float im_dist = 0;
	char mychar = 0;
	uint8_t j = 0, s;
	fract32 r1 = 0;
	fract32 i1 = 0;
	fract32 r2 = 0;
	fract32 i2 = 0;
	float div_r = 0;
	float div_i = 0;

	float mindist, dist, dist_t;
	unsigned char ddd[len];
	unsigned char a = 0, b;
	for (i = 0; i < len / HAIP_SYMBOLS_PER_BYTE; ++i) {
		ppp[i] = 0;
	}
	ddd[0] = 0;
	for (i = 0; i < len; i++) {
		mindist = 200; //Big number
		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			re_dist = fr32_to_float(haip_const[j][0])
					- fr32_to_float(subsample_r[i]);
			im_dist = fr32_to_float(haip_const[j][1])
					- fr32_to_float(subsample_i[i]);
			dist = sqrt(re_dist * re_dist + im_dist * im_dist);
			if (dist < mindist) {
				mindist = dist;
				s = j;
			}
		}
		if (i > 3) {
			a = ppp[i / 2];
		}
		if (i % 2 == 0) {
			mychar = mychar | (s << 4);
		} else {
			mychar = mychar | s;
			ppp[i / 2] = mychar;
			mychar = 0;
		}
	}
	for (i = 0; i < len / 2; ++i) {
		r1 = subsample_r[i];
		r2 = frame_symbols_real[i] * cos_modulator_6KHz[i] * SQRT_2;
		i1 = subsample_i[i];
		i2 = -frame_symbols_imag[i] * sin_modulator_6KHz[i] * SQRT_2;

		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			re_dist = fr32_to_float(haip_const[j][0])
					- fr32_to_float(frame_symbols_real[i]);
			im_dist = fr32_to_float(haip_const[j][1])
					- fr32_to_float(frame_symbols_imag[i]);
			dist = sqrt(re_dist * re_dist + im_dist * im_dist);

			re_dist = fr32_to_float(haip_const[j][0])
					- fr32_to_float(subsample_r[i]);
			im_dist = fr32_to_float(haip_const[j][1])
					- fr32_to_float(subsample_i[i]);
			dist_t = sqrt(re_dist * re_dist + im_dist * im_dist);
		}

		a = ppp[i];
		b = frame_buffer[i];
	}
}

void get_quadrature_inphase_test(int buffer_len) {
	float re[150];
	float im[150];
	for (int i = 0; i < buffer_len; i++) {
		raw_samples_r[i] = (modulated_signal[i] * cos_modulator_6KHz[i % 8])
				* SQRT_2;
		raw_samples_i[i] = (-modulated_signal[i] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
		re[i] = fr32_to_float(raw_samples_r[i]);
		im[i] = fr32_to_float(raw_samples_i[i]);
	}
}

void subsample_test(int n, int t, int len) {
	int indx = 0;
	float re[160];
	float im[160];
	while (len+n - indx * t >= t) {
		subsample_r[indx] = filtered_samples_r[indx * t + n];
		subsample_i[indx] = filtered_samples_i[indx * t + n];
		indx++;
	}
	for (int i = 0; i < 15; i++) {
		re[i] = fr32_to_float(subsample_r[i]);
		im[i] = fr32_to_float(subsample_i[i]);
	}
}

void filter_sqrcosine_test(int len) {
	float re[200];
	float im[200];

	for (int i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		ndelay_real[i] = 0;
		ndelay_imag[i] = 0;
	}

	fir_init(nstate_real, haip_srcos_fir_fil_coeffs_fr32, ndelay_real, HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(nstate_imag, haip_srcos_fir_fil_coeffs_fr32, ndelay_imag, HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(raw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM * 2, &nstate_real);
	fir_fr32(raw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM * 2, &nstate_imag);

	for (int i = 0; i < len + HAIP_SRCOS_COEFF_NUM * 2; i++) {
		re[i] = fr32_to_float(filtered_samples_r[i]);
		im[i] = fr32_to_float(filtered_samples_i[i]);
	}
}
