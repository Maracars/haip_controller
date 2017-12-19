/*
 * demodulador.c
 *
 *  Created on: 18/12/2013
 *      Author: Usuario
 */

/*****************************************************************************
**																			**
**	 Nombre del archivo:	Demodulador.c								    **
**																			**
******************************************************************************

Descripcion: Crear el demodulador de un modem


******************************************************************************/
#include <fract_typedef.h>
#include <filter.h>
#include <fract2float_conv.h>
#include <fract_typedef.h>
#include <math.h>

#include "funciones.h"
#include "declaraciones.h"

void interpretarEntradaADC(void);
void oscilador2 (void);
void filtro2 (void);
void decimador (void);
void corregirConstelacion (void);
void ajustarPicos (void);
void demapeador (void);

/*****************************************************************************

Variables para la modulacion de las tramas

******************************************************************************/

//Contadores
segment ("sdram0") int indice_muestrasPortadora = 0;
segment ("sdram0") int indice_muestrasDemoduladas = 0;
segment ("sdram0") int indice_muestrasDecimadas = 0;
segment ("sdram0") int indice_simbolosConstelacion = 0;
segment ("sdram0") int indices_real[SIMBOLOS_CONSTELACION];
segment ("sdram0") int indices_imag[SIMBOLOS_CONSTELACION];
segment ("sdram0") int indice_constelacion = 0;
segment ("sdram0") int indice_deteccion_real = 0;
segment ("sdram0") int indice_deteccion_imag = 0;
segment ("sdram0") int indice_deteccion = 0;
segment ("sdram0") int indice_correccion= 0;

segment ("sdram0") int h;
segment ("sdram0") int distancia_minima;

segment ("sdram0") float simbolos_salida_real[NUM_SIMBOLOS];
segment ("sdram0") float simbolos_salida_imag[NUM_SIMBOLOS];

//Variables obtenidas después del adc
segment ("sdram0") float trama_entrada_demod_float[NUM_MODULADOS];
segment ("sdram0") fract32 trama_entrada_demod_fr32[NUM_MODULADOS];

segment ("sdram0") float sincronizacion2_float[LONGITUD_SINCRONIZACION];
segment ("sdram0") fract32 sincronizacion2_fr32[LONGITUD_SINCRONIZACION];

//Variables para el filtro

segment ("sdram0") fract32 left_in_fr32_real_2[NUM_FILTRADOS_2];
segment ("sdram0") fract32 left_in_fr32_imag_2[NUM_FILTRADOS_2];
segment ("sdram0") fract32 left_out_fr32_real_2[NUM_FILTRADOS_2];
segment ("sdram0") fract32 left_out_fr32_imag_2[NUM_FILTRADOS_2];
segment ("sdram0") float left_out_float_real_2[NUM_FILTRADOS_2];
segment ("sdram0") float left_out_float_imag_2[NUM_FILTRADOS_2];
section("L1_data_b") fract32 retraso2[COEFICIENTES_FILTRO];

//Variables en las que se guarda los valores trasladados a la frecuencia de la señal portadora
segment ("sdram0") float garraiatzaile_real_2[NUM_MODULADOS];
segment ("sdram0") float garraiatzaile_imag_2[NUM_MODULADOS];

//Variables en la que se guarda los valores decimados
segment ("sdram0") float decimador_real[NUM_SIMBOLOS];
segment ("sdram0") float decimador_imag[NUM_SIMBOLOS];

//Variables en la que se guarda los valores detectados
segment ("sdram0") float detectado_real[NUM_SIMBOLOS];
segment ("sdram0") float detectado_imag[NUM_SIMBOLOS];

//Variables en la que se guarda los valores empleados para la decodificacion
segment ("sdram0") float trama_salida_demod_codificada[NUM_CODIFICADOS];

