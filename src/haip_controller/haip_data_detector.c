//
// Created by ander on 12/01/2018.
//

#include "haip_data_detector.h"
#include <fract2float_conv.h>
#include <fract_typedef.h>

static double avg = 0;
static double margin = 0.15;

double update_avg(double old_avg, double new_value, double weight);

double update_avg(double old_avg, double new_value, double weight){
	return old_avg * (1 - weight) + new_value * weight;
}
int haip_next_data(fract32* input, int len){
	int i = 0;
	double sum = 0;
	double current = 0;
	bool data_found = false;
	double low_lim = avg - margin;
	double high_lim = avg + margin;

	for(i = 0; i < len; i++) {
		current = fr32_to_float(input[i]);
		sum += current;
		if(current >=  high_lim || current <= low_lim){
			data_found = true;
			break;
		}
	}

	double curr_avg = sum / i;
	avg = update_avg(avg, curr_avg, 0.5 - 0.5/(i+1));

	if(data_found)
		return i;
	else
		return -1;

}
