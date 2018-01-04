/*****************************************************************************
 File Name   : srcos_filter1_fr32.c
 ****************************************************************************/
#ifndef HAIP_SRCOS_FILTER_FR32_H_
#define HAIP_SRCOS_FILTER_FR32_H_

#include <filter.h>

#define HAIP_SRCOS_COEFF_NUM 			49
#define HAIP_SRCOS_FILTER_DELAY         ((HAIP_SRCOS_COEFF_NUM-1)/2)


extern fract32 haip_srcos_fir_fil_coeffs_fr32[HAIP_SRCOS_COEFF_NUM];

#endif
