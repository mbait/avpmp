#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.hpp"

#include "util.h"

int main(int argc, char *argv[])
{
	FILE *fp;
	char str[8];
	unsigned char *buf, *dbuf;
	int len;
	
	if (argc != 3) {
		fprintf(stderr, "usage: %s <compressed.rif> <outfile>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	
	fread(str, 1, 8, fp);
	if (strncmp(str, COMPRESSED_RIF_IDENTIFIER, 8) != 0) {
		fprintf(stderr, "%s: invalid compressed rif file\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	len = fsize(argv[1]);
	buf = (unsigned char *)malloc(len);
	if (buf == NULL) {
		fprintf(stderr, "could not allocate %d bytes to load %s\n", len, argv[1]);
		exit(EXIT_FAILURE);
	}
	
	fseek(fp, 0, SEEK_SET);
	
	fread(buf, 1, len, fp);
	fclose(fp);
	
	len = ((HuffmanPackage *)buf)->UncompressedDataSize;
	
	dbuf = (unsigned char *)HuffmanDecompress((HuffmanPackage *)buf);
	if (dbuf == NULL) {
		fprintf(stderr, "Something went wrong with HuffmanDecompress\n");
		exit(EXIT_FAILURE);
	}
	
	free(buf);
	
	fp = fopen(argv[2], "wb");
	if (fp == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	
	fwrite(dbuf, 1, len, fp);
	fclose(fp);
	
	free(dbuf);
	
	return 0;
}
