//
// Created by ander on 12/01/2018.
//

#ifndef PRUEBAS_C_HAIP_DATA_DETECTOR_H
#define PRUEBAS_C_HAIP_DATA_DETECTOR_H

#include "fract_typedef.h"

int haip_next_data(fract32* input, int len);
void haip_detector_config(double new_margin, double update_weight);
fract32 haip_get_noise_lvl_fr32(void);

#endif //PRUEBAS_C_HAIP_DATA_DETECTOR_H
