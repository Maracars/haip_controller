#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "declaraciones.h"
#include "funciones.h"

void bits_to_char(void);
void ordenar(void);
void corregir(int c[][] , int transpose_c[][]);

segment ("sdram0") int trama_salida_demod_decodificada[NUM_BITS];
segment ("sdram0") int c1[1][7] = {{0,0,0,0,0,0,0}}; //Primer Codeword ordenado. 7 son el número de bits de cada grupo después de codificar, es decir, a cada grupo de 4 bits se le añaden 3, consiguiendo grupos de 7 bits
segment ("sdram0") int c2[1][7] = {{0,0,0,0,0,0,0}}; //Segundo Codeword ordenado
segment ("sdram0") int a[1][7] = {{0,0,0,0,0,0,0}}; //Primer Codeword desordenado
segment ("sdram0") int b[1][7] = {{0,0,0,0,0,0,0}}; //Segundo Codeword desordenado
segment ("sdram0") int g = 0;

void decodificador(){

	int n=0,k=0,m=0,j=0,s=0,x=0,z=0,h=0,p=0,f=0 ,q=0,columna;
	g = 0;

	int transpose_c1[7][1]={{0},{0},{0},{0},{0},{0},{0}};
	int transpose_c2[7][1]={{0},{0},{0},{0},{0},{0},{0}};
	
	for (n=0;n<NUM_CODIFICADOS;n=n+7*2){
		columna=0;

		for(k=0;k<LONGITUD_RECEPTOR;k++){
			c1[columna][k]=0;
			c2[columna][k]=0;
			a[columna][k]=0;
			b[columna][k]=0;
		}

		for (k=0;k<LONGITUD_RECEPTOR;k++){
            a[p][k]=trama_salida_demod_codificada[m];
            b[p][k]=trama_salida_demod_codificada[m+LONGITUD_RECEPTOR];
			m++;
		}	

		m = m + LONGITUD_RECEPTOR;

		ordenar();

		for(h=0;h<LONGITUD_RECEPTOR;h++){
			transpose_c1[h][p]=c1[p][h];
			transpose_c2[h][p]=c2[p][h];
		}

		corregir(c1 , transpose_c1);
		corregir(c2 , transpose_c2);

	}

	bits_to_char();

}

void ordenar(){

	int i=0 , j=0 , z=0 , p = 0;
	bool primerCodeword = true, segundoCodeword = false;

	for(j=0 ; j<7 ; j++){

		if(primerCodeword){
			c1[p][j] = a[p][z];
			if(j == 3){
				z = 0;
				primerCodeword = false;
				segundoCodeword = true;
			}else{
				c2[p][j] =  a[p][z+1];
			}
		}

		if(segundoCodeword){

			c2[p][j] = b[p][z];
			if(j == 6){
				break;
			}
			c1[p][j+1] = b[p][z+1];
		}

		z += 2;
		i += 2;


	}

}

void corregir(int c[1][7] , int transpose_c[7][1]){

	int i , j , h , s , x , z , p = 0 , q;

	int b_prima[4]={0, 0, 0, 0};

	int H[3][7]={{0, 0, 0, 1, 1, 1, 1},{0, 1, 1, 0, 0, 1, 1},{1, 0, 1, 0, 1, 0, 1}};
	int R[4][7]={{0, 0, 1, 0, 0, 0, 0},{0, 0, 0, 0, 1, 0, 0 },{0, 0, 0, 0, 0, 1, 0},{0, 0, 0, 0, 0, 0, 1}};

	int sindrome[3][1]={{0},{0},{0}};

	for (i=0;i<7;i++){
		for( j=0;j<3;j++){
			sindrome[j][p] = sindrome[j][p]+H[j][i]*transpose_c[i][p];
			sindrome[j][p] = sindrome[j][p]%2;
		}
	}

	s=0;
	for (i=0;i<3;i++){
		s=s+sindrome[i][p]*pow(2,(3-(i+1)));
	}

	for (x=1;x<=7;x++){
		if (x==s){
			if (c[p][x-1]==0){
			  c[p][x-1]=1;
			}else{
			  c[p][x-1]=0;
			}
		}
	}

	for(h=0;h<7;h++){
		transpose_c[h][p]=c[p][h];
	}

	for(z=0;z<7;z++){
		for(q=0;q<4;q++){
			b_prima[q]= b_prima[q]+R[q][z]*transpose_c[z][p];
		}
	}

	for (h=0;h<4;h++){
		trama_salida_demod_decodificada[g]=b_prima[h];
		g++;
	}
}

void bits_to_char(){

	int num_decimal = 0;
	int indice_caracteres = 0;
	int aux = 0;

		for (indice_caracteres=0 ; indice_caracteres<BUFFER_SIZE ; indice_caracteres++){

			for (indice_bits=0 ; indice_bits<8 ; indice_bits++){

				aux = pow(2 , indice_bits);
				num_decimal = num_decimal + aux * trama_salida_demod_decodificada[(indice_caracteres+1)*8 - (indice_bits + 1)];

			}

			trama_salida_demod[indice_caracteres] = num_decimal;


			num_decimal = 0;

		}

}
