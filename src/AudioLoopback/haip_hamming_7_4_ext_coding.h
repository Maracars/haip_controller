/*
 * HaipHamming_7_4_coding.h
 *
 *  Created on: 12 Dec 2017
 *      Author: ander
 */

#ifndef HAIPHAMMING_7_4_CODING_H_
#define HAIPHAMMING_7_4_CODING_H_

void haip_hamming_7_4_ext_code(const unsigned char *input_bytes, unsigned char *output_bytes, int len);
int haip_hamming_7_4_ext_decode(const unsigned char *input_bytes, unsigned char *output_bytes, int len);

#endif /* HAIPHAMMING_7_4_CODING_H_ */