/*****************************************************************************

Funciones declaradas

******************************************************************************/
//--------------------------------------------------------------------------//
// Funcion:	demodulador														//
//																			//
// Descripcion: Funcion general que llama a las funciones concretas  		//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void demodulador(){

	interpretarEntradaADC();
	oscilador2();
	filtro2();
	decimador();
	corregirConstelacion();
	demapeador();
	decodificador();

}
//--------------------------------------------------------------------------//
// Funcion:	adc												                //
//																			//
// Descripcion: Funcion que convierte los datos al recibir (de int a float) //
// para que puedan procesarse           									//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void interpretarEntradaADC(){

	for(indice_conversor=0 ; indice_conversor<LONGITUD_ADC ; indice_conversor+=2){

		sincronizacion2_fr32[indice_conversor/2] = ((entrada_adc[indice_conversor]));     //Derecha (Sincronización)

		if(indice_conversor/2 < NUM_MODULADOS){
			trama_entrada_demod_fr32[indice_conversor/2]=((entrada_adc[indice_conversor+1])*(-1)); //Izquierda (Datos)
		}

	}


	//Convertir de fract32 a float

	for(indice_muestrasModuladas = 0 ; indice_muestrasModuladas < NUM_MODULADOS ; indice_muestrasModuladas++){

		trama_entrada_demod_float[indice_muestrasModuladas] = VALOR_MAX_CONSTELACION * fr32_to_float(trama_entrada_demod_fr32[indice_muestrasModuladas]  / REGULADOR_POTENCIA);

	}

	for(indice_sincronizacion = 0 ; indice_sincronizacion < LONGITUD_SINCRONIZACION ; indice_sincronizacion++){

		sincronizacion2_float[indice_sincronizacion] = VALOR_PICO * fr32_to_float(sincronizacion2_fr32[indice_sincronizacion]);

	}

	ajustarPicos();

}

//--------------------------------------------------------------------------//
// Funcion:	ajustarPicos										            //
//																			//
// Descripcion: Funcion que se encarga de que todos los picos tengan        //
// exactamente 3 valores, ni más ni menos                                   //
//                       													//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void ajustarPicos(){


	for(indice_sincronizacion = 0 ; indice_sincronizacion<LONGITUD_SINCRONIZACION ; ){


		if((sincronizacion2_float[indice_sincronizacion]>MIN_PICO) && (sincronizacion2_float[indice_sincronizacion+1]>MIN_PICO)){ //Detección de picos en la señal de sincronización


			indice_sincronizacion = indice_sincronizacion + LONGITUD_PICO;

			for(indice_muestras = 0 ; indice_muestras < SM-1 ; indice_muestras++){
				if(indice_sincronizacion+indice_muestras < LONGITUD_SINCRONIZACION){
					sincronizacion2_float[indice_sincronizacion+indice_muestras] = 0;
				}
			}

		}else{
			indice_sincronizacion++;
		}

	}


}

