//
// Created by ander on 12/01/2018.
//

#ifndef PRUEBAS_C_HAIP_DATA_DETECTOR_H
#define PRUEBAS_C_HAIP_DATA_DETECTOR_H

#include "fract_typedef.h"

int haip_next_data(fract32* input, int len);
fract32 haip_get_noise_level(void);

#endif //PRUEBAS_C_HAIP_DATA_DETECTOR_H
