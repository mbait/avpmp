#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util.h"
 
int main(int argc, char *argv[])
{
	FILE *fp;
	unsigned char id[4];
	char cdir[560];
	int nfiles;
	int off;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s <file.ffl>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		fprintf(stderr, "unable to open %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	fread(id, 1, 4, fp);
	if ((id[0] != 'R') || (id[1] != 'F') | (id[2] != 'F') | (id[3] != 'L')) {
		fprintf(stderr, "%s is not a valid RFFL file\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	if (ReadInt32L(fp) != 0x0000) {
		fprintf(stderr, "%s is not a version 0000 RFFL file\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	nfiles = ReadInt32L(fp);
	
	off = ReadInt32L(fp) + 20;
	ReadInt32L(fp);
	
	//if (getcwd(cdir, sizeof(cdir)) == NULL) {
	//	fprintf(stderr, "your cwd is too long...\n");
	//	exit(EXIT_FAILURE);
	//}
		
	while (nfiles--) {
		FILE *ofp;
		char name[512], *nptr;
		int i, j;
		int c;
		
		int offset = ReadInt32L(fp);
		int length = ReadInt32L(fp);
		int pffset;
		
		j = 0;
		do {
			for (i = 0; i < 4; i++, j++) {
				c = fgetc(fp);
				name[j] = c;
				
				if (c == 0) break;
			}
			
		} while (c != 0);
		for (; i < 3; i++) fgetc(fp);
		
		printf("Filename: %s (%d, %d)\n", name, offset, length);
		
		pffset = ftell(fp);
		
		for (j = 0, nptr = &name[0]; name[j]; j++)
			if (name[j] == '\\')
				nptr = &name[j+1];
		
		fseek(fp, off+offset, SEEK_SET);
		
		ofp = fopen(nptr, "wb");
		
		for (i = 0; i < length; i++)
			fputc(fgetc(fp), ofp);
		
		fclose(ofp);
		
		fseek(fp, pffset, SEEK_SET);
		
	}
	
	fclose(fp);
	
	return 0;
}