//--------------------------------------------------------------------------//
// Funcion:	oscilador2											            //
//																			//
// Descripcion: Funcion que separa la parte real e imaginaria de la señal   //
// multiplicandola por la señal portadora valiendonos de la señal de        //
// sincronizacion          													//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void oscilador2(){

	indice_sincronizacion = 0;
	bool muestrasValidas = false;
	//int contador_picos = 0;

	for(indice_muestrasModuladas=0;indice_muestrasModuladas<NUM_MODULADOS;indice_muestrasModuladas++){

		if(indice_muestrasModuladas == ((COEFICIENTES_FILTRO-1)/2)){
			indice_muestrasPortadora = 0;
			muestrasValidas = true;
		}

		if(muestrasValidas){
			if((sincronizacion2_float[indice_sincronizacion]>MIN_PICO) && (sincronizacion2_float[indice_sincronizacion+1]>MIN_PICO)
			    /*&& (sincronizacion2_float[indice_sincronizacion+2]>MIN_PICO)*/){ //Detección de picos en la señal de sincronización

				//contador_picos++;
				//if(contador_picos == 2){
					indice_muestrasPortadora = 0; //Esto significa que después de multiplicar SM muestras, el indice que se emplea para multiplicar las muestras
											  //con los diferentes valores del seno y del coseno se vuelve a inicializar a cero
					//contador_picos = 0;
				//}

				indice_sincronizacion = indice_sincronizacion + LONGITUD_PICO;

			}else{
				indice_sincronizacion++;
			}
		}

			garraiatzaile_real_2[indice_muestrasModuladas]=trama_entrada_demod_float[indice_muestrasModuladas]*coseno[(indice_muestrasPortadora)%COEFICIENTES_PORTADORA];
			garraiatzaile_imag_2[indice_muestrasModuladas]=-trama_entrada_demod_float[indice_muestrasModuladas]*seno[(indice_muestrasPortadora)%COEFICIENTES_PORTADORA];

			indice_muestrasPortadora++;

	}
}
//--------------------------------------------------------------------------//
// Funcion:	filtro2   														//
//																			//
// Descripcion: Funcion que filtra la señal de entrada para darle forma,va  //
// haciendiendo la convolucion entre los simbolos muestreados y los 		//
// coeficientes del filtro aunque le añade un retraso						//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void filtro2(){
	int k;


	for(indice_muestrasDemoduladas=0 ; indice_muestrasDemoduladas<NUM_MODULADOS ; indice_muestrasDemoduladas++)
	{
		left_in_fr32_real_2[indice_muestrasDemoduladas]=float_to_fr32(garraiatzaile_real_2[indice_muestrasDemoduladas])/(VALOR_MAX_CONSTELACION);
		left_in_fr32_imag_2[indice_muestrasDemoduladas]=float_to_fr32(garraiatzaile_imag_2[indice_muestrasDemoduladas])/(VALOR_MAX_CONSTELACION);
	}

	for (indice_retraso = 0 ; indice_retraso < COEFICIENTES_FILTRO ; indice_retraso++){

  		retraso2[indice_retraso] = 0;
	}

	//fir_init(state_real,filter_fr32,retraso2,COEFICIENTES_FILTRO,0);
	//fir_fr32(left_in_fr32_real_2,left_out_fr32_real_2,NUM_FILTRADOS_2,&state_real);

	for (indice_retraso = 0 ; indice_retraso < COEFICIENTES_FILTRO ; indice_retraso++){

	  		retraso2[indice_retraso] = 0;
	}

	//fir_init(state_imag,filter_fr32,retraso2,COEFICIENTES_FILTRO,0);
	//fir_fr32(left_in_fr32_imag_2,left_out_fr32_imag_2,NUM_FILTRADOS_2,&state_imag);

	for(indice_muestrasFiltradas=0 ; indice_muestrasFiltradas<NUM_FILTRADOS_2 ; indice_muestrasFiltradas++)
	{
		left_out_float_real_2[indice_muestrasFiltradas] = VALOR_MAX_CONSTELACION*fr32_to_float(left_out_fr32_real_2[indice_muestrasFiltradas]);
		left_out_float_imag_2[indice_muestrasFiltradas] = VALOR_MAX_CONSTELACION*fr32_to_float(left_out_fr32_imag_2[indice_muestrasFiltradas]);
	}
}
//--------------------------------------------------------------------------//
// Funcion:	decimador   													//
//																			//
// Descripcion: Funcion que elige la muestra optima de la trama valiendonos //
// de la señal de sincronizacion											//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void decimador(){

	indice_sincronizacion = 0;
	indice_muestrasDecimadas = 0;

	for(indice_muestrasFiltradas=0 ; indice_muestrasFiltradas<NUM_FILTRADOS_2 && indice_muestrasDecimadas<NUM_SIMBOLOS ; indice_muestrasFiltradas++){

		if(indice_muestrasFiltradas >= ((COEFICIENTES_FILTRO-1))){//Sólo se hace la detección en los valores que contienen la información

			if((sincronizacion2_float[indice_sincronizacion]>MIN_PICO) && (sincronizacion2_float[indice_sincronizacion+1]>MIN_PICO)
				/*&& (sincronizacion2_float[indice_sincronizacion+2]>MIN_PICO)*/){ //Detección de picos en la señal de sincronización

				decimador_real[indice_muestrasDecimadas]=left_out_float_real_2[indice_muestrasFiltradas]*REGULADOR_POTENCIA_DEMOD;
				decimador_imag[indice_muestrasDecimadas]=left_out_float_imag_2[indice_muestrasFiltradas]*REGULADOR_POTENCIA_DEMOD;

				indice_muestrasDecimadas++;
				indice_sincronizacion = indice_sincronizacion + LONGITUD_PICO;

			}else{
				indice_sincronizacion++;
			}
		}
	}

}

