//
// Created by ander on 20/12/2017.
//

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
#define HAIP_BAUD_RATE                9600u

#define HAIP_UART_BUFFER_SIZE            50
#define HAIP_UART_DEV_NUM                0
#define HAIP_FRAME_LENGTH_MIN            5
#define HAIP_FRAME_LENGTH_MAX            260

#define HAIP_FRAME_HEADER_INDEX        0
#define HAIP_FRAME_DEST_ID_INDEX        1
#define HAIP_FRAME_SRC_ID_INDEX        2
#define HAIP_FRAME_DATA_LEN_INDEX        3

#define HAIP_DIGITAL_INPUT_TIMEOUT        0.1
#define HAIP_DIGITAL_INPUT_BUFFER_SIZE    500
#define HAIP_FRAME_BUFFER_SIZE            10
#define HAIP_ANALOG_BUFFER_SIZE        65536*2
#define HAIP_DAC_BUFFER_SIZE            900

/* Add your custom header content here */
#define HAIP_SAMPLING_FREQ                    48000//Hz
#define HAIP_CARRIER_FREQ                    6000//Hz
#define HAIP_SYMBOL_FREQ                    6000//Hz
#define HAIP_OVERSAMPLING_FACTOR            (HAIP_SAMPLING_FREQ/HAIP_SYMBOL_FREQ)
#define HAIP_NUMBER_OF_SYMBOLS_OVERSAMPLED  (NUMBER_OF_SYMBOLS*8)
#define HAIP_NUM_SAMPLES_TX                 (NUMBER_OF_SYMBOLS_OVERSAMPLED + NUM_COEFFS)
#define HAIP_NUM_SAMPLES_RX                 (NUMBER_OF_SYMBOLS_OVERSAMPLED + NUM_COEFFS*2)
#define HAIP_SYMBOLS_16QAM                    16
#define HAIP_BITS_PER_SYMBOL                4
#define HAIP_CODING_WORDLEN                    8
#define HAIP_CODING_DATABITS                4
#define HAIP_CODING_RATE                    HAIP_CODING_WORDLEN / HAIP_CODING_DATABITS
#define HAIP_PREAMBLE_SYMBOLS                8
#define HAIP_SYMBOLS_PER_BYTE                8/HAIP_BITS_PER_SYMBOL
/* Haip frame structure */
#define HAIP_FRAME_HEADER_LEN            1
#define HAIP_FRAME_HEADER_OFF            1
#define HAIP_HEADER_LEN_LEN            3
#define HAIP_HEADER_LEN_OFF                0
#define HAIP_HEADER_TYPE_LEN            2
#define HAIP_HEADER_TYPE_OFF            3
#define HAIP_HEADER_CNT_LEN            3
#define HAIP_HEADER_CNT_OFF            5
#define HAIP_FRAME_DEST_LEN            1
#define HAIP_FRAME_ORIG_LEN            1
#define HAIP_FRAME_DEST_OFF            2
#define HAIP_FRAME_ORIG_OFF            1
#define HAIP_FRAME_DATA_OFF            3
#define HAIP_FRAME_CRC_LEN            1

#define HAIP_FRAME_DATA_MAX_LEN            7 //HAIP_HEADER_LEN_LEN^2 - 1
#define HAIP_HEADER_AND_ADDR_LEN        HAIP_FRAME_ORIG_LEN + HAIP_FRAME_DEST_LEN + HAIP_FRAME_HEADER_LEN
#define HAIP_FRAME_MAX_LEN                HAIP_HEADER_AND_ADDR_LEN + HAIP_FRAME_CRC_LEN + HAIP_FRAME_DATA_MAX_LEN


typedef struct haip_header_t {
    union {
        struct {
        	uint8_t cnt :HAIP_HEADER_CNT_LEN;
            uint8_t type :HAIP_HEADER_TYPE_LEN;
            uint8_t len :HAIP_HEADER_LEN_LEN;
        };
        uint8_t header;
    };
} haip_header_t;

typedef struct haip_sync_t {
    uint8_t sample;
    double phase_off;
    float att;
    int start_indx;
} haip_sync_t;

//Mathematical constant
#define SQRT_10                        3.17
#define SQRT_2                            1.4142

//Function declarations
bool check_timeout(double last, double limit, double *new_last);

int char_to_dec(char c, int bits_p_symbol);

#endif /* HAIPCOMMONS_H_ */
