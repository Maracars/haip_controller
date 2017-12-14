/*****************************************************************************
 **																			**
 **	 Nombre del archivo:	Modulador.c								        **
 **																			**
 ******************************************************************************

 Descripcion: Crear el modulador de un modem


 ******************************************************************************/
#include <fract_typedef.h>
#include "declaraciones.h"
#include "funciones.h"
#include <filter.h>
#include <fract2float_conv.h>
#include <math.h>
#include "HaipModulator.h"
#include "HaipCommons.h"
#include <string.h>

void mapeador(unsigned char* frame_buffer);
void muestreo(void);
void filtro(void);
void oscilador(void);
void sumador(void);
void introducirSincronizacion(void);
void crearEntradaParaDAC(void);

/*****************************************************************************

 Variables para la modulacion de las tramas

 ******************************************************************************/

//Contadores
segment ("sdram0") int indice_simbolos = 0;
segment ("sdram0") int indice_bits = 0;
segment ("sdram0") int indice_muestras = 0;
segment ("sdram0") int indice_muestrasBuenas = 0;
segment ("sdram0") int indice_retraso = 0;
segment ("sdram0") int indice_muestrasFiltradas = 0;
segment ("sdram0") int indice_muestrasModuladas = 0;
segment ("sdram0") int indice_sincronizacion = 0;
segment ("sdram0") int indice_conversor = 0;

//Arrays para cada paso

segment ("sdram0") float trama_entrada_mod_codificada[HAIP_FRAME_LENGTH_MAX];

segment ("sdram0") float simbolo_real[NUM_SIMBOLOS];
segment ("sdram0") float simbolo_imag[NUM_SIMBOLOS];

segment ("sdram0") float constelacion_real[SIMBOLOS_CONSTELACION] = { -3, -3,
		-3, -3, -1, -1, -1, -1, +3, +3, +3, +3, +1, +1, +1, +1 };
segment ("sdram0") float constelacion_imag[SIMBOLOS_CONSTELACION] = { -3, -1,
		+3, +1, -3, -1, +3, +1, -3, -1, +3, +1, -3, -1, +3, +1 };
segment ("sdram0") float simbolos_comprobacion_real[CARACTERES_PRUEBA * 2] = {
		-3, -3, -3, -3, -1, -1, -1, -1, +3, +3, +3, +3, +1, +1, +1, +1 }; //{-3 , +3 , -3 , +3};
segment ("sdram0") float simbolos_comprobacion_imag[CARACTERES_PRUEBA * 2] = {
		-3, -1, +3, +1, -3, -1, +3, +1, -3, -1, +3, +1, -3, -1, +3, +1 }; //{+3 , +3 , -3 , -3};

segment ("sdram0") float simbolo_muestreado_real[NUM_SOBREMUESTREADOS];
segment ("sdram0") float simbolo_muestreado_imag[NUM_SOBREMUESTREADOS];

//Variables para el filtro
segment ("sdram0") fir_state_fr32 state_real;
segment ("sdram0") fir_state_fr32 state_imag;
#pragma section("L1_data_b")
fract32 retraso_real[COEFICIENTES_FILTRO];
fract32 retraso_imag[COEFICIENTES_FILTRO];

segment ("sdram0") fract32 left_in_fr32_real[NUM_FILTRADOS_1];
segment ("sdram0") fract32 left_in_fr32_imag[NUM_FILTRADOS_1];

segment ("sdram0") float left_out_float_real[NUM_FILTRADOS_1];
segment ("sdram0") float left_out_float_imag[NUM_FILTRADOS_1];

segment ("sdram0") fract32 left_out_fr32_real[NUM_FILTRADOS_1];
segment ("sdram0") fract32 left_out_fr32_imag[NUM_FILTRADOS_1];
//

segment ("sdram0") float sincronizacion1_float[LONGITUD_SINCRONIZACION];
segment ("sdram0") fract32 sincronizacion1_fr32[LONGITUD_SINCRONIZACION];

segment ("sdram0") float trama_salida_mod_float[NUM_MODULADOS];
segment ("sdram0") fract32 trama_salida_mod_fr32[NUM_MODULADOS];

//

//Variables que definen el valor de la señal portadora
/*FC = 4000 Hz*/
segment ("sdram0") float seno[COEFICIENTES_PORTADORA] = { 0, 0.5, 0.866, 1,
		0.866, 0.5, 0, -0.5, -0.866, -1, -0.866, -0.5 };
segment ("sdram0") float coseno[COEFICIENTES_PORTADORA] = { 1, 0.866, 0.5, 0,
		-0.5, -0.866, -1, -0.866, -0.5, 0, 0.5, 0.866 };

/*FC = 6000 Hz
 segment ("sdram0") float seno[COEFICIENTES_PORTADORA]= {0,	0.707, 1, 0.707, 0, -0.707, -1, -0.707};
 segment ("sdram0") float coseno[COEFICIENTES_PORTADORA]={1, 0.707, 0, -0.707, -1, 0.707, 0, 0.707};
 */