//--------------------------------------------------------------------------//
// Funcion:	corregirConstelacion 										    //
//																			//
// Descripcion: Calcula el desfase del ángulo y la amplitud y corrige los   //
// símbolos decimados											            //
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void corregirConstelacion(){

	float anguloCorrecto , anguloDesfasado;
	float mediaDesfase , mediaDiferenciaAmplitudX , mediaDiferenciaAmplitudY;
	float desfase = 0 , diferenciaAmplitudX = 0 , diferenciaAmplitudY = 0;

	for(indice_correccion = 0 ;  indice_correccion < CARACTERES_PRUEBA*2 ; indice_correccion++){

		if(simbolos_comprobacion_real[indice_correccion] > 0){
			anguloCorrecto = atan (simbolos_comprobacion_imag[indice_correccion] / simbolos_comprobacion_real[indice_correccion]);
			anguloDesfasado = atan (decimador_imag[indice_correccion] / decimador_real[indice_correccion]);
		}else{
			anguloCorrecto = atan (simbolos_comprobacion_imag[indice_correccion] / simbolos_comprobacion_real[indice_correccion]) + PI;
			anguloDesfasado = atan (decimador_imag[indice_correccion] / decimador_real[indice_correccion]) + PI;

		}

		float hipotenusaCorrecta = sqrt(pow(simbolos_comprobacion_imag[indice_correccion] , 2) + pow(simbolos_comprobacion_real[indice_correccion] , 2));
		float hipotenusaDesfasada = sqrt(pow(decimador_imag[indice_correccion] , 2) + pow(decimador_real[indice_correccion] , 2));

		desfase =desfase + (anguloCorrecto - anguloDesfasado);

		float diferenciaAmplitud = hipotenusaDesfasada - hipotenusaCorrecta;
		diferenciaAmplitudX = diferenciaAmplitudX + diferenciaAmplitud * cos (anguloCorrecto);
	    diferenciaAmplitudY = diferenciaAmplitudY + diferenciaAmplitud * sin (anguloCorrecto);

	}

	mediaDesfase = desfase / (CARACTERES_PRUEBA*2);
	mediaDiferenciaAmplitudX = diferenciaAmplitudX / (CARACTERES_PRUEBA*2);
	mediaDiferenciaAmplitudY = diferenciaAmplitudY / (CARACTERES_PRUEBA*2);

	for(indice_muestrasDecimadas = 0 ;  indice_muestrasDecimadas < NUM_SIMBOLOS ; indice_muestrasDecimadas++){

		if((floor(pow(10,5)*(simbolos_comprobacion_real[indice_muestrasDecimadas]) + 0.5) / pow(10,5)) == 1 && (floor( pow(10,5)*(simbolos_comprobacion_imag[indice_muestrasDecimadas])  + 0.5) / pow(10,5)) == 1){
			indice_correccion = indice_muestrasDecimadas;
		}

		if(decimador_real[indice_muestrasDecimadas] > 0){
			anguloDesfasado = atan (decimador_imag[indice_muestrasDecimadas] / decimador_real[indice_muestrasDecimadas]);
		}else{
			anguloDesfasado = atan (decimador_imag[indice_muestrasDecimadas] / decimador_real[indice_muestrasDecimadas]) + PI;
		}

		float hipotenusaDesfasada = sqrt(pow(decimador_imag[indice_muestrasDecimadas] , 2) + pow(decimador_real[indice_muestrasDecimadas] , 2));

		decimador_real[indice_muestrasDecimadas] = cos (anguloDesfasado + mediaDesfase) * hipotenusaDesfasada;
		decimador_imag[indice_muestrasDecimadas] = sin (anguloDesfasado + mediaDesfase) * hipotenusaDesfasada;

		decimador_real[indice_muestrasDecimadas] = decimador_real[indice_muestrasDecimadas] - mediaDiferenciaAmplitudX;
		decimador_imag[indice_muestrasDecimadas] = decimador_imag[indice_muestrasDecimadas] - mediaDiferenciaAmplitudY;
	}

}

