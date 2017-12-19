/*
 * HaipCommons.h
 *
 *  Created on: 11 Dec 2017
 *      Author: ander
 */

#ifndef HAIPCOMMONS_H_
#define HAIPCOMMONS_H_

#include <blackfin.h>
#include <stdio.h>
#include <services\int\adi_int.h>
#include <drivers\uart\adi_uart.h>
#include <services\pwr\adi_pwr.h>
#include "system/adi_initialize.h"
/* AD1854 driver includes */
#include <drivers/dac/ad1854/adi_ad1854.h>
/* AD1871 driver includes */
#include <drivers/adc/ad1871/adi_ad1871.h>
#include <string.h>
#include <math.h>
//DEFINITIONS

/* Baud rate to be used for char echo */
#define HAIP_BAUD_RATE           9600u

#define HAIP_UART_BUFFER_SIZE 50
#define HAIP_UART_DEV_NUM 0
#define HAIP_FRAME_LENGTH_MIN 5
#define HAIP_FRAME_LENGTH_MAX 260

#define HAIP_FRAME_HEADER_INDEX 0
#define HAIP_FRAME_DEST_ID_INDEX 1
#define HAIP_FRAME_SRC_ID_INDEX 2
#define HAIP_FRAME_DATA_LEN_INDEX 3

#define HAIP_DIGITAL_INPUT_TIMEOUT 0.02
#define HAIP_DIGITAL_INPUT_BUFFER_SIZE 500
#define HAIP_FRAME_BUFFER_SIZE 10
#define HAIP_AUDIO_BUFFER_SIZE 65536*2
#define HAIP_DAC_BUFFER_SIZE 900

//DATA TYPES
typedef struct haip_sync_params {
	int phase_offset;
	int frequency_offset;
} haip_sync_params;

//Function declarations
bool check_timeout(double last, double limit, double* new_last);
int char_to_dec(char c, int bits_p_symbol);
#endif /* HAIPCOMMONS_H_ */
