//
// Created by ander on 12/01/2018.
//

#include "haip_data_detector.h"
#include "fract2float_conv.h"
#include "fract_typedef.h"

double avg = 0;
static double margin = 0.2;
static double new_avg_max_weight = 0.4;

void update_avg(double new_value, double weight);

void update_avg(double new_value, double weight){
	avg = avg * (1 - weight) + new_value * weight;
}

int haip_next_data(fract32* input, int len){
	int i = 0;
	bool data_found = false;
	double low_lim = avg - margin;
	double high_lim = avg + margin;
	double sum = 0;
	double actual;

	for(i = 0; i < len-1; i += 2) {
		actual = fr32_to_float(input[i] << 8);
		if(actual >=  high_lim || actual <= low_lim){
			data_found = true;
			break;
		}
		sum += actual;
	}
	if(i > 0)
		update_avg(sum / i, new_avg_max_weight - new_avg_max_weight/i);
	if(data_found)
		return i;
	else
		return -1;

}

void haip_detector_config(double new_margin, double update_weight){
	margin = new_margin;
	new_avg_max_weight = update_weight;
}

fract32 haip_get_noise_lvl_fr32(){
	return float_to_fr32(avg);
}
