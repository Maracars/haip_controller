/*
 * haip_16QAM_mapping.h
 *
 *  Created on: 20 Dec 2017
 *      Author: ander
 */

#ifndef HAIP_16QAM_MAPPING_H_
#define HAIP_16QAM_MAPPING_H_

#include "haip_commons.h"

#include "fract_typedef.h"
#include <fract2float_conv.h>

#define HAIP_16QAM_NORM_F SQRT_10

section("sdram0") extern double haip_ideal_const[16][2];
section("sdram0") extern fract32 haip_const[16][2];

void haip_init_const(void);

#endif /* HAIP_16QAM_MAPPING_H_ */
