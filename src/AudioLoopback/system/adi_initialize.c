/*
 ** ADI initialize C file.
 **
 ** Copyright (C) 2000-2012 Analog Devices Inc., All Rights Reserved.
 **
 ** This file is generated automatically
 */

#include "adi_initialize.h"
int32_t adi_initpinmux(void); /* auto-generated code */

static int32_t adi_initInterrupts(void);

int32_t adi_initComponents(void) {
	int32_t result = 0;
	result = adi_initInterrupts();
	if (result == 0) {
		result = adi_initpinmux(); /* auto-generated code (order:0) */
	}

	return result;
}

static int32_t adi_initInterrupts(void) {
	int32_t result = 0;

#ifdef __HAS_SEC__
	result = adi_sec_Init();
#endif

	return result;
}
