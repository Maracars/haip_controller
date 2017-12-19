/*
 * HaipTxRx.h
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */

#ifndef HAIPTXRX_H_
#define HAIPTXRX_H_

#include "haip_commons.h"

//FUNCTION DECLARATIONS
bool haiptxrx_iterate(void);
void haiptxrx_init_devices(ADI_UART_HANDLE uart, ADI_AD1871_HANDLE dac, ADI_AD1871_HANDLE adc);

#endif /* HAIPTXRX_H_ */