void demapeador (void){

	int num_indices_real = 0;
	int num_indices_imag = 0;

	for (indice_simbolos=CARACTERES_PRUEBA*2 ; indice_simbolos<NUM_SIMBOLOS ; indice_simbolos++){

		distancia_minima = 1000;
		if(indice_simbolos == NUM_SIMBOLOS){
			distancia_minima = 1000;
		}

		//Parte real
		for (indice_constelacion=0 ; indice_constelacion<SIMBOLOS_CONSTELACION ; indice_constelacion++){

			if (abs(constelacion_real[indice_constelacion] - decimador_real[indice_simbolos]) <= distancia_minima){

				distancia_minima = abs(constelacion_real[indice_constelacion] - decimador_real[indice_simbolos]);

				if(distancia_minima == 0){
					break;
				}
			}
		}

		for(indice_constelacion=0 ; indice_constelacion<SIMBOLOS_CONSTELACION ; indice_constelacion++){

			if (abs(constelacion_real[indice_constelacion] - decimador_real[indice_simbolos]) == distancia_minima){

				indices_real[num_indices_real++] = indice_constelacion;
			}

		}
		//

		distancia_minima = 1000;

		//Parte imaginaria
		for (indice_constelacion=0 ; indice_constelacion<SIMBOLOS_CONSTELACION ; indice_constelacion++){

			if (abs(constelacion_imag[indice_constelacion] - decimador_imag[indice_simbolos]) <= distancia_minima){

				distancia_minima = abs(constelacion_imag[indice_constelacion] - decimador_imag[indice_simbolos]);

				if(distancia_minima == 0){
					break;
				}

			}

		}

		for (indice_constelacion=0 ; indice_constelacion<SIMBOLOS_CONSTELACION ; indice_constelacion++){

			if (abs(constelacion_imag[indice_constelacion] - decimador_imag[indice_simbolos]) == distancia_minima){

				indices_imag[num_indices_imag++] = indice_constelacion;
			}

		}
		//

		//Obtener el indice del simbolo en la constelación
		for(indice_deteccion_real=0 ; indice_deteccion_real<num_indices_real ; indice_deteccion_real++){

			for(indice_deteccion_imag=0 ; indice_deteccion_imag<num_indices_real ; indice_deteccion_imag++){

				if(indices_real[indice_deteccion_real] == indices_imag[indice_deteccion_imag]){

					indice_deteccion = indices_real[indice_deteccion_real];
					break;

				}
			}
		}

		simbolos_salida_real[indice_simbolos-CARACTERES_PRUEBA*2] = constelacion_real[indice_deteccion];
		simbolos_salida_imag[indice_simbolos-CARACTERES_PRUEBA*2] = constelacion_imag[indice_deteccion];

		//Convertir los simbolos en bits
		for (indice_bits=0 ; indice_bits<NUM_BITS_POR_SIMBOLO ; indice_bits++){

			trama_salida_demod_codificada[(indice_simbolos-CARACTERES_PRUEBA*2+1)*NUM_BITS_POR_SIMBOLO - (indice_bits + 1)] = indice_deteccion % 2;
			indice_deteccion =  floor(indice_deteccion/2);

		}


		num_indices_real = 0;
		num_indices_imag = 0;

	}

}

