/*
 * haip_demodulator.c
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#include "haip_demodulator.h"

//VARIABLES
segment ("sdram0") static fract32 raw_samples_i[HAIP_DEMOD_MAX_SAMPLES];
segment ("sdram0") static fract32 raw_samples_r[HAIP_DEMOD_MAX_SAMPLES];
segment ("sdram0") static fract32 filtered_samples_i[HAIP_DEMOD_MAX_FILTERED_SAMPLES];
segment ("sdram0") static fract32 filtered_samples_r[HAIP_DEMOD_MAX_FILTERED_SAMPLES];
segment ("sdram0") static complex_fract32 subsamples[HAIP_DEMOD_MAX_SUBSAMPLES];
segment ("sdram0") static unsigned char demapped_data[HAIP_DEMOD_MAX_CODED_DATA];
segment ("sdram0") static fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

//Need to be normalized
segment ("sdram0") static float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") static float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };
segment ("sdram0") static float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071,
		0, -0.7071, -1, -0.7071 };
segment ("sdram0") static float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071,
		-1, -0.7071, 0, 0.7071 };

//LOCAL FUNCTIONS
/* Separates the quadrature and in-phase values in the raw analog data received */
void get_quadrature_inphase(fract32* analog_data, int buffer_len,
		fract32* raw_samples_r, fract32* raw_samples_i);
/* Applies the squared raised cosine filter to the analog data ( see haip_srcos_filter_fr32.h) */
void filter_sqrcosine(fract32* raw_samples_i, fract32* raw_samples_r, int len,
		fract32* filtered_samples_r, fract32* filtered_samples_i);
/* Gets the synchronization parameters from the preamble */
haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r,
		fract32* filtered_analog_complex_data_i);
/* Subsamples the analog data taking the n index of every t samples */
void subsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,
		complex_fract32* subsampled_data);
/* Demaps the analog data according to the constellation adjusting phase difference */
void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off,
		unsigned char* data);
/* calculate the distance between symbols */
double calc_dist(fract32* p1, complex_fract32 p2);

haip_sync_t haip_demodulate_head(fract32* analog_data,
		unsigned char* output_digital_data) {
	haip_sync_t sync;
	fract32* samples_w0_pream_r;
	fract32* samples_w0_pream_i;

	get_quadrature_inphase(analog_data, 10*8, raw_samples_r, raw_samples_i); //demodulate
	//filter_sqrcosine(raw_samples_r, raw_samples_i, HAIP_DEMOD_HEADER_SAMPLES, filtered_samples_r, filtered_samples_i); //filter
	//sync = syncronize_with_preamble(raw_samples_r, raw_samples_i); //sync

	//samples_w0_pream_r = &raw_samples_r[HAIP_PREAMBLE_SYMBOLS/ HAIP_SYMBOLS_PER_BYTE]; //Ignore preamble from now on
	//samples_w0_pream_i = &raw_samples_i[HAIP_PREAMBLE_SYMBOLS/ HAIP_SYMBOLS_PER_BYTE]; //Ignore preamble from now on

	subsample(raw_samples_r, raw_samples_i, 0,
	HAIP_OVERSAMPLING_FACTOR, 10*8, subsamples); //subsample
	demap_16QAM(subsamples, 10, 0, output_digital_data); //demap
	//haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, HAIP_DEMOD_HEADER_CODED_LEN); //decode

	return sync;
}

void haip_demodulate_payload(fract32* analog_data, int buffer_len,
		haip_sync_t sync, unsigned char* output_digital_data) {

	get_quadrature_inphase(analog_data, buffer_len, raw_samples_r,
			raw_samples_i); //demodulate
	//filter_sqrcosine(raw_samples_r, raw_samples_i, buffer_len, filtered_samples_r, filtered_samples_i); //filter
	subsample(raw_samples_r, raw_samples_i, sync.sample,
	HAIP_OVERSAMPLING_FACTOR, buffer_len, subsamples); //subsample

	int subsample_num = buffer_len / HAIP_OVERSAMPLING_FACTOR;

	demap_16QAM(subsamples, subsample_num, sync.phase_off, output_digital_data); //demap
	//haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, subsample_num / (8 / HAIP_BITS_PER_SYMBOL)); //decode

}

void get_quadrature_inphase(fract32* analog_data, int buffer_len,
		fract32* raw_samples_r, fract32* raw_samples_i) {
	for (int i = 0; i < buffer_len; i++) {
		raw_samples_r[i] =
				(analog_data[i] * cos_modulator_6KHz[i % 8]) * SQRT_2;
		raw_samples_i[i] = (-analog_data[i] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
	}
}

void filter_sqrcosine(fract32* raw_samples_r, fract32* raw_samples_i, int len,
		fract32* filtered_samples_r, fract32* filtered_samples_i) {

	fir_state_fr32 state_real;
	fir_state_fr32 state_imag;

	for (int i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(raw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM * 2,
			&state_real);
	fir_fr32(raw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM * 2,
			&state_imag);

}

haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r,
		fract32* filtered_analog_complex_data_i) {
	haip_sync_t sync;
	//Placeholder by now loco
	sync.phase_off = 0;
	sync.sample = 0;
	return sync;
}

void subsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,
		complex_fract32* subsampled_data) {
	int indx = 0;
	while (len - indx * t >= t) {
		subsampled_data[indx].re = filtered_r[indx * t + n];
		subsampled_data[indx].im = filtered_i[indx * t + n];
		indx++;
	}
}

void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off,
		unsigned char* data) {
	int i = 0;
	uint8_t j = 0, s;
	float mindist, dist;
	char mychar = 0;
	for (i = 0; i < len / HAIP_SYMBOLS_PER_BYTE; ++i) {
		data[i] = 0;
	}
	for (; i < len; i++) {
		mindist = 200; //Big number
		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			dist = calc_dist(haip_const[j], analog_samples[i]);
			if (dist < mindist) {
				mindist = dist;
				s = j;
			}
		}
		//data[i / 2] = data[i / 2] | (s << (i % 2 ? 4 : 0));
		if (i % 2 == 0) {
			mychar = mychar | (s << 4);
		} else {
			mychar = mychar | s;
			data[i / 2] = mychar;
			mychar = 0;
		}
	}
}

double calc_dist(fract32* p1, complex_fract32 p2) {
	float re_dist = fr32_to_float(p1[0]) - fr32_to_float(p2.re);
	float im_dist = fr32_to_float(p1[1]) - fr32_to_float(p2.im);

	double a = sqrt(re_dist * re_dist + im_dist * im_dist);
	return a;
}