/*FC = 8000 Hz
 segment ("sdram0") float seno[COEFICIENTES_PORTADORA]= {0,	0.866, 0.866, 0, -0.866, -0.866};
 segment ("sdram0") float coseno[COEFICIENTES_PORTADORA]={1, 0.5, -0.5, -1, -0.5, 0.5};
 */
//
//Variables en las que se guarda los valores trasladados a la frecuencia de la señal portadora
segment ("sdram0") float garraiatzaile_real[NUM_MODULADOS];
segment ("sdram0") float garraiatzaile_imag[NUM_MODULADOS];
//

/*****************************************************************************

 Funcioiones declaradas

 ******************************************************************************/
//--------------------------------------------------------------------------//
// Funcion:	modulador														//
//																			//
// Descripcion: Funcion general que llama a las funciones concretas  		//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void modulate_frame(unsigned char* frame_buffer, fract32* output_buffer) {
	mapeador(frame_buffer);
	muestreo();
	filtro();
	oscilador();
	sumador();
	introducirSincronizacion();
	crearEntradaParaDAC();
}

//--------------------------------------------------------------------------//
// Funcion:	mapeador														//
//																			//
// Descripcion: Funcion que mapea la señal de entrada y busca los símbolos 	//
// correspondientes de los bits en la constelacion							//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//

void mapeador(unsigned char* frame_buffer) {

	int num_decimal = 0;

	for (indice_simbolos = 0; indice_simbolos < strlen(frame_buffer) * 2;
			indice_simbolos++) {
		if (indice_simbolos % 2 == 0) {
			num_decimal = char_to_dec(frame_buffer[indice_simbolos / 2],
					NUM_BITS_POR_SIMBOLO);
		} else {
			num_decimal = char_to_dec(frame_buffer[indice_simbolos / 2] >> 4,
					NUM_BITS_POR_SIMBOLO);
		}

		simbolo_real[indice_simbolos] = constelacion_real[num_decimal];
		simbolo_imag[indice_simbolos] = constelacion_imag[num_decimal];

		num_decimal = 0;

	}

}

//--------------------------------------------------------------------------//
// Funcion:	muestreo														//
//																			//
// Descripcion: Funcion que muestrea la señal de entrada y le va añadiendo  //
// SM-1 ceros entre símbolo y símbolo										//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void muestreo() {

	indice_muestrasBuenas = 0;
	for (indice_muestras = 0; indice_muestras < NUM_SOBREMUESTREADOS;
			indice_muestras++) {

		if (indice_muestras == 0 || (indice_muestras % SM) == 0) {

			simbolo_muestreado_real[SM * indice_muestrasBuenas] =
					simbolo_real[indice_muestrasBuenas];
			simbolo_muestreado_imag[SM * indice_muestrasBuenas] =
					simbolo_imag[indice_muestrasBuenas];
			indice_muestrasBuenas++;

		} else {
			simbolo_muestreado_real[indice_muestras] = 0;
			simbolo_muestreado_imag[indice_muestras] = 0;
		}

	}
}

//--------------------------------------------------------------------------//
// Funcion:	filtro   														//
//																			//
// Descripcion: Funcion que filtra la señal de entrada para darle forma. Va //
//              haciendiendo la convolucion entre los símbolos muestreados  //
//              y los coeficientes del filtro pero también se le añade      //
//              retraso. Paraf filtrar la señal los valores tienen que ser  //
//              de tipo fract32 y es por eso por lo que se hace la          //
//              conversiónde float a fract32 y viceversa	                //
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void filtro() {

	for (indice_muestras = 0; indice_muestras < NUM_SOBREMUESTREADOS;
			indice_muestras++) {
		left_in_fr32_real[indice_muestras] = float_to_fr32(
				simbolo_muestreado_real[indice_muestras]
						/ VALOR_MAX_CONSTELACION);
		left_in_fr32_imag[indice_muestras] = float_to_fr32(
				simbolo_muestreado_imag[indice_muestras]
						/ VALOR_MAX_CONSTELACION);
	}

	for (indice_retraso = 0; indice_retraso < COEFICIENTES_FILTRO;
			indice_retraso++) {

		retraso_real[indice_retraso] = 0;
		retraso_imag[indice_retraso] = 0;
	}

	fir_init(state_real, filter_fr32, retraso_real, COEFICIENTES_FILTRO, 0);
	fir_init(state_imag, filter_fr32, retraso_imag, COEFICIENTES_FILTRO, 0);
	fir_fr32(left_in_fr32_real, left_out_fr32_real, NUM_FILTRADOS_1,
			&state_real);
	fir_fr32(left_in_fr32_imag, left_out_fr32_imag, NUM_FILTRADOS_1,
			&state_imag);

	// HAU ZETAKO??????
	for (indice_muestrasFiltradas = 0;
			indice_muestrasFiltradas < NUM_FILTRADOS_1;
			indice_muestrasFiltradas++) {

		left_out_float_real[indice_muestrasFiltradas] =
		VALOR_MAX_CONSTELACION
				* fr32_to_float(left_out_fr32_real[indice_muestrasFiltradas]);
		left_out_float_imag[indice_muestrasFiltradas] =
		VALOR_MAX_CONSTELACION
				* fr32_to_float(left_out_fr32_imag[indice_muestrasFiltradas]);
	}
}

