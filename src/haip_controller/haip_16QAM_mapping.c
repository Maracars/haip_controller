/*
 * haip_16QAM_mapping.c
 *
 *  Created on: 20 Dec 2017
 *      Author: ander
 */


#include "haip_16QAM_mapping.h"

double haip_ideal_const[16][2] = { { -3, 3 }, { -3, 1 }, { -3, -3 }, { -3,
		-1 }, { -1, 3 }, { -1, 1 }, { -1, -3 }, { -1, -1 }, { 3, 3 }, { 3, 1 },
		{ 3, -3 }, { 3, -1 }, { 1, 3 }, { 1, 1 }, { 1, -3 }, { 1, -1 } };

fract32 haip_const[16][2];

void haip_init_const(void){
	for (int i = 0;  i < 16; i++) {
			haip_const[i][0] =	float_to_fr32(haip_ideal_const[i][0]/HAIP_16QAM_NORM_F);
			haip_const[i][1] =	float_to_fr32(haip_ideal_const[i][1]/HAIP_16QAM_NORM_F);
	}
}
