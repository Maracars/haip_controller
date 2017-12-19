/*
 * declaraciones.h
 *
 *  Created on: 16/12/2013
 *      Author: nerea
 */

#ifndef DECLARACIONES_H_
#define DECLARACIONES_H_

#include <filter.h>

//Definiciones UART
/* UART Device Number connected to RS232 socket on the BF537 EZ-KIT Lite */
#define UART_DEVICE_NUM     0u

/* BF537 specific default power settings */
#define MHZTOHZ             (1000000)
#define CLKIN               (25 * MHZTOHZ)
#define CORE_MAX            (400 * MHZTOHZ)
#define SYSTEM_MAX          (100 * MHZTOHZ)
#define VCO_MIN             (72 * MHZTOHZ)
//

//Definiciones mod/demod
#define PI 3.141592
#define BUFFER_SIZE 50
#define BUFFER_SIZE_CONV (65536*2)
#define SM 12

#define FC 4000 //Hz
#define FS 48000 //Hz

#define SIMBOLOS_CONSTELACION 16
#define NUM_BITS_POR_SIMBOLO 4
#define CARACTERES_PRUEBA 8
#define CORRECTOR_CONSTELACION_REAL 0.997211245
#define CORRECTOR_CONSTELACION_IMAGINARIO 0.074630638

#define COEFICIENTES_PORTADORA (FS/FC)
#define COEFICIENTES_FILTRO 73
#define VALOR_MAX_CONSTELACION 3
#define REGULADOR_POTENCIA (1.8/2.5)
#define REGULADOR_POTENCIA_DEMOD (3.0*4.28)

#define NUM_BITS (BUFFER_SIZE)*8
#define NUM_CODIFICADOS ((NUM_BITS)+((NUM_BITS) / NUM_BITS_POR_SIMBOLO )*3)
//#define NUM_SIMBOLOS (NUM_BITS) / NUM_BITS_POR_SIMBOLO
#define NUM_SIMBOLOS ((NUM_CODIFICADOS / NUM_BITS_POR_SIMBOLO)+CARACTERES_PRUEBA*2)
#define NUM_SOBREMUESTREADOS NUM_SIMBOLOS*SM
#define NUM_FILTRADOS_1 (NUM_SOBREMUESTREADOS + (COEFICIENTES_FILTRO - 1)/2)
#define NUM_MODULADOS NUM_FILTRADOS_1
#define NUM_FILTRADOS_2 (NUM_MODULADOS + (COEFICIENTES_FILTRO - 1)/2)
#define LONGITUD_SINCRONIZACION (SM+2)*(NUM_SIMBOLOS)
#define LONGITUD_DAC LONGITUD_SINCRONIZACION * 2
#define LONGITUD_ADC LONGITUD_DAC

#define VALOR_PICO 2000000
#define MIN_PICO 500000
#define LONGITUD_PICO 3

#define LONGITUD_SINDROME 3
#define LONGITUD_RECEPTOR 7
//

extern unsigned char trama_entrada_mod[];
extern unsigned char trama_salida_demod[];


//Declaraciones modulador/demodulador

extern fract32 entrada_dac [];
extern fract32 entrada_adc [];

extern int indice_bits;
extern int indice_simbolos;
extern int indice_muestrasFiltradas;
extern int indice_retraso;
extern int indice_muestrasModuladas;
extern int indice_sincronizacion;
extern int indice_conversor;
extern int indice_muestras;

extern float trama_entrada_mod_codificada[];
extern float trama_salida_demod_codificada[];

extern float constelacion_real [];
extern float constelacion_imag [];

extern float simbolos_comprobacion_real[];
extern float simbolos_comprobacion_imag[];

extern float seno[];
extern float coseno[];

extern fract32 filter_fr32[];
extern fir_state_fr32 state;

//



#endif /* DECLARACIONES_H_ */
