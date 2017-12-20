/*
 * haip_demodulator.c
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#include "haip_demodulator.h"

//VARIABLES
segment ("sdram0") complex_fract32 raw_samples[HAIP_DEMOD_MAX_SAMPLES];
segment ("sdram0") complex_fract32 filtered_samples[HAIP_DEMOD_MAX_FILTERED_SAMPLES];
segment ("sdram0") complex_fract32 subsamples[HAIP_DEMOD_MAX_SUBSAMPLES];
segment ("sdram0") complex_fract32 synced_data[HAIP_DEMOD_MAX_SUBSAMPLES];
segment ("sdram0") unsigned char demapped_data[HAIP_DEMOD_MAX_CODED_DATA];


//LOCAL FUNCTIONS
/* Separates the quadrature and in-phase values in the raw analog data received */
void get_quadrature_inphase(fract32* analog_data, int buffer_len, complex_fract32* analog_complex_data);
/* Applies the squared raised cosine filter to the analog data ( see haip_srcos_filter_fr32.h) */
void filter_sqrcosine(complex_fract32* analog_complex_data, int len, complex_fract32* filtered_analog_complex_data);
/* Gets the synchronization parameters from the preamble */
haip_sync_t syncronize_with_preamble(complex_fract32* filtered_analog_complex_data);
/* Subsamples the analog data taking the n index of every t samples */
void subsample(complex_fract32* filtered, int n, int t, int len, complex_fract32* subsampled_data);
/* Demaps the analog data according to the constellation adjusting phase difference */
void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data);
/* calculate the distance between symbols */
double calc_dist(fract32* p1, complex_fract32 p2);


haip_sync_t haip_demodulate_head(fract32* analog_data, unsigned char* output_digital_data) {
	haip_sync_t sync;
	complex_fract32* samples_w0_pream;
	get_quadrature_inphase(analog_data, HAIP_DEMOD_HEADER_SAMPLES, raw_samples); //demodulate
	filter_sqrcosine(raw_samples, HAIP_DEMOD_HEADER_SAMPLES, filtered_samples); //filter
	sync = syncronize_with_preamble(filtered_samples);//sync
	samples_w0_pream = &filtered_samples[HAIP_PREAMBLE_SYMBOLS / HAIP_SYMBOLS_PER_BYTE]; //Ignore preamble from now on
	subsample(samples_w0_pream, sync.sample, HAIP_OVERSAMPLING_FACTOR, HAIP_DEMOD_HEADER_NO_PREAMBLE, subsamples);//subsample
	demap_16QAM(subsamples, HAIP_DEMOD_HEADER_SUBSAMPLES, sync.phase_off, demapped_data);//demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, HAIP_DEMOD_HEADER_CODED_LEN);//decode
	return sync;
}

void haip_demodulate_payload(fract32* analog_data, int buffer_len, haip_sync_t sync, unsigned char* output_digital_data) {
	get_quadrature_inphase(analog_data, buffer_len, raw_samples); //demodulate
	filter_sqrcosine(raw_samples, buffer_len, filtered_samples); //filter
	subsample(filtered_samples, sync.sample, HAIP_OVERSAMPLING_FACTOR, buffer_len, subsamples);//subsample
	int subsample_num = buffer_len / HAIP_OVERSAMPLING_FACTOR;
	demap_16QAM(subsamples, subsample_num, sync.phase_off, demapped_data);//demap
	haip_hamming_7_4_ext_decode(demapped_data, output_digital_data, subsample_num / (8 / HAIP_BITS_PER_SYMBOL));//decode
}

void get_quadrature_inphase(fract32* analog_data, int buffer_len, complex_fract32* analog_complex_data) {

}

void filter_sqrcosine(complex_fract32* analog_complex_data, int len, complex_fract32* filtered_analog_complex_data){

}

haip_sync_t syncronize_with_preamble(complex_fract32* filtered_analog_complex_data){
	haip_sync_t sync;
	sync.phase_off = 0;
	sync.sample = 0;
	return sync;
}

void subsample(complex_fract32* filtered, int n, int t, int len, complex_fract32* subsampled_data){
	int indx = 0;
	while(len - indx*t > t){
		subsampled_data[indx] = filtered[indx*t + n];
		indx++;
	}
}

void demap_16QAM(complex_fract32* analog_samples, int len, double phase_off, unsigned char* data){
	int i = 0;
	uint8_t j = 0, s;
	memcpy(data, 0, len/HAIP_SYMBOLS_PER_BYTE);
	fract32 mindist, dist;
	for(;i < len; i++){
		mindist = 200; //Big number
		for(j = 0; j < HAIP_SYMBOLS_16QAM; j++){
			dist = calc_dist(haip_const[j], analog_samples[i]);
			if(dist < mindist){
				mindist = dist;
				s = j;
			}
		}
		data[i/2] = data[i/2] | (s << (i%2 ? 4 : 0));
	}
}

double calc_dist(fract32* p1, complex_fract32 p2){
	fract32 re_dist = p1[0] - p2.re;
	fract32 im_dist = p1[1] - p2.im;
	return sqrt(re_dist*re_dist + im_dist*im_dist);
}
