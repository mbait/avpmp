#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "data.h"

int8_t ReadInt8(FILE *fp)
{
	unsigned char d[1];
	
	fread(d, 1, 1, fp);
	
	return d[0];
}

int16_t ReadInt16M(FILE *fp)
{
	unsigned char d[2];
	
	fread(d, 1, 2, fp);
	
	return d[1] | (d[0] << 8);
}

int32_t ReadInt32M(FILE *fp)
{
	unsigned char d[4];
	
	fread(d, 1, 4, fp);
	
	return d[3] | (d[2] << 8) | (d[1] << 16) | (d[0] << 24);
}

#define IFF_UNKNOWN	-1
#define IFF_LIST	0
#define IFF_FORM	1
#define IFF_TYPE_ILBM	2
#define IFF_PROP	3
#define IFF_TRAN	4
#define IFF_BMHD	6
#define IFF_CMAP	7
#define IFF_BODY	8
#define IFF_TYPE_MIPM	9
#define IFF_CONT	10
#define IFF_TYPE_PBM	11
#define IFF_DPPS	12
#define IFF_CRNG	13
#define IFF_TINY	14

int main(int argc, char *argv[])
{
	FILE *fp;
	char tag[4];
	int c;
	unsigned char *cmap;
	int w, h;
	int x, y;
	int planes;
	int masking;
	int compression;
	int tcolor;
	int xa, ya;
	int pw, ph;
	
	if (argc != 2) {
		fprintf(stderr, "usage: %s <file.lbm>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		fprintf(stderr, "unable to open %s for reading\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
	printf("Filename: %s\n", argv[1]);
	
	/* printf("This program is lame. You have been warned.\n"); */
	
	fseek(fp, 48, SEEK_SET);
	
	fread(tag, 1, 4, fp);
	if (tag[0] != 'I' || tag[1] != 'L' || tag[2] != 'B' || tag[3] != 'M') {
		fprintf(stderr, "This program is lame! Give me a file in the right format please!\n");
		exit(EXIT_FAILURE);
	}
	fread(tag, 1, 4, fp);
	if (tag[0] != 'B' || tag[1] != 'M' || tag[2] != 'H' || tag[3] != 'D') {
		fprintf(stderr, "This program is lame! Give me a file in the right format please!\n");
		exit(EXIT_FAILURE);
	}
	
	if (ReadInt32M(fp) != 0x14) {
		fprintf(stderr, "This program is lame! Give me a file in the right format please!\n");
		exit(EXIT_FAILURE);
	}
{
	/* 'BMHD' */

	
	w = ReadInt16M(fp);
	h = ReadInt16M(fp);
	x = ReadInt16M(fp);
	y = ReadInt16M(fp);
	
	planes = ReadInt8(fp);
	masking = ReadInt8(fp);
	compression = ReadInt8(fp);
	
	ReadInt8(fp); /* padding */
	
	tcolor = ReadInt16M(fp);
	xa = ReadInt8(fp);
	ya = ReadInt8(fp);
	pw = ReadInt16M(fp);
	ph = ReadInt16M(fp);
	
	printf("Info: w:%d h:%d x:%d y:%d p:%d m:%02X c:%d tc:%d xa:%d ya:%d pw:%d ph:%d\n", w, h, x, y, planes, masking, compression, tcolor, xa, ya, pw, ph);
	
}	
	fread(tag, 1, 4, fp);
	if (tag[0] != 'C' || tag[1] != 'M' || tag[2] != 'A' || tag[3] != 'P') {
		fprintf(stderr, "How could you betray me, this is crap!\n");
		exit(EXIT_FAILURE);
	}
	
	c = ReadInt32M(fp);
	
	printf("Colors present = %d (%d)\n", c / 3, c);
	if (c % 3) {
		fprintf(stderr, "Gee, your colormap is messed up!\n");
		exit(EXIT_FAILURE);
	}

	printf("Offset = %d\n", ftell(fp));
	
	cmap = (unsigned char *)malloc(c);
	fread(cmap, 1, c, fp);
	
	if (c & 1)
		fgetc(fp); /* throwaway extra byte (even padding) */
		
	printf("Offset = %d\n", ftell(fp));
	fread(tag, 1, 4, fp);
	printf("Offset = %d\n", ftell(fp));
	if (tag[0] != 'B' || tag[1] != 'O' || tag[2] != 'D' || tag[3] != 'Y') {
		printf("Where's the body tag!?\n");
		exit(EXIT_FAILURE);
	}
	
{
	unsigned char *compbuf;
	unsigned char *grphbuf;
	int len, len2;
	int rw;
	int i, j, p, pixel;
	
	len2 = ReadInt32M(fp);

	printf("Pos 1: pos:%d len:%d\n", ftell(fp), len2);
	
	rw = ((w + 7) / 8) * 8; /* round up to multiple of 8 */
	len = rw * h * planes / 8;
	grphbuf = (unsigned char *)malloc(w * h * 3);
	compbuf = (unsigned char *)malloc(len);
	
	if (compression == 1) {
		j = 0;
		
		while (len2 > 0) {
			signed char s = fgetc(fp);
			len2--;
			
			if (s >= 0) {
				i = s + 1;
				
				fread(&compbuf[j], 1, i, fp);
				j += i;
				len2 -= i;
			} else {
				if (s == -128) printf("Nop?\n");
				
				i = -s+1;
				
				p = fgetc(fp);
				len2--;
				
				while (i--) {
					compbuf[j] = p;
					j++;
				}
			}
		}
		if (len2 < 0) printf("somebody set up us the messed up decompression: %d\n", len2);
		
	} else if (compression == 0) {
		fread(compbuf, 1, len, fp);
		
		for (j = 0; j < h; j++) {
			for (i = 0; i < w; i++) {
				for (pixel = 0, p = 0; p < planes; p++) {
					if (compbuf[(rw/8)*p+(rw*planes/8)*j+i/8] & (1 << (7 - (i & 7))))
						pixel |= (1 << p);
				}
				grphbuf[(w*j+i)*3+0] = cmap[3*pixel+0];
				grphbuf[(w*j+i)*3+1] = cmap[3*pixel+1];
				grphbuf[(w*j+i)*3+2] = cmap[3*pixel+2];
			}
		}
		
	} else {
		fprintf(stderr, "What kinda compression is this?!\n");
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			for (pixel = 0, p = 0; p < planes; p++) {
				if (compbuf[(rw/8)*p+(rw*planes/8)*j+i/8] & (1 << (7 - (i & 7))))
					pixel |= (1 << p);
			}
			grphbuf[(w*j+i)*3+0] = cmap[3*pixel+0];
			grphbuf[(w*j+i)*3+1] = cmap[3*pixel+1];
			grphbuf[(w*j+i)*3+2] = cmap[3*pixel+2];
		}
	}

	if (ftell(fp) & 1) fgetc(fp); 
	
	printf("Pos 2: pos:%d\n", ftell(fp));	
	{
		char *fn;
		
		fn = (char *)malloc(strlen(argv[1])+6);
		strcpy(fn, argv[1]);
		strcat(fn, ".pcx");
		SavePCXRGBToFile(grphbuf, w, h, fn);
		free(fn);
	}
	
	p = fgetc(fp);
	if (p != -1) {
		printf("%c", p);
		printf("%c", fgetc(fp));
		printf("%c", fgetc(fp));
		printf("%c", fgetc(fp));
		printf("\n");
	}
	
	free(compbuf);
	free(grphbuf);	
}
	
	free(cmap);
	fclose(fp);
	
	return 0;
}
