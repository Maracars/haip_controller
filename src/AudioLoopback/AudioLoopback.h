/*********************************************************************************

 Copyright(c) 2012 Analog Devices, Inc. All Rights Reserved.

 This software is proprietary and confidential.  By using this software you agree
 to the terms of the associated Analog Devices License Agreement.

 *********************************************************************************/
/*!
 * @file      AudioLoopback.h
 * @brief     Audio loopback example using AD1871 ADC and AD1854 DAC.
 * @version:  $Revision: 9599 $
 * @date:     $Date: 2012-06-26 02:46:24 -0400 (Tue, 26 Jun 2012) $
 *
 * @details
 *            This is the primary include file for audio loopback example using
 *            AD1871 Stereo Audio ADC and AD1854 Stereo Audio DAC drivers.
 *
 */

/*=============  I N C L U D E S   =============*/
//#ifndef AUDIO_LOOPBACK_H_
//#define AUDIO_LOOPBACK_H_
/* AD1854 driver includes */
#include <drivers/dac/ad1854/adi_ad1854.h>
/* AD1871 driver includes */
#include <drivers/adc/ad1871/adi_ad1871.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*=============  D E F I N E S  =============*/

/* AD1854 Device instance number */
#define AD1854_DEV_NUM          (0u)

/* AD1871 Device instance number */
#define AD1871_DEV_NUM          (0u)

/* SPORT Device number allocated to AD1854 */
#define AD1854_SPORT_DEV_NUM    (0u)

/* SPORT Device number allocated to AD1871 */
#define AD1871_SPORT_DEV_NUM    (0u)

/* Device number of the UART */
#define UART_DEV_NUM    (0u)

/* Buffer size */
#define	BUFFER_SIZE				4

/* Application time out value */
#define	TIME_OUT_VAL			(0xFFFFFFFu)

/* GPIO port/pin connected to AD1854 DAC reset pin */
#define	AD1854_RESET_PORT		ADI_GPIO_PORT_F
#define	AD1854_RESET_PIN		ADI_GPIO_PIN_12

/* GPIO port/pin connected to AD1871 ADC reset pin */
#define	AD1871_RESET_PORT		ADI_GPIO_PORT_F
#define	AD1871_RESET_PIN		ADI_GPIO_PIN_12

/**** Processor specific Macros ****/

/* External input clock frequency in Hz */
#define 	PROC_CLOCK_IN       		25000000
/* Maximum core clock frequency in Hz */
#define 	PROC_MAX_CORE_CLOCK 		600000000
/* Maximum system clock frequency in Hz */
#define 	PROC_MAX_SYS_CLOCK  		133333333
/* Minimum VCO clock frequency in Hz */
#define 	PROC_MIN_VCO_CLOCK  		25000000
/* Required core clock frequency in Hz */
#define 	PROC_REQ_CORE_CLOCK 		400000000
/* Required system clock frequency in Hz */
#define 	PROC_REQ_SYS_CLOCK  		100000000

#define DEBUG_MSG1(message)     printf(message)
#define DEBUG_MSG2(message, result) \
    do { \
        printf(message); \
        if(result) \
        { \
            printf(", Error Code: 0x%08X", result); \
            printf("\n"); \
        } \
    } while (0)
//#endif /* AUDIO_LOOPBACK_H_ */
/*****/