//--------------------------------------------------------------------------//
// Funcion:	oscilador														//
//																			//
// Descripcion: Funcion que traslada la señal filtrada a la frecuencia de   //
// la señal portadora (8kHz)											        //
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//
void oscilador() {

	for (indice_muestrasFiltradas = 0;
			indice_muestrasFiltradas < NUM_FILTRADOS_1;
			indice_muestrasFiltradas++) {

		garraiatzaile_real[indice_muestrasFiltradas] =
				left_out_float_real[indice_muestrasFiltradas]
						* coseno[(indice_muestrasFiltradas
								% COEFICIENTES_PORTADORA)];
		garraiatzaile_imag[indice_muestrasFiltradas] =
				left_out_float_imag[indice_muestrasFiltradas]
						* seno[(indice_muestrasFiltradas
								% COEFICIENTES_PORTADORA)];
		trama_salida_mod_float[indice_muestrasModuladas] =
				(garraiatzaile_real[indice_muestrasModuladas]
						- garraiatzaile_imag[indice_muestrasModuladas]);
	}

}

//--------------------------------------------------------------------------//
// Funcion:	sumador														    //
//																			//
// Descripcion: Funcion que suma (en realidad resta) la parte real y la     //
//              imaginaria y que convierte la señal final float a fract32   //
//				para que se envie al dac                                    //
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//

void sumador() {

	//Sumar
	//AURREKO FOR-ean INEZ GERO SOBRAN EGONGO ZAN
	for (indice_muestrasModuladas = 0; indice_muestrasModuladas < NUM_MODULADOS;
			indice_muestrasModuladas++) {

		trama_salida_mod_float[indice_muestrasModuladas] =
				(garraiatzaile_real[indice_muestrasModuladas]
						- garraiatzaile_imag[indice_muestrasModuladas]);
	}

	//Convertir de float a fract32 para que se envie al dac
	//REGULADOR DE POTENCIA?????
	for (indice_muestrasModuladas = 0; indice_muestrasModuladas < NUM_MODULADOS;
			indice_muestrasModuladas++) {

		trama_salida_mod_fr32[indice_muestrasModuladas] = REGULADOR_POTENCIA
				* float_to_fr32(
						trama_salida_mod_float[indice_muestrasModuladas]
								/ (VALOR_MAX_CONSTELACION));

	}

}

//--------------------------------------------------------------------------//
// Funcion:	Introducir sincronización									    //
//																			//
// Descripcion: Funcion que crea la señal de sincronización para que sea    //
//			    enviada		                                                //
//                                                                          //
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//

void introducirSincronizacion() {

	int contador_picos = 0;

	for (indice_muestras = 0; indice_muestras < LONGITUD_SINCRONIZACION;) {

		if (contador_picos % (SM) == 0) {

			for (indice_sincronizacion = 0;
					indice_sincronizacion < LONGITUD_PICO;
					indice_sincronizacion++) {

				sincronizacion1_float[indice_muestras] = VALOR_PICO;
				indice_muestras++;
			}

		} else {

			sincronizacion1_float[indice_muestras] = 0;
			indice_muestras++;
		}

		contador_picos++;

	}

	for (indice_sincronizacion = 0;
			indice_sincronizacion < LONGITUD_SINCRONIZACION;
			indice_sincronizacion++) {

		sincronizacion1_fr32[indice_sincronizacion] = float_to_fr32(
				sincronizacion1_float[indice_sincronizacion] / VALOR_PICO);

	}

}

//--------------------------------------------------------------------------//
// Funcion:	Crear señal para el DAC										    //
//																			//
// Descripcion: Funcion que convierte los datos a enviar (de float a int)   //
// para que puedan pasar por el conversor									//
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//

void crearEntradaParaDAC() {

	for (indice_conversor = 0; indice_conversor < LONGITUD_DAC;
			indice_conversor += 2) {

		entrada_dac[indice_conversor] = sincronizacion1_fr32[indice_conversor
				/ 2]; //Derecha (Sincronización)

		if (indice_conversor < NUM_MODULADOS * 2) {
			entrada_dac[indice_conversor + 1] =
					trama_salida_mod_fr32[indice_conversor / 2]; //Izquierda (Datos)
		}

	}

}
