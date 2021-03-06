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

int modulate_frame(unsigned char* frame_buffer, int frame_length, fract32* modulated_signal);

void addPreamble(void);
void mapper(unsigned char* frame_buffer, int frame_symbols);
void upsample(int frame_symbols);
void filter(void);
void oscilate(fract32* modulated_signal);
void addHeader(void);
void demap_16QAM_test(int len, double phase_off, unsigned char* frame_buffer);
void get_quadrature_inphase_test(int buffer_len, fract32* modulated_signal);
void subsample_test(int n, int t, int len);

void filter_sqrcosine_test(int len);
void symbol2char();
void demapper();
void init_ranges();
void dowmsample();
void filter_demodulator();
void demodulate();


#endif /* HAIPMODULATOR_H_ */
