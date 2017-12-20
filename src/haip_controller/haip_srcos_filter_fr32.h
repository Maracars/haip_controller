/*****************************************************************************
 File Name   : srcos_filter1_fr32.c
 ****************************************************************************/
#ifndef HAIP_SRCOS_FILTER_FR32_H_
#define HAIP_SRCOS_FILTER_FR32_H_

#include <filter.h>

#define HAIP_SRCOS_COEFF_NUM 			49
#define HAIP_SRCOS_FILTER_DELAY         ((HAIP_SRCOS_COEFF_NUM-1)/2)

#pragma section("L1_data_a")
fract32 haip_srcos_fir_fil_coeffs_fr32[]= {
0xff98d901,
0xffb087cc,
0x00000000,
0x005f78fa,
0x0094ff70,
0x0074ddf3,
0x00000000,
0xff6d9bd5,
0xff15dc50,
0xff433702,
0x00000000,
0x00fcdb8f,
0x01a57370,
0x01649819,
0x00000000,
0xfde22984,
0xfc289d50,
0xfc60daf2,
0x00000000,
0x07c2bd1e,
0x1334ed71,
0x20984d7c,
0x2d413ccd,
0x36532bcf,
0x399ec853,
0x36532bcf,
0x2d413ccd,
0x20984d7c,
0x1334ed71,
0x07c2bd1e,
0x00000000,
0xfc60daf2,
0xfc289d50,
0xfde22984,
0x00000000,
0x01649819,
0x01a57370,
0x00fcdb8f,
0x00000000,
0xff433702,
0xff15dc50,
0xff6d9bd5,
0x00000000,
0x0074ddf3,
0x0094ff70,
0x005f78fa,
0x00000000,
0xffb087cc,
0xff98d901
};

#endif
