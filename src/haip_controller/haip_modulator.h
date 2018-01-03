/*
 * HaipModulator.h
 *
 *  Created on: 11/12/2017
 *      Author: joseba
 */

#ifndef HAIPMODULATOR_H_
#define HAIPMODULATOR_H_

#include <fract_typedef.h>
#include "haip_srcos_filter_fr32.h"
#include "haip_commons.h"

#define HAIP_PACKET_LENGTH 				(HAIP_FRAME_MAX_LEN * HAIP_SYMBOLS_PER_BYTE + HAIP_PREAMBLE_SYMBOLS)
#define HAIP_TX_PACKET_LENGTH 			HAIP_PACKET_LENGTH * HAIP_OVERSAMPLING_FACTOR + HAIP_SRCOS_COEFF_NUM

fract32* modulate_frame(unsigned char* frame_buffer, int frame_length);

void addPreamble(void);
void mapper(unsigned char* frame_buffer, int frame_symbols);
void upsample(int frame_symbols);
void filter(void);
void oscilate(void);
void addHeader(void);
void demap_16QAM_test(fract32* analog_samples_r, fract32* analog_samples_i,
		int len, double phase_off, unsigned char* data, unsigned char* in, fract32* test_r, fract32* test_i);
void get_quadrature_inphase_test(fract32* analog_data, int buffer_len,
		fract32* raw_samples_r, fract32* raw_samples_i);
void subsample_test(fract32* filtered_r, fract32* filtered_i, int n, int t, int len,
		fract32* raw_samples_r, fract32* raw_samples_i);

void filter_sqrcosine_test(fract32* nraw_samples_r, fract32* nraw_samples_i, int len,
		fract32* filtered_samples_r, fract32* filtered_samples_i);


#endif /* HAIPMODULATOR_H_ */
