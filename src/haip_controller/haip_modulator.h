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

int haip_modulate_frame(unsigned char* frame_buffer, int frame_length, fract32* modulated_signal);

#endif /* HAIPMODULATOR_H_ */
