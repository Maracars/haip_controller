/*
 * HaipCommons.h
 *
 *  Created on: 11 Dec 2017
 *      Author: ander
 */

#ifndef HAIPCOMMONS_H_
#define HAIPCOMMONS_H_

//DEFINITIONS

#define UART_BUFFER_SIZE 50
#define HAIP_FRAME_LENGTH_MIN 5
#define HAIP_FRAME_LENGTH_MAX 260

#define HAIP_FRAME_HEADER_INDEX 0
#define HAIP_FRAME_DEST_ID_INDEX 1
#define HAIP_FRAME_SRC_ID_INDEX 2
#define HAIP_FRAME_DATA_LEN_INDEX 3



//DATA TYPES

typedef struct haip_sync_params {
	int phase_offset;
	int frequency_offset;
} haip_sync_params;

//Function declarations

bool check_timeout(unsigned long last, unsigned long limit, unsigned long* new_last);
int char_to_dec(char c, int bits_p_symbol);
#endif /* HAIPCOMMONS_H_ */