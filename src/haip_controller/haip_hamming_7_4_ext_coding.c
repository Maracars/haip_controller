/*
 * HaipHamming_7_4_coding.c
 *
 *  Created on: 12 Dec 2017
 *      Author: ander
 */
#include "haip_hamming_7_4_ext_coding.h"

segment ("sdram0") static char G[4][7] = { { 1, 1, 1, 0, 0, 0, 0 }, { 1, 0, 0,
		1, 1, 0, 0 }, { 0, 1, 0, 1, 0, 1, 0 }, { 1, 1, 0, 1, 0, 0, 1 } };

segment ("sdram0") static char H[3][7] = { { 0, 0, 0, 1, 1, 1, 1 }, { 0, 1, 1,
		0, 0, 1, 1 }, { 1, 0, 1, 0, 1, 0, 1 } };

segment ("sdram0") static char R[4][7] = { { 0, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0,
		0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 1 } };

/* gets the 7/4 hamming codeword for 1 symbol */
unsigned char code_hamming(unsigned char b);

/* Calculates the parity of the first 7 bits */
unsigned char calc_parity(unsigned char _7bit);

/* Calculates the Extended 7/4 hamming codeword of 1 symbol */
unsigned char code_ext_hamming(unsigned char _4bit);

/* Checks whether the parity of the codeword is correct */
int check_parity(unsigned char b);

/* Checks if the hamming 7/4 codeword is correct, if not performs correction (single bit tolerance)
 * Returns whether an error was detected*/
int check_and_correct_hamming(unsigned char b, unsigned char *string);

/* Decodes the given codeword  */
unsigned char decode_hamming(unsigned char b);

void haip_hamming_7_4_ext_code(const unsigned char *input_bytes,
		unsigned char *output_bytes, int len) {
	int i = 0;
	unsigned char test;
	for (i = 0; i < len; i++) {
		if (i % 2)
			output_bytes[i] = code_ext_hamming(
					input_bytes[i / 2] & (unsigned char) 0x0F);
		else
			output_bytes[i] = code_ext_hamming(
					(input_bytes[i / 2] & (unsigned char) 0xF0) >> 4);

		test = output_bytes[i / 2];
	}
}

int haip_hamming_7_4_ext_decode(const unsigned char *input_bytes,
		unsigned char *output_bytes, int len) {

	int i = 0;
	int ret = 0;
	unsigned char corrected, decoded;
	unsigned char temp = 0;
	for (; i < len; i++) {
		int hamm_not_ok = check_and_correct_hamming(input_bytes[i], &corrected);
		if (hamm_not_ok) {
			int parity_ok = check_parity(corrected);
			if (!parity_ok)
				ret++;
		}
		decoded = decode_hamming(corrected);
		temp = temp | (decoded << 4 * (i % 2 == 1 ? 0 : 1));
		if (i % 2){
			output_bytes[i / 2] = temp;
			temp = 0;
		}

	}
	return ret;
}

unsigned char decode_hamming(unsigned char b) {
	int i = 0;
	int j = 0;
	unsigned char aux = 0;
	unsigned char ret = 0;
	for (; j < 4; j++) {
		aux = 0;
		for (i = 0; i < 7; i++) {
			aux = aux + R[j][i] * ((b >> i) & (unsigned char) 0x01);
		}
		ret = (unsigned char) (ret | ((aux % 2) << j));
	}
	return ret;
}

int check_and_correct_hamming(const unsigned char b, unsigned char *corrected) {
	int i = 0;
	int j = 0;
	unsigned char aux = 0;
	unsigned char ret = 0;
	for (; j < 3; j++) {
		aux = 0;
		for (i = 0; i < 7; i++) {
			aux = aux + H[j][i] * ((b >> i) & (unsigned char) 0x01);
		}
		ret = (unsigned char) (ret | ((aux % 2) << j));
	}
	if (!ret) {
		*corrected = b;
	} else {
		int z = 0;
		for (; z < 7; z++) {
			if ((ret & 0x01) == H[0][z] && ((ret >> 1) & 0x01) == H[1][z]
					&& ((ret >> 2) & 0x01) == H[2][z])
				break;
		}
		*corrected = b ^ (((unsigned char) 0x01) << z);
	}
	return ret;
}

int check_parity(unsigned char b) {
	return ((unsigned char) (0x80 & b)) >> 7
			== calc_parity((unsigned char) (0x7F & b));
}

unsigned char code_hamming(unsigned char b) {
	int i = 0;
	int j = 0;
	unsigned char aux = 0;
	unsigned char ret = 0;
	for (; j < 7; j++) {
		aux = 0;
		for (i = 0; i < 4; i++) {
			aux = aux + G[i][j] * ((b >> i) & (unsigned char) 0x01);
		}
		ret = (unsigned char) (ret | ((aux % 2) << j));
	}
	return ret;
}

unsigned char calc_parity(unsigned char _7bit) {
	unsigned char aux = 0, i = 0;

	for (; i < 7; i++) {
		aux = (unsigned char) (aux + (_7bit >> i) % 2);
	}
	return (unsigned char) (aux % 2);
}

unsigned char code_ext_hamming(unsigned char _4bit) {
	unsigned char result = code_hamming(_4bit);
	result = result | (calc_parity(result) << 7);
	return result;
}
