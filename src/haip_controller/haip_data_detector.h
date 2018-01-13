//
// Created by ander on 12/01/2018.
//

#ifndef PRUEBAS_C_HAIP_DATA_DETECTOR_H
#define PRUEBAS_C_HAIP_DATA_DETECTOR_H

#include "fract_typedef.h"

double haip_update_avg(double old_avg, double new_value, double weight);
int haip_next_data(fract32* input, int len, double avg, double margin);

#endif //PRUEBAS_C_HAIP_DATA_DETECTOR_H
