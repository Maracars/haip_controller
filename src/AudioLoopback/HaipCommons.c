/*
 * HaipCommons.c
 *
 *  Created on: 11 Dec 2017
 *      Author: ander
 */
#include "HaipCommons.h"
#include <time.h>
#include <stdio.h>

bool check_timeout(double last, double limit, double* new_last) {
	int instant;
	instant = clock() / CLOCKS_PER_SEC;
	bool tempBool = (instant - last) <= limit;
	*new_last = instant;
	return tempBool;
}

int char_to_dec(char c, int bits_p_symbol) {
	int ret_dec = 0;
	int i;
	for (i = 0; i < bits_p_symbol; i++) {
		ret_dec = ret_dec + c & pow(2, i);
	}
	return ret_dec;
}

