/*
 * HaipTxRx.h
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */

#ifndef HAIPTXRX_H_
#define HAIPTXRX_H_

#include "HaipCommons.h"
//GLOBAL VARIABLES

extern ADI_UART_HANDLE uart_device;
extern ADI_AD1871_HANDLE dac_device;

//FUNCTION DECLARATIONS
void haiptxrx_iterate();


#endif /* HAIPTXRX_H_ */
