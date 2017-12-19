/*!
 * =========== H A I P -  M O D E M =============
 */
#ifndef HAIP_MODEM_H_
#define HAIP_MODEM_H_
/*=============  I N C L U D E S   =============*/

/* AD1854 driver includes */
#include <drivers/dac/ad1854/adi_ad1854.h>
/* AD1871 driver includes */
#include <drivers/adc/ad1871/adi_ad1871.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*=============  D E F I N E S  =============*/

/* Enable macro to build example in callback mode */
//#define ENABLE_CALLBACK
/* Enable macro to enable application time-out */
//#define ENABLE_APP_TIME_OUT
/* Enable macro to display debug information */
#define ENABLE_DEBUG_INFO

/* AD1854 Device instance number */
#define AD1854_DEV_NUM          (0u)

/* AD1871 Device instance number */
#define AD1871_DEV_NUM          (0u)

/* SPORT Device number allocated to AD1854 */
#define AD1854_SPORT_DEV_NUM    (0u)

/* SPORT Device number allocated to AD1871 */
#define AD1871_SPORT_DEV_NUM    (0u)

/* Buffer size */
#define	BUFFER_SIZE				50

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

/* IF (Debug info enabled) */
#if defined(ENABLE_DEBUG_INFO)
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
/* ELSE (Debug info disabled) */
#else
#define DEBUG_MSG1(message)
#define DEBUG_MSG2(message, result)
#endif

#endif

/*****/
