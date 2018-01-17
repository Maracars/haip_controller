/*
 * HaipCommons.c
 *
 *  Created on: 11 Dec 2017
 *      Author: ander
 */
#include "haip_commons.h"
#include <time.h>
#include <stdio.h>

bool check_timeout(double last, double limit, double* new_last) {
	int instant;
	instant = clock() / CLOCKS_PER_SEC;
	bool tempBool = (instant - last) > limit;
	*new_last = instant;
	return tempBool;
}

