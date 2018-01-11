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

int haip_modulate_frame(unsigned char* frame_buffer, int frame_length, fract32* modulated_signal);

#endif /* HAIPMODULATOR_H_ */
