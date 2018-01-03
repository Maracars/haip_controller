/*
 * haip_demodulator.c
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#include "haip_demodulator.h"

#define CORRELATION_SAMPLE_LENGTH 	(HAIP_SRCOS_FILTER_DELAY*2/HAIP_OVERSAMPLING_FACTOR+HAIP_PREAMBLE_SYMBOLS)
#define PERFECT_CORRELATION			16

//VARIABLES
segment ("sdram0") static fract32 raw_samples_i[HAIP_DEMOD_MAX_SAMPLES];
segment ("sdram0") static fract32 raw_samples_r[HAIP_DEMOD_MAX_SAMPLES];
segment ("sdram0") static fract32 filtered_samples_i[HAIP_DEMOD_MAX_FILTERED_SAMPLES];
segment ("sdram0") static fract32 filtered_samples_r[HAIP_DEMOD_MAX_FILTERED_SAMPLES];
segment ("sdram0") static complex_fract32 downsamples[HAIP_DEMOD_MAX_SUBSAMPLES];
segment ("sdram0") static unsigned char demapped_data[HAIP_DEMOD_MAX_CODED_DATA];
segment ("sdram0") static fract32 delay_real[HAIP_SRCOS_COEFF_NUM];
segment ("sdram0") static fract32 delay_imag[HAIP_SRCOS_COEFF_NUM];

segment ("sdram0") complex_fract32 filtered_complex_signal[HAIP_OVERSAMPLING_FACTOR][CORRELATION_SAMPLE_LENGTH];
segment ("sdram0") complex_fract32 correlation[HAIP_OVERSAMPLING_FACTOR][CORRELATION_SAMPLE_LENGTH+HAIP_PREAMBLE_SYMBOLS/2];

//Need to be normalized
segment ("sdram0") static float preamble_real[] = { -1, 1, -1, -1, 1, 1, -1, 1 };
segment ("sdram0") static float preamble_imag[] = { -1, 1, 1, -1, 1, -1, 1, 1 };
segment ("sdram0") complex_fract32 inv_preamble[HAIP_PREAMBLE_SYMBOLS];
segment ("sdram0") complex_fract32 delay_fir[HAIP_PREAMBLE_SYMBOLS];
segment ("sdram0") static float sin_modulator_6KHz[] = { 0, 0.7071, 1, 0.7071, 0, -0.7071, -1, -0.7071 };
segment ("sdram0") static float cos_modulator_6KHz[] = { 1, 0.7071, 0, -0.7071, -1, -0.7071, 0, 0.7071 };
segment ("sdram0") cfir_state_fr32 cfir_state;
segment ("sdram0") fract32 adj_const[16][2];
//LOCAL FUNCTIONS
/* Separates the quadrature and in-phase values in the raw analog data received */
void get_quadrature_inphase(fract32* analog_data, int buffer_len, fract32* raw_samples_r, fract32* raw_samples_i);
/* Applies the squared raised cosine filter to the analog data ( see haip_srcos_filter_fr32.h) */
void filter_sqrcosine(fract32* raw_samples_i, fract32* raw_samples_r, int len, fract32* filtered_samples_r, fract32* filtered_samples_i);
/* Gets the synchronization parameters from the preamble */
haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r, fract32* filtered_analog_complex_data_i);
/* Subsamples the analog data taking the n index of every t samples */
void downsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len, float att, complex_fract32* subsampled_data);
/* Demaps the analog data according to the constellation adjusting phase difference */
void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data);
/* calculate the distance between symbols */
double calc_dist(fract32* p1, complex_fract32 p2);

void haip_init_demodulator(){
	complex_fract32 aux;

	for(int i = 0; i < HAIP_PREAMBLE_SYMBOLS; i++){
		aux.re = float_to_fr32(preamble_real[HAIP_PREAMBLE_SYMBOLS-i-1]/32);
		aux.im = float_to_fr32(preamble_imag[HAIP_PREAMBLE_SYMBOLS-i-1]/32);
		inv_preamble[i] = conj_fr32(aux);
	}
}

haip_sync_t haip_demodulate_head(fract32* analog_data, unsigned char* output_digital_data) {
	haip_sync_t sync;
	fract32* samples_w0_pream_r;
	fract32* samples_w0_pream_i;

	get_quadrature_inphase(analog_data, HAIP_DEMOD_HEADER_SAMPLES, raw_samples_r, raw_samples_i); //demodulate
	filter_sqrcosine(raw_samples_r, raw_samples_i, HAIP_DEMOD_HEADER_SAMPLES, filtered_samples_r, filtered_samples_i); //filter
	sync = syncronize_with_preamble(filtered_samples_r, filtered_samples_i); //sync

	samples_w0_pream_r = &filtered_samples_r[sync.start_indx * HAIP_OVERSAMPLING_FACTOR]; //Ignore preamble from now on
	samples_w0_pream_i = &filtered_samples_i[sync.start_indx * HAIP_OVERSAMPLING_FACTOR]; //Ignore preamble from now on

	downsample(samples_w0_pream_r, samples_w0_pream_i, sync.sample, HAIP_OVERSAMPLING_FACTOR, HAIP_DEMOD_HEADER_NO_PREAMBLE, sync.att, downsamples); //downsample
	demap_16QAM(downsamples, HAIP_DEMOD_HEADER_SUBSAMPLES, sync.phase_off, demapped_data); //demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, HAIP_DEMOD_HEADER_CODED_LEN); //decode

	return sync;
}

