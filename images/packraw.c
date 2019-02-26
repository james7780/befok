/* pack 8-bit RAW image data into 4-bit format */
/*
	JH 2001
	Takes 256-colour RAW image from PSP and converts it into 16-colour (4-bit) format
	for use on Lynx

	Usage: packraw <file_in> <file_out>
*/

#include <stdio.h>
#include <stdlib.h>

#ifndef uchar
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;
#endif

int main(int argc, char *argv[]) {

	FILE *pf;
	int len, len_out, i;
	uchar *addr, *p1;
	uchar a, b;

	pf = fopen(argv[1],"rb");
	if (pf == NULL) {
		printf("Error opening file!\n");
    	return -1;
	}

	// find length of file
	fseek(pf, 0L, SEEK_END);
	len = ftell(pf);
	fseek(pf,0L,SEEK_SET);
	printf("File length: %d bytes\n", len);

	// allocate memory to load file into
	addr = (uchar *)malloc(len);
	if (addr == NULL) {
		printf("Error: could not allocate memory to load file into!\n");
    	fclose(pf);
    	return -1;
	}

	// read file in 2 bytes at a time, compressing to 1 byte in allocated memory
	p1 = addr;
	for(i=0; i<len; i+=2) {
		a = fgetc(pf); b = fgetc(pf);
		*p1++ = (a << 4) | b;
	}

	// close file
  	fclose(pf);

	// open new file to write out compressed image
	if ( (pf = fopen(argv[2],"wb")) == NULL) {
		printf("Error opening file for writing!\n");
		free(addr);
    	return -1;
	}

	// write new file
	len_out = fwrite(addr, sizeof(uchar), len/2, pf);
	printf("4-bit packed file %s written, %d bytes.\n", argv[2], len_out);
	fclose(pf);

	free(addr);

	return len_out;
}
