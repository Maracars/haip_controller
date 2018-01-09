/*
 * haip_demodulator.c
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#include "haip_demodulator.h"

//VARIABLES
segment ("sdram0") static fract32 analog_samples_r[HAIP_DEMOD_MAX_SAMPLES
		+ 49 * 2];
segment ("sdram0") static fract32 analog_samples_i[HAIP_DEMOD_MAX_SAMPLES
		+ 49 * 2];
segment ("sdram0") static fract32 filtered_samples_i[HAIP_DEMOD_MAX_FILTERED_SAMPLES
		+ 49 * 2];
segment ("sdram0") static fract32 filtered_samples_r[HAIP_DEMOD_MAX_FILTERED_SAMPLES
		+ 49 * 2];
segment ("sdram0") static complex_fract32 subsamples[HAIP_DEMOD_MAX_SUBSAMPLES
		* 2];
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
		fract32* filtered_samples_r, fract32* filtered_samples_i,
		bool dem_header);
/* Gets the synchronization parameters from the preamble */
haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r,
		fract32* filtered_analog_complex_data_i);
/* Subsamples the analog data taking the n index of every t samples */
void subsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,
		complex_fract32* subsampled_data, int delay);
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
	unsigned char start, final;

	get_quadrature_inphase(analog_data,
	HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM * 2, analog_samples_r,
			analog_samples_i); //demodulate
	filter_sqrcosine(analog_samples_r, analog_samples_i,
	HAIP_DEMOD_HEADER_SAMPLES, filtered_samples_r, filtered_samples_i, true); //filter
	sync = syncronize_with_preamble(filtered_samples_r, filtered_samples_i); //sync

	subsample(filtered_samples_r, filtered_samples_i, 0,
	HAIP_OVERSAMPLING_FACTOR, HAIP_DEMOD_HEADER_SAMPLES, subsamples,
	HAIP_SRCOS_FILTER_DELAY + HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR); //subsample
	demap_16QAM(subsamples, HAIP_DEMOD_HEADER_SUBSAMPLES, 0, demapped_data); //demap
	for (int tm = 0; tm < 10; ++tm) {
		start = demapped_data[tm];
	}
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data,
	HAIP_DEMOD_HEADER_CODED_LEN); //decode
	for (int i = 0; i < 10; i++) {
		final = output_digital_data[i];
	}
	return sync;
}

void haip_demodulate_payload(fract32* analog_data, int buffer_len,
		haip_sync_t sync, unsigned char* output_digital_data) {
	int data_symbol_length = buffer_len * HAIP_CODING_RATE * 2
			* HAIP_OVERSAMPLING_FACTOR;
	unsigned char final;

	get_quadrature_inphase(analog_data,
			data_symbol_length + HAIP_SRCOS_COEFF_NUM
					+ (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR),
			analog_samples_r, analog_samples_i); //demodulate
	filter_sqrcosine(analog_samples_r, analog_samples_i,
			data_symbol_length
					+ (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR),
			filtered_samples_r, filtered_samples_i, false); //filter
	subsample(filtered_samples_r, filtered_samples_i, 0,
	HAIP_OVERSAMPLING_FACTOR, data_symbol_length, subsamples,
	HAIP_SRCOS_FILTER_DELAY + HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR); //subsample

	demap_16QAM(subsamples, data_symbol_length / HAIP_OVERSAMPLING_FACTOR, 0,
			demapped_data); //demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data,
			buffer_len * HAIP_CODING_RATE); //decode

	for (int i = 0; i < buffer_len; i++) {
		final = output_digital_data[i];
	}

}

void get_quadrature_inphase(fract32* analog_data, int buffer_len,
		fract32* raw_samples_r, fract32* raw_samples_i) {
	float re[(10 * 2 + 8) * 8 + 50];
	float im[(10 * 2 + 8) * 8 + 50];
	float mod[(10 * 2 + 8) * 8 + 50];
	for (int i = 0; i < buffer_len; i++) {
		mod[i] = fr32_to_float(analog_data[i]);
		raw_samples_r[i] =
				(analog_data[i] * cos_modulator_6KHz[i % 8]) * SQRT_2;
		raw_samples_i[i] = (-analog_data[i] * sin_modulator_6KHz[i % 8])
				* SQRT_2;
		re[i] = fr32_to_float(raw_samples_r[i]);
		im[i] = fr32_to_float(raw_samples_i[i]);
	}
}

void filter_sqrcosine(fract32* raw_samples_r, fract32* raw_samples_i, int len,
		fract32* filtered_samples_r, fract32* filtered_samples_i,
		bool dem_header) {

	fir_state_fr32 state_real;
	fir_state_fr32 state_imag;
	int i;
	float re[(10 * 2 + 8) * 8 + 100];
	float im[(10 * 2 + 8) * 8 + 100];
	for (i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	/* Paketean tamañue pasau eta hori aldatu 160 gaitik
	 * If bet ipini aurretik jakiteko headerra edo payload demoduletan gabizen*/
	if (!dem_header) {
		for (i = len; i < (len + (HAIP_SRCOS_COEFF_NUM * 2)); i++) {
			raw_samples_r[i] = 0;
			raw_samples_i[i] = 0;
			if (i > 300) {
				printf("a");
			}
		}
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real,
			HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag,
			HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(raw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM * 2,
			&state_real);
	fir_fr32(raw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM * 2,
			&state_imag);

	for (int i = 0; i < len + HAIP_SRCOS_COEFF_NUM * 2; i++) {
		re[i] = fr32_to_float(filtered_samples_r[i]);
		im[i] = fr32_to_float(filtered_samples_i[i]);
	}

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
		complex_fract32* subsampled_data, int delay) {
	int indx = 0;
	float re[20];
	float im[20];
	while (len + delay - indx * t >= t) {
		subsampled_data[indx].re = filtered_r[indx * t + n + delay];
		subsampled_data[indx].im = filtered_i[indx * t + n + delay];
		indx++;
	}
	for (int i = 0; i < 20; i++) {
		re[i] = fr32_to_float(subsampled_data[i].re);
		im[i] = fr32_to_float(subsampled_data[i].im);
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
