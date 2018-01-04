/*
 * haip_demodulator.h
 *
 *  Created on: 19 Dec 2017
 *      Author: ander
 */

#ifndef HAIP_DEMODULATOR_H_
#define HAIP_DEMODULATOR_H_

//INCLUDES
#include "haip_commons.h"
#include "haip_hamming_7_4_ext_coding.h"
#include "haip_srcos_filter_fr32.h"
#include "haip_16QAM_mapping.h"

#include "fract_typedef.h"
#include "fract2float_conv.h"
#include "filter.h"
#include <math.h>

//DEFINES
#define HAIP_DEMOD_MAX_DATA 				(HAIP_FRAME_DATA_MAX_LEN + 1)
#define HAIP_DEMOD_MAX_CODED_DATA 			(HAIP_DEMOD_MAX_DATA * HAIP_CODING_RATE)
#define HAIP_DEMOD_MAX_SUBSAMPLES  			(HAIP_DEMOD_MAX_CODED_DATA * 8/HAIP_BITS_PER_SYMBOL)
#define HAIP_DEMOD_MAX_SAMPLES 				((HAIP_DEMOD_MAX_SUBSAMPLES + HAIP_PREAMBLE_SYMBOLS) * HAIP_OVERSAMPLING_FACTOR)
#define HAIP_DEMOD_MAX_FILTERED_SAMPLES 	(HAIP_DEMOD_MAX_SAMPLES + HAIP_SRCOS_COEFF_NUM)

#define HAIP_DEMOD_HEADER_CODED_LEN 	(HAIP_HEADER_AND_ADDR_LEN * HAIP_CODING_RATE)
#define HAIP_DEMOD_HEADER_SUBSAMPLES 	(HAIP_DEMOD_HEADER_CODED_LEN * 8/HAIP_BITS_PER_SYMBOL)
#define HAIP_DEMOD_HEADER_SAMPLES 	((HAIP_DEMOD_HEADER_SUBSAMPLES + HAIP_PREAMBLE_SYMBOLS) * HAIP_OVERSAMPLING_FACTOR)
#define HAIP_DEMOD_HEADER_NO_PREAMBLE (HAIP_DEMOD_HEADER_SUBSAMPLES * HAIP_OVERSAMPLING_FACTOR)
#define HAIP_DEMOD_HEADER_FILTERED 	(HAIP_DEMOD_HEADER_SAMPLES + HAIP_SRCOS_COEFF_NUM)

//FUNCTIONS
haip_sync_t haip_demodulate_head(fract32* analog_data, unsigned char* output_digital_data);
void haip_demodulate_payload(fract32* analog_data, int buffer_len, haip_sync_t sync,  unsigned char* output_digital_data);
void haip_init_demodulator(void);

#endif /* HAIP_DEMODULATOR_H_ */