void haip_demodulate_payload(fract32* analog_data, int buffer_len, haip_sync_t sync, unsigned char* output_digital_data) {

	get_quadrature_inphase(analog_data, buffer_len, raw_samples_r, raw_samples_i); //demodulate
	filter_sqrcosine(raw_samples_r, raw_samples_i, buffer_len, filtered_samples_r, filtered_samples_i); //filter
	downsample(filtered_samples_r, filtered_samples_i, sync.sample, HAIP_OVERSAMPLING_FACTOR, buffer_len, sync.att, downsamples); //subsample

	int subsample_num = buffer_len / HAIP_OVERSAMPLING_FACTOR;

	demap_16QAM(downsamples, subsample_num, sync.phase_off, demapped_data); //demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, subsample_num / (8 / HAIP_BITS_PER_SYMBOL)); //decode

}

void get_quadrature_inphase(fract32* analog_data, int buffer_len, fract32* raw_samples_r, fract32* raw_samples_i) {
	for (int i = 0; i < buffer_len; i++) {
		raw_samples_r[i] = (analog_data[i] * cos_modulator_6KHz[i % HAIP_OVERSAMPLING_FACTOR]) * SQRT_2;
		raw_samples_i[i] = (-analog_data[i] * sin_modulator_6KHz[i % HAIP_OVERSAMPLING_FACTOR]) * SQRT_2;
	}
}

void filter_sqrcosine(fract32* raw_samples_r, fract32* raw_samples_i, int len, fract32* filtered_samples_r, fract32* filtered_samples_i) {

	fir_state_fr32 state_real;
	fir_state_fr32 state_imag;

	for (int i = 0; i < HAIP_SRCOS_COEFF_NUM; i++) {
		delay_real[i] = 0;
		delay_imag[i] = 0;
	}

	fir_init(state_real, haip_srcos_fir_fil_coeffs_fr32, delay_real, HAIP_SRCOS_COEFF_NUM, 0);
	fir_init(state_imag, haip_srcos_fir_fil_coeffs_fr32, delay_imag, HAIP_SRCOS_COEFF_NUM, 0);

	fir_fr32(raw_samples_r, filtered_samples_r, len + HAIP_SRCOS_COEFF_NUM, &state_real);
	fir_fr32(raw_samples_i, filtered_samples_i, len + HAIP_SRCOS_COEFF_NUM, &state_imag);

}

haip_sync_t syncronize_with_preamble(fract32* filtered_analog_complex_data_r, fract32* filtered_analog_complex_data_i) {
	haip_sync_t sync;
	int corr_number = 0;
	int j = 0, i = 0;
	double c_value = 0.0;
	double c_actual_value = 0.0;
	double c_max_value = 0.0;

	cfir_init(cfir_state,inv_preamble,delay_fir,HAIP_PREAMBLE_SYMBOLS);
	for(corr_number = 0; corr_number < HAIP_OVERSAMPLING_FACTOR; corr_number++){

			for(j = 0; j < CORRELATION_SAMPLE_LENGTH; j++){
				filtered_complex_signal[corr_number][j].im = filtered_analog_complex_data_i[corr_number+j*HAIP_OVERSAMPLING_FACTOR];
				filtered_complex_signal[corr_number][j].re = filtered_analog_complex_data_r[corr_number+j*HAIP_OVERSAMPLING_FACTOR];
			}

			cfir_fr32(filtered_complex_signal[corr_number],correlation[corr_number],CORRELATION_SAMPLE_LENGTH,&cfir_state);

			i = 0;
			c_value = 0.0;

			for(j = 0; j < CORRELATION_SAMPLE_LENGTH; j++){
				c_actual_value = cabs_fr32(correlation[corr_number][j]);

				if(c_value < c_actual_value){
					c_value = c_actual_value;
					i = j;
				}
			}

			if(c_max_value < c_value){
				c_max_value = c_value;
				sync.sample = corr_number;
				sync.start_indx = i-HAIP_PREAMBLE_SYMBOLS+1;
				sync.phase_off = atan2(fr32_to_float(correlation[corr_number][i].im), fr32_to_float(correlation[corr_number][i].re));
			}
		}

	sync.att = PERFECT_CORRELATION*0.9/(fr32_to_float(c_max_value)*32/0.75);

	return sync;
}

void downsample(fract32* filtered_r, fract32* filtered_i, int n, int t, int len, float att, complex_fract32* subsampled_data) {
	int indx = 0;
	while (len - indx * t > t) {
		subsampled_data[indx].re = float_to_fr32(fr32_to_float(filtered_r[indx * t + n]) * att);
		subsampled_data[indx].im = float_to_fr32(fr32_to_float(filtered_i[indx * t + n]) * att);
		indx++;
	}
}

void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data) {
	int i = 0;
	uint8_t j = 0, s;
	memcpy(data, 0, len / HAIP_SYMBOLS_PER_BYTE);

	for(i = 0; i < 16; i++){
		adj_const[i][0] = float_to_fr32(cos(phase_off) * fr32_to_float(haip_const[i][0]) - sin(phase_off) * fr32_to_float(haip_const[i][1]));
		adj_const[i][1] = float_to_fr32(sin(phase_off) * fr32_to_float(haip_const[i][0]) + cos(phase_off) * fr32_to_float(haip_const[i][1]));
	}

	fract32 mindist, dist;
	for (i = 0; i < len; i++) {
		mindist = 200; //Big number
		for (j = 0; j < HAIP_SYMBOLS_16QAM; j++) {
			dist = calc_dist(adj_const[j], analog_samples[i]);
			if (dist < mindist) {
				mindist = dist;
				s = j;
			}
		}
		data[i / 2] = data[i / 2] | (s << (i % 2 ? 4 : 0));
	}
}

double calc_dist(fract32* p1, complex_fract32 p2) {
	fract32 re_dist = p1[0] - p2.re;
	fract32 im_dist = p1[1] - p2.im;
	return sqrt(re_dist * re_dist + im_dist * im_dist);
}
