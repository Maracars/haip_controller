//
// Created by ander on 12/01/2018.
//

#include "haip_data_detector.h"
#include "fract2float_conv.h"
#include "fract_typedef.h"


double haip_update_avg(double old_avg, double new_value, double weight){
	return old_avg * (1 - weight) + new_value * weight;
}
int haip_next_data(fract32* input, int len, double avg, double margin){
	int i = 0;
	bool data_found = false;
	double low_lim = avg - margin;
	double high_lim = avg + margin;

	for(i = 0; i < len; i++) {
		if(fr32_to_float(input[i]) >=  high_lim || fr32_to_float(input[i]) <= low_lim){
			data_found = true;
			break;
		}
	}

	if(data_found)
		return i;
	else
		return -1;

}
