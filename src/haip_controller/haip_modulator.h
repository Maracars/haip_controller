/*
 * HaipModulator.h
 *
 *  Created on: 11/12/2017
 *      Author: joseba
 */

#ifndef HAIPMODULATOR_H_
#define HAIPMODULATOR_H_

#include <fract_typedef.h>
fract32* modulate_frame(unsigned char* frame_buffer, int frame_length);

void addPreamble(void);
void mapper(unsigned char* frame_buffer, int frame_symbols);
void upsample(int frame_symbols);
void filter(void);
void oscilate(void);
void addHeader(void);


#endif /* HAIPMODULATOR_H_ */
