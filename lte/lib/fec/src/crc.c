/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2014 The libLTE Developers. See the
 * COPYRIGHT file at the top-level directory of this distribution.
 *
 * \section LICENSE
 *
 * This file is part of the libLTE library.
 *
 * libLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lte/utils/pack.h"

#include "lte/fec/crc.h"


void gen_crc_table(crc_t *crc_params) {

	int i, j, ord=(crc_params->order-8);
	unsigned long bit, crc;

	for (i=0; i<256; i++) {
		crc = ((unsigned long)i)<<ord;
		for (j=0; j<8; j++) {
			bit = crc & crc_params->crchighbit;
			crc<<= 1;
			if (bit) crc^= crc_params->polynom;
		}			
		crc_params->table[i]=crc & crc_params->crcmask;
	}
}


unsigned long  crctable (unsigned long length, crc_t *crc_params) {

	// Polynom order 8, 16, 24 or 32 only.
	int ord=crc_params->order-8;
	unsigned long crc = crc_params->crcinit;
	unsigned char* data = crc_params->data0;

	while (length--){
		 crc = (crc << 8) ^ crc_params->table[ ((crc >> (ord)) & 0xff) ^ *data++];
	}
	return((crc ^ crc_params->crcxor) & crc_params->crcmask);
}

unsigned long reversecrcbit(unsigned int crc, int nbits, crc_t *crc_params) {

	unsigned long m, rmask=0x1;

	for(m=0; m<nbits; m++){	
		if((rmask & crc) == 0x01 )crc = (crc ^ crc_params->polynom)>>1;
		else crc = crc >> 1;
	}
	return((crc ^ crc_params->crcxor) & crc_params->crcmask);
}

int crc_set_init(crc_t *crc_par, unsigned long crc_init_value){
	
	crc_par->crcinit=crc_init_value; 
	if (crc_par->crcinit != (crc_par->crcinit & crc_par->crcmask)) {
		printf("ERROR, invalid crcinit in crc_set_init().\n");
		return(-1);
	}
	return(0);
}
int crc_set_xor(crc_t *crc_par, unsigned long crc_xor_value){
	
	crc_par->crcxor=crc_xor_value;
	if (crc_par->crcxor != (crc_par->crcxor & crc_par->crcmask)) {
		printf("ERROR, invalid crcxor in crc_set_xor().\n");
		return(-1);
	}
	return(0);
}

int crc_init(crc_t *crc_par, unsigned int crc_poly, int crc_order){

	// Set crc working default parameters 	
	crc_par->polynom=crc_poly;
	crc_par->order=crc_order;
	crc_par->crcinit=0x00000000; 
	crc_par->crcxor=0x00000000;

	// Compute bit masks for whole CRC and CRC high bit
	crc_par->crcmask = ((((unsigned long)1<<(crc_par->order-1))-1)<<1)|1;
	crc_par->crchighbit = (unsigned long)1<<(crc_par->order-1);

	// check parameters
	if (crc_par->order%8 != 0) {
		printf("ERROR, invalid order=%d, it must be 8, 16, 24 or 32.\n", crc_par->order);
		return(-1);
	}
	if(crc_set_init( crc_par, crc_par->crcxor))return(-1);
	if(crc_set_xor( crc_par, crc_par->crcxor))return(-1);

	// generate lookup table
	gen_crc_table(crc_par);

	// Alloocate memory
	crc_par->data0 = (unsigned char *)malloc(sizeof(*crc_par->data0) * (MAX_LENGTH+crc_par->order));
	if (!crc_par->data0) {
		perror("malloc ERROR: Allocating memory for data pointer in crc() function");
		return(-1);
	}

	return(0);
}

void crc_free(crc_t *crc_p){
	free(crc_p->data0);
	crc_p->data0=NULL;
}

//ELIMINATE
unsigned int crc(unsigned int crc, char *bufptr, int len,
		int long_crc, unsigned int poly, int paste_word){

	return(1);
};
/////


unsigned int crc_attach(char *bufptr, int len, crc_t *crc_params) {

	int i, len8, res8, a=0;
	unsigned int crc;
	char *pter;

	if(len > MAX_LENGTH){
		perror("Data lenght ERROR: Input data lenght exceeds available memory (MAX_LENGTH)");
		return(-1);
	}

	//# Pack bits into bytes 
	len8=(len>>3);
	res8=(len - (len8<<3));
	if(res8>0)a=1;

	// Move to char format
	for(i=0; i<len8; i++){
		pter=(char *)(bufptr+8*i);
		crc_params->data0[i]=(unsigned char)(unpack_bits(&pter, 8)&0xFF);
	}
	crc_params->data0[len8]=0x00;
	for(i=0; i<res8; i++){
		crc_params->data0[len8] |= ((unsigned char)*(pter+i))<<(7-i);
	}

	// Calculate CRC
	crc=crctable(len8+a, crc_params);

	// Reverse CRC res8 positions
	if(a==1)crc=reversecrcbit(crc, 8-res8, crc_params);

	// Add CRC
	pter=(char *)(bufptr+len);
	pack_bits(crc, &pter, crc_params->order);

	//Return CRC value
	return crc;
}
