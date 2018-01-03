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

segment ("sdram0") static fract32 frame_symbols_real[HAIP_PACKET_LENGTH];
segment ("sdram0") static fract32 frame_symbols_imag[HAIP_PACKET_LENGTH];

segment ("sdram0") static fract32 frame_symbols_real_upsample[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") static fract32 frame_symbols_imag_upsample[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") static fract32 modulated_synchronization[HAIP_TX_PACKET_LENGTH];

segment ("sdram0") static fract32 filtered_real_symbols[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") static fract32 filtered_imag_symbols[HAIP_TX_PACKET_LENGTH];
segment ("sdram0") static fract32 modulated_signal[HAIP_TX_PACKET_LENGTH];

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
segment ("sdram0") static fract32 raw_samples_r[10*HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 raw_samples_i[10*HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 subsample_r[10*HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 subsample_i[10*HAIP_OVERSAMPLING_FACTOR];
segment ("sdram0") static fract32 filtered_samples_r[10*HAIP_OVERSAMPLING_FACTOR+50];
segment ("sdram0") static fract32 filtered_samples_i[10*HAIP_OVERSAMPLING_FACTOR+50];
segment ("sdram0") static unsigned char* ppp[10];

fract32* modulate_frame(unsigned char* frame_buffer, int frame_length) {
	int frame_symbols = frame_length * HAIP_SYMBOLS_PER_BYTE;

	//haip_hamming_7_4_ext_code(frame_buffer,frame_code, frame_symbols);
	//addPreamble();
	mapper(frame_buffer, frame_symbols);
	upsample(frame_symbols);

	//filter();
	oscilate();
	get_quadrature_inphase_test(modulated_signal,frame_symbols,raw_samples_r,raw_samples_i);
	//filter_sqrcosine(raw_samples_r, raw_samples_i, 10*8, filtered_samples_r, filtered_samples_i);
	subsample_test(raw_samples_r, raw_samples_i, 0,
		HAIP_OVERSAMPLING_FACTOR, frame_symbols*8, subsample_r, subsample_i);
	demap_16QAM_test(subsample_r, subsample_i, frame_symbols, 0, ppp, frame_buffer,frame_symbols_real,frame_symbols_imag);

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

	float re[20];
	float im[20];

	for (i = 0; i < frame_length; i++) {
		if (i % 2 != 0)
			numDecimal = frame_buffer[i / 2] & 0xF;
		else
			numDecimal = ((frame_buffer[i / 2] & 0xF0) >> 4);

		frame_symbols_real[i] = haip_const[numDecimal][0];
		frame_symbols_imag[i] = haip_const[numDecimal][1];

		re[i] = haip_ideal_const[numDecimal][0];
		im[i] = haip_ideal_const[numDecimal][1];

	}
}

void upsample(int frame_length) {
	int i = 0;
	int oversampled_length = (frame_length + HAIP_PREAMBLE_SYMBOLS)
			* HAIP_OVERSAMPLING_FACTOR;
	fract32 re[HAIP_TX_PACKET_LENGTH];
	fract32 im[HAIP_TX_PACKET_LENGTH];

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
			re[i] = frame_symbols_real[i / HAIP_OVERSAMPLING_FACTOR];
			im[i] = frame_symbols_imag[i / HAIP_OVERSAMPLING_FACTOR];
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
	fract32 re[HAIP_TX_PACKET_LENGTH];
	fract32 im[HAIP_TX_PACKET_LENGTH];

	for (i = 0; i < 10*8; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	//Filters the signal
	fir_fr32(frame_symbols_real_upsample, filtered_real_symbols,
	10*8+HAIP_SRCOS_COEFF_NUM, &state_real);
	fir_fr32(frame_symbols_imag_upsample, filtered_imag_symbols,
			10*8+HAIP_SRCOS_COEFF_NUM, &state_imag);

}
void oscilate() {
	int i = 0;
	float real = 0;
	float imaginary = 0;

	for (i = 0; i < HAIP_TX_PACKET_LENGTH; ++i) {
		real = frame_symbols_real_upsample[i]
				* cos_modulator_6KHz[i % 8];
		imaginary = frame_symbols_imag_upsample[i]
				* sin_modulator_6KHz[i % 8];
		modulated_signal[i] = (real - imaginary) * SQRT_2;
	}
}

void demap_16QAM_test(fract32* analog_samples_r, fract32* analog_samples_i,
		int len, double phase_off, unsigned char* data, unsigned char* in, fract32* test_r, fract32* test_i) {
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

	float mindist, dist;
	unsigned char ddd[len];
	int a=0, b;
	for (i = 0; i < len / HAIP_SYMBOLS_PER_BYTE; ++i) {
			data[i] = 0;
		}
	ddd[0] = 0;
	for (i=0; i < len; i++) {
		mindist = 200; //Big number
		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			re_dist = fr32_to_float(haip_const[j][0])
					- fr32_to_float(analog_samples_r[i]);
			im_dist = fr32_to_float(haip_const[j][1])
					- fr32_to_float(analog_samples_i[i]);
			dist = sqrt(re_dist * re_dist + im_dist * im_dist);
			if (dist < mindist) {
				mindist = dist;
				s = j;
			}
		}
		if(i>3){
			a = data[i/2];
		}
		if (i % 2 == 0) {
			mychar = mychar | (s << 4);
		} else {
			mychar = mychar | s;
			data[i / 2] = mychar;
			mychar = 0;
		}
	}
	for (i = 0; i < len/2; ++i) {
		r1 = analog_samples_r[i];
		r2 = test_r[i];
		i1 = analog_samples_i[i];
		i2 = test_i[i];
		div_r = fr32_to_float(r1)/fr32_to_float(r2);
		div_i = fr32_to_float(i1)/fr32_to_float(i2);
		a = data[i];
		b = in[i];
	}
}

void get_quadrature_inphase_test(fract32* analog_data, int buffer_len,
		fract32* raw_samples_r, fract32* raw_samples_i) {
	for (int i = 0; i < buffer_len; i++) {
		raw_samples_r[i] =
				(analog_data[i] * cos_modulator_6KHz[i % 8]) * SQRT_2;
		raw_samples_i[i] = (-analog_data[i] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
	}
}

void subsample_test(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,
		fract32* raw_samples_r, fract32* raw_samples_i) {
	int indx = 0;
	fract32 tmp_r = 0;
	fract32 tmp_i = 0;
	while (len - indx * t >= t) {
		raw_samples_r[indx] = filtered_r[indx * t + n];
		raw_samples_i[indx] = filtered_i[indx * t + n];
		tmp_r = filtered_r[indx * t + n];
		tmp_i = filtered_i[indx * t + n];
		indx++;
	}
}

void filter_sqrcosine_test(fract32* nraw_samples_r, fract32* nraw_samples_i, int len,
		fract32* filtered_samples_r, fract32* filtered_samples_i) {

	for (int i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		ndelay_real[i] = 0;
		ndelay_imag[i] = 0;
	}

	fir_init(nstate_real, haip_srcos_fir_fil_coeffs_fr32, ndelay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, ndelay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(nraw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM * 2,
			&nstate_real);
	fir_fr32(nraw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM * 2,
			&nstate_imag);

}


