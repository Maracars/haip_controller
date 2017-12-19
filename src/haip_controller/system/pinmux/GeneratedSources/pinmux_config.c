/*
 **
 ** pinmux_config.c source file generated on September 17, 2012 at 21:25:12.	
 **
 ** Copyright (C) 2000-2012 Analog Devices Inc., All Rights Reserved.
 **
 ** This file is generated automatically based upon the options selected in 
 ** the Pin Multiplexing configuration editor. Changes to the Pin Multiplexing
 ** configuration should be made by changing the appropriate options rather
 ** than editing this file.
 **
 ** Selected Peripherals
 ** --------------------
 ** SPORT0 (DRPRI, RFS, RSCLK, DTPRI, TFS, TSCLK)
 ** SPORT1 (DRPRI, RSCLK, RFS, TFS, DTPRI, TSCLK)
 **
 ** GPIO (unavailable)
 ** ------------------
 ** PG10, PG11, PG12, PG13, PG14, PG15
 */

#include <sys/platform.h>
#include <stdint.h>

#define SPORT0_DTPRI_PORTJ_MUX  ((uint16_t) ((uint16_t) 0<<0))
#define SPORT0_TFS_PORTJ_MUX  ((uint16_t) ((uint16_t) 0<<0))
#define SPORT1_DRPRI_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<10))
#define SPORT1_RSCLK_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<10))
#define SPORT1_RFS_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<10))
#define SPORT1_TFS_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<11))
#define SPORT1_DTPRI_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<11))
#define SPORT1_TSCLK_PORTG_MUX  ((uint16_t) ((uint16_t) 1<<11))

#define SPORT0_DRPRI_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<8))
#define SPORT0_RFS_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<7))
#define SPORT0_RSCLK_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<6))
#define SPORT0_DTPRI_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<11))
#define SPORT0_TFS_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<10))
#define SPORT0_TSCLK_PORTJ_FER  ((uint16_t) ((uint16_t) 1<<9))
#define SPORT1_DRPRI_PORTG_FER  ((uint16_t) ((uint16_t) 1<<12))
#define SPORT1_RSCLK_PORTG_FER  ((uint16_t) ((uint16_t) 1<<10))
#define SPORT1_RFS_PORTG_FER  ((uint16_t) ((uint16_t) 1<<11))
#define SPORT1_TFS_PORTG_FER  ((uint16_t) ((uint16_t) 1<<14))
#define SPORT1_DTPRI_PORTG_FER  ((uint16_t) ((uint16_t) 1<<15))
#define SPORT1_TSCLK_PORTG_FER  ((uint16_t) ((uint16_t) 1<<13))




#define UART0_TX_PORTF_MUX  ((uint16_t) ((uint16_t) 0<<3))
#define UART0_RX_PORTF_MUX  ((uint16_t) ((uint16_t) 0<<3))

#define UART0_TX_PORTF_FER  ((uint16_t) ((uint16_t) 1<<0))
#define UART0_RX_PORTF_FER  ((uint16_t) ((uint16_t) 1<<1))




int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX and FER Registers
 */
int32_t adi_initpinmux(void) {
    /* PORTx_MUX registers */
    *pPORT_MUX = SPORT1_DRPRI_PORTG_MUX | SPORT1_RSCLK_PORTG_MUX
     | SPORT1_RFS_PORTG_MUX | SPORT1_TFS_PORTG_MUX | SPORT1_DTPRI_PORTG_MUX
     | SPORT1_TSCLK_PORTG_MUX;
    *pPORT_MUX |= SPORT0_DTPRI_PORTJ_MUX | SPORT0_TFS_PORTJ_MUX;


    *pPORT_MUX = UART0_TX_PORTF_MUX | UART0_RX_PORTF_MUX;



    /* PORTx_FER registers */
    *pPORTG_FER = SPORT1_DRPRI_PORTG_FER | SPORT1_RSCLK_PORTG_FER
     | SPORT1_RFS_PORTG_FER | SPORT1_TFS_PORTG_FER | SPORT1_DTPRI_PORTG_FER
     | SPORT1_TSCLK_PORTG_FER;


    *pPORTF_FER = UART0_TX_PORTF_FER | UART0_RX_PORTF_FER;



    return 0;
}
