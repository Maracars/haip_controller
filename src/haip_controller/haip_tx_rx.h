/*
 * HaipTxRx.h
 *
 *  Created on: 5 Dec 2017
 *      Author: ander
 */

#ifndef HAIPTXRX_H_
#define HAIPTXRX_H_

//#include "haip_commons.h"
/* AD1854 driver includes */
#include <drivers/dac/ad1854/adi_ad1854.h>
/* AD1871 driver includes */
#include <drivers/adc/ad1871/adi_ad1871.h>
/*UART driver include */
#include <drivers\uart\adi_uart.h>


//FUNCTION DECLARATIONS
bool haiptxrx_iterate(void);
void haiptxrx_init_devices(ADI_UART_HANDLE uart, ADI_AD1871_HANDLE dac, ADI_AD1871_HANDLE adc);

#endif /* HAIPTXRX_H_ */
