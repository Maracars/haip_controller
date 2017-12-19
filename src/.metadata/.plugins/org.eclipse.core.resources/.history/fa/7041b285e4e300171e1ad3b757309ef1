/*
 * declaraciones.h
 *
 *  Created on: 16/12/2013
 *      Author: nerea
 */

#ifndef DECLARACIONES_H_
#define DECLARACIONES_H_

//--------------------------------------------------------------------------//
// Header files																//
//--------------------------------------------------------------------------//
#include <sys\exception.h>
#include <cdefBF537.h>
#include <fract2float_conv.h>
#include <fract_typedef.h>

//--------------------------------------------------------------------------//
// Symbolic constants														//
//--------------------------------------------------------------------------//
// names for registers in AD1854/AD187 converters
#define INTERNAL_ADC_L0			0
#define INTERNAL_ADC_R0			1
#define INTERNAL_DAC_L0			0
#define INTERNAL_DAC_R0			1


#define delay 0xf00

// SPORT0 word length
#define SLEN_24	0x0017

// DMA flow mode
#define FLOW_1					0x1000


//-----------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// Global variables															//
//--------------------------------------------------------------------------//
extern int entrada0LeftIn;
extern int entrada0RightIn;
extern int cont_seno;
extern int cont_buffer;
extern int salida_left[];
extern int salida_right[];
extern int trama2_left[];
extern int trama2_right[];
extern float salida_left_float[];
extern float salida_right_float[];
extern fract16 entrada_left_fr16[];
extern fract16 entrada_right_fr16[];
extern int salida_dac[];
extern int entrada_adc[];
extern int iRxBuffer1[];
extern int iTxBuffer1[];
extern unsigned char m[];
extern float konstelazioa_real [];
extern float konstelazioa_imag [];
extern float simbolo_real[];
extern float simbolo_imag[];
extern float datos[];
extern int l[];
extern unsigned char simbolo[];
extern int indice;
extern float simbolo_muestreado_real[];
extern float simbolo_muestreado_imag[];
extern int eee;
extern int flag3;
extern int flag1;
extern int flag2;
extern int flag21;
extern int flag22;
extern int con;
extern unsigned char compr[];
extern int banderita;
extern unsigned char trama[];
extern float trama_envio[];
extern float seno[];
extern float coseno[];

//--------------------------------------------------------------------------//
// Prototypes																//
//--------------------------------------------------------------------------//
// in file Initialize.c
void Init_Flags(void);
void Audio_Reset(void);
void Init_Sport0(void);
void Init_DMA(void);
void Init_Interrupts(void);
void Enable_DMA_Sport0(void);

// in file Process_data.c
void Process_Data(void);

// in file ISRs.c
EX_INTERRUPT_HANDLER(Sport0_RX_ISR);

#endif /* DECLARACIONES_H_ */
