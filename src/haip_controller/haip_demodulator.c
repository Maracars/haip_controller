/*
 * haip_demodulator.c
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#include "haip_demodulator.h"

#define CORRELATION_SAMPLE_LENGTH 	(HAIP_PREAMBLE_SYMBOLS)
#define PERFECT_CORRELATION			16
#define DELAY 						(HAIP_SRCOS_FILTER_DELAY + HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR)

//VARIABLES
segment ("sdram0") static fract32 analog_samples_r[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static fract32 analog_samples_i[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static fract32 filtered_samples_i[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static fract32 filtered_samples_r[HAIP_FRAME_SAMPLES_W_COEFFS];
segment ("sdram0") static complex_fract32 subsamples[HAIP_FRAME_MAX_SYMBOLS + DELAY/8];
segment ("sdram0") static unsigned char demapped_data[HAIP_FRAME_MAX_SYMBOLS / HAIP_SYMBOLS_PER_BYTE];
segment ("sdram0") static fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") complex_fract32 filtered_complex_signal[HAIP_OVERSAMPLING_FACTOR][CORRELATION_SAMPLE_LENGTH];

//Resources
segment ("sdram0") static float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") static float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };
segment ("sdram0") static float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071, 0, -0.7071, -1, -0.7071 };
segment ("sdram0") static float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071, -1, -0.7071, 0, 0.7071 };

// For Sync
segment ("sdram0") complex_fract32 correlation[HAIP_OVERSAMPLING_FACTOR][CORRELATION_SAMPLE_LENGTH + HAIP_PREAMBLE_SYMBOLS / 2];
segment ("sdram0") cfir_state_fr32 cfir_state;
segment ("sdram0") fract32 adj_const[16][2]; //16 QAM constelation inverted so that fir acts as xcorr
segment ("sdram0") complex_fract32 inv_preamble[HAIP_PREAMBLE_SYMBOLS];
segment ("sdram0") complex_fract32 delay_fir[HAIP_PREAMBLE_SYMBOLS];

//LOCAL FUNCTIONS
/* Separates the quadrature and in-phase values in the raw analog data received */
void get_quadrature_inphase(fract32* analog_data, int buffer_len, fract32* raw_samples_r, fract32* raw_samples_i);
/* Applies the squared raised cosine filter to the analog data ( see haip_srcos_filter_fr32.h) */
void filter_sqrcosine(fract32* raw_samples_i, fract32* raw_samples_r, int len, fract32* filtered_samples_r, fract32* filtered_samples_i, bool dem_header);
/* Gets the synchronization parameters from the preamble */
haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r, fract32* filtered_analog_complex_data_i);
/* Subsamples the analog data taking the n index of every t samples */
void subsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len, complex_fract32* subsampled_data, int delay, double att);
/* Demaps the analog data according to the constellation adjusting phase difference */
void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data);
/* calculate the distance between symbols */
double calc_dist(fract32* p1, complex_fract32 p2);

void haip_init_demodulator() {
	complex_fract32 aux;

	for (int i = 0; i < HAIP_PREAMBLE_SYMBOLS; i++) {
		aux.re = float_to_fr32(preamble_real[HAIP_PREAMBLE_SYMBOLS - i - 1] / 32);
		aux.im = float_to_fr32(preamble_imag[HAIP_PREAMBLE_SYMBOLS - i - 1] / 32);
		inv_preamble[i] = conj_fr32(aux);
	}
}

haip_sync_t haip_demodulate_head(fract32* analog_data, unsigned char* output_digital_data) {
	haip_sync_t sync;

	//TODO: Se ha quitado el *2 de COEFF_NUM
	get_quadrature_inphase(analog_data,	HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM*2, analog_samples_r, analog_samples_i); //demodulate
	filter_sqrcosine(analog_samples_r, analog_samples_i, HAIP_DEMOD_HEADER_SAMPLES, filtered_samples_r, filtered_samples_i, true); //filter
	sync = syncronize_with_preamble(filtered_samples_r, filtered_samples_i); //sync
	subsample(filtered_samples_r, filtered_samples_i, sync.sample, HAIP_OVERSAMPLING_FACTOR, HAIP_DEMOD_HEADER_SAMPLES, subsamples, DELAY, sync.att); //subsample
	demap_16QAM(subsamples, HAIP_DEMOD_HEADER_SUBSAMPLES, sync.phase_off, demapped_data); //demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data,	HAIP_DEMOD_HEADER_CODED_LEN); //decode
	return sync;
}

