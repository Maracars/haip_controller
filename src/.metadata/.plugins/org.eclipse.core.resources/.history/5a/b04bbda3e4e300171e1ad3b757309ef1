#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "declaraciones.h"
#include "funciones.h"

void char_to_bits(void);
void desordenar(void);

section ("sdram0") int trama_entrada_mod_bits[NUM_BITS];
section ("sdram0") int c1_c[7]; //Primer Codeword ordenado. 7 son el número de bits de cada grupo después de codificar, es decir, a cada grupo de 4 bits se le añaden 3, consiguiendo grupos de 7 bits
section ("sdram0") int c2_c[7]; //Segundo Codeword ordenado
section ("sdram0") int a_c[7]; //Primer Codeword desordenado
section ("sdram0") int b_c[7]; //Segundo Codeword desordenado



//--------------------------------------------------------------------------//
// Funcion:	codificador														//
//																			//
// Descripcion: Funcion que codifica la señal de entrada,le da valores a 	//
// las tramas de entrada que luego seran comprobadas para detectar y		//
// corregir posibles errores  											    //
//																			//
// Parametros de entrada: Ninguno											//
//																			//
// Parametros de salida: Ninguno											//
//																			//
//--------------------------------------------------------------------------//

void codificador(){

	int z=0,p=0,j=0,n,k,col,fila,h,i;
	int aux1[4]; // Primer grupo ordenado
	int aux2[4]; //Segundo grupo ordenado
	int g[4][7]= {{1, 1, 1, 0, 0, 0, 0},{1, 0, 0, 1, 1, 0, 0},{0, 1, 0, 1, 0, 1, 0},{1, 1, 0, 1, 0, 0, 1}};

	char_to_bits();

	for(n=0;n<NUM_BITS;n=n+8){
		
		for(j=0;j<7;j++){
			c1_c[j]=0;
			c2_c[j]=0;
		}

		for(k=0;k<4;k++){
			aux1[k]=trama_entrada_mod_bits[z];
			aux2[k] = trama_entrada_mod_bits[z+4];
			z++;
		}

		z = z + 4;

		for(col=0;col<7;col++){
			for(fila=0;fila<4;fila++){
				c1_c[col]+=aux1[fila]*g[fila][col];
			}
			c1_c[col]=c1_c[col]%2;
		}

		for(col=0;col<7;col++){
			for(fila=0;fila<4;fila++){
				c2_c[col]+=aux2[fila]*g[fila][col];
			}
			c2_c[col]=c2_c[col]%2;
		}

		desordenar();

		for(h=0;h<7;h++){
			trama_entrada_mod_codificada[p]=a_c[h];
			p++;
		}
		for(h=0;h<7;h++){
			trama_entrada_mod_codificada[p]=b_c[h];
			p++;
		}
	}
}

void char_to_bits(){

	int i=0;
	int j = 0;
	unsigned char aux;

	for(j=0 ; j<BUFFER_SIZE ; j++){

		for(i=7 ; i>=0 ; i--){

			aux = (unsigned char) pow(2 , i);
			trama_entrada_mod_bits [(j * 8) + (7 - i)] = (aux & trama_entrada_mod[j]) >> i;

		}
	}

}

void desordenar(void){

	int i=0 , j=0 , z=0;
	bool primerCodeword = true, segundoCodeword = false;

	for(j=0 ; j<7 ; j++){

		if(segundoCodeword){

			b_c[z] = c2_c[j];
			b_c[z-1] = c1_c[j];
		}

		if(primerCodeword){
			a_c[z] = c1_c[j];
			if(j == 3){
				z = 0;
				b_c[z] = c2_c[j];
				primerCodeword = false;
				segundoCodeword = true;
			}else{
				a_c[z+1] = c2_c[j];
			}
		}

		z += 2;
		i += 2;


	}

}