void haip_demodulate_payload(fract32* analog_data, int buffer_len, haip_sync_t sync, unsigned char* output_digital_data) {
	int data_symbol_length = buffer_len * HAIP_CODING_RATE * HAIP_SYMBOLS_PER_BYTE * HAIP_OVERSAMPLING_FACTOR;

	get_quadrature_inphase(analog_data, data_symbol_length + HAIP_SRCOS_COEFF_NUM + (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR), analog_samples_r, analog_samples_i); //demodulate
	filter_sqrcosine(analog_samples_r, analog_samples_i, data_symbol_length + (HAIP_PREAMBLE_SYMBOLS * HAIP_OVERSAMPLING_FACTOR), filtered_samples_r, filtered_samples_i, false); //filter
	subsample(filtered_samples_r, filtered_samples_i, sync.sample, HAIP_OVERSAMPLING_FACTOR, data_symbol_length, subsamples, DELAY, sync.att); //subsample
	demap_16QAM(subsamples, data_symbol_length / HAIP_OVERSAMPLING_FACTOR, sync.phase_off, demapped_data); //demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, buffer_len * HAIP_CODING_RATE); //decode
}

void get_quadrature_inphase(fract32* analog_data, int buffer_len, fract32* raw_samples_r, fract32* raw_samples_i) {

	for (int i = 0; i < buffer_len; i++) {
		raw_samples_r[i] = (analog_data[i] * cos_modulator_6KHz[i % 8]) * SQRT_2;
		raw_samples_i[i] = (-analog_data[i] * sin_modulator_6KHz[i % 8]) * SQRT_2;
	}
}

void filter_sqrcosine(fract32* raw_samples_r, fract32* raw_samples_i, int len, fract32* filtered_samples_r, fract32* filtered_samples_i, bool dem_header) {

	fir_state_fr32 state_real;
	fir_state_fr32 state_imag;
	int i;

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
		}
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real, HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag, HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(raw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM * 2, &state_real);
	fir_fr32(raw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM * 2, &state_imag);
}

haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r, fract32* filtered_analog_complex_data_i) {
	haip_sync_t sync;
	int corr_number = 0;
	int j = 0, i = 0;
	double c_value = 0.0;
	double c_actual_value = 0.0;
	double coors[8];
	double c_max_value = 0.0;

	cfir_init(cfir_state, inv_preamble, delay_fir, HAIP_PREAMBLE_SYMBOLS);
	for (corr_number = 0; corr_number < HAIP_OVERSAMPLING_FACTOR; corr_number++) {

		for (j = 0; j < CORRELATION_SAMPLE_LENGTH; j++) {
			filtered_complex_signal[corr_number][j].im = filtered_analog_complex_data_i[HAIP_SRCOS_FILTER_DELAY + corr_number + j * HAIP_OVERSAMPLING_FACTOR];
			filtered_complex_signal[corr_number][j].re = filtered_analog_complex_data_r[HAIP_SRCOS_FILTER_DELAY + corr_number + j * HAIP_OVERSAMPLING_FACTOR];
		}

		cfir_fr32(filtered_complex_signal[corr_number], correlation[corr_number], CORRELATION_SAMPLE_LENGTH, &cfir_state);

		i = 0;
		c_value = 0.0;

		for (j = 0; j < CORRELATION_SAMPLE_LENGTH; j++) {
			c_actual_value = cabs_fr32(correlation[corr_number][j]);

			if (c_value < c_actual_value) {
				c_value = c_actual_value;
				coors[corr_number] = c_value;
				i = j;
			}
		}

		if (c_max_value <= c_value) {
			c_max_value = c_value;
			sync.sample = corr_number;
			sync.start_indx = 0;
			sync.phase_off = atan2(fr32_to_float(correlation[corr_number][i].im), fr32_to_float(correlation[corr_number][i].re));
		}
	}

	sync.att = PERFECT_CORRELATION * 0.9 / (fr32_to_float(c_max_value) * 32 / 0.75);

	return sync;
}

void subsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,	complex_fract32* subsampled_data, int delay, double att) {
	int indx = 0;
	while (len + delay - indx * t >= t) {
		subsampled_data[indx].re = filtered_r[indx * t + n + delay] * att;
		subsampled_data[indx].im = filtered_i[indx * t + n + delay] * att;
		indx++;
	}
}

void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data) {
	int i = 0;
	uint8_t j = 0, s;
	float mindist, dist;
	char mychar = 0;
	double co = cos(phase_off);
	double si = sin(phase_off);
	for(i = 0; i < 16; i++){
		adj_const[i][0] = float_to_fr32(co * fr32_to_float(haip_const[i][0]) - si * fr32_to_float(haip_const[i][1]));
		adj_const[i][1] = float_to_fr32(si * fr32_to_float(haip_const[i][0]) + co * fr32_to_float(haip_const[i][1]));
	}

	for (i = 0; i < len / HAIP_SYMBOLS_PER_BYTE; ++i) {
		data[i] = 0;
	}
	for (; i < len; i++) {
		mindist = 200; //Big number
		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			dist = calc_dist(adj_const[j], analog_samples[i]);
			if (dist < mindist) {
				mindist = dist;
				s = j;
			}
		}
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
