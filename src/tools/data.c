#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "data.h"

unsigned char *Pal_3_4(unsigned char *dat)
{
	unsigned char *buf;
	int i;

	buf = (unsigned char *)malloc(256 * 4);

	for (i = 0; i < 256; i++) {
		buf[i*4+0] = dat[i*3+0];
		buf[i*4+1] = dat[i*3+1];
		buf[i*4+2] = dat[i*3+2];
		buf[i*4+3] = 0xFF;
	}
	buf[3] = 0;

	free(dat);

	return buf;
}

unsigned char *Pixel_BGR_RGB(unsigned char *data, int length)
{
	int x;
	unsigned char *ptr;
	unsigned char tmp;
	
	if (length % 3)
		return data;
		
	length /= 3;
	
	ptr = data;
	for (x = 0; x < length; x++) {
		tmp = *ptr;
		*ptr = *(ptr + 2);
		*(ptr + 2) = tmp;
		ptr += 3;
	}
	
	return data;
}

unsigned char *Pixel_BGRA_RGBA(unsigned char *data, int length)
{
	int x;
	unsigned char *ptr;
	unsigned char tmp;
	
	if (length & 3)
		return data;
		
	length /= 4;
	
	ptr = data;
	for (x = 0; x < length; x++) {
		tmp = *ptr;
		*ptr = *(ptr + 2);
		*(ptr + 2) = tmp;
		ptr += 4;
	}
	
	return data;
}

unsigned char *Pixel_256_RGB(unsigned char *data, unsigned char *pal, int length)
{
	unsigned char *buf;
	int x;
		
	buf = (unsigned char *)malloc(length * 3);
	
	for (x = 0; x < length; x++) {
		buf[x*3+0] = pal[data[x]*3+0];
		buf[x*3+1] = pal[data[x]*3+1];
		buf[x*3+2] = pal[data[x]*3+2];	
	}
	
	return buf;
}

unsigned char *Pixel_256_RGBA(unsigned char *data, unsigned char *pal, int length, int trans)
{
	unsigned char *buf;
	int x;
		
	buf = (unsigned char *)malloc(length * 4);
	
	for (x = 0; x < length; x++) {
		buf[x*4+0] = pal[data[x]*3+0];
		buf[x*4+1] = pal[data[x]*3+1];
		buf[x*4+2] = pal[data[x]*3+2];	
		if (data[x] == trans)
			buf[x*4+3] = 0;
		else 
			buf[x*4+3] = 0xFF;
	}
	
	return buf;
}

unsigned char *LoadTGA(char *filename, int *width, int *height)
{
	FILE *fp;
	unsigned char *buf;
	int len;
	int x, y, w, h;
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;
		
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);
	
	buf = (unsigned char *)malloc(len);
	fread(buf, len, 1, fp);
	fclose(fp);
	
	len = buf[0];
	
	printf("one");
	
	if (buf[1] != 0)
		return NULL;
	
	printf("one"); 
	
	if (buf[2] != 2)
		return NULL;
        
        printf("one");
        		
	x = buf[8]  | (buf[9]  << 8);
	y = buf[10] | (buf[11] << 8);
	w = buf[12] | (buf[13] << 8);
	h = buf[14] | (buf[15] << 8);
	
	if (buf[16] != 24)
		return NULL;

        printf("one");
        
        printf("buf 17 = %d\n", buf[17]);
        
	if (buf[17] != 32)
		return NULL;
	
	printf("one");
	        
	*width  = (w - x);
	*height = (h - y);
		
	return Pixel_BGR_RGB(&buf[18 + len], *width * *height * 3);	
}

void SaveTGA()
{
}

unsigned char *LoadPCX(char *filename, unsigned char **pal, int *width, int *height)
{
	pcx_header pcx;
	FILE *fp;
	unsigned char *buf, *ptr;
	int i, j, n, x, y, ch;

	fp = fopen(filename, "rb");
	if (fp == NULL)
		return NULL;

	fread(&pcx, sizeof(pcx), 1, fp);
	
	if (pcx.manufacturer != 10)
		return NULL;
	
	if (pcx.version != 5)
		return NULL;
		
	if (pcx.encoding != 1)
		return NULL;
		
	if (pcx.bits_per_pixel != 8)
		return NULL;
	
	if (pcx.num_color_planes != 1)
		return NULL;
	
	if (pcx.palette_type != 1)
		return NULL;
	
	x = pcx.width - pcx.x + 1;
	y = pcx.height - pcx.y + 1;
	
	buf = (unsigned char *)malloc(x * y);
	
	ptr = buf;	
	for (i = 0; i < (x * y); i++) {
		ch = fgetc(fp);
		if (ch == -1)
			return NULL;
		if (ch > 0xC0) {
			n = ch - 0xC1; /* 0xC0 */
			ch = fgetc(fp);
			for (j = 0; j < n; j++) {
				*ptr = (unsigned char)ch;
				ptr++;
				i++;
			}	
		}
		*ptr = (unsigned char)ch;
		ptr++;
	}
	
	fgetc(fp);
		
	ptr = (unsigned char *)malloc(768);
	for (i = 0; i < 768; i++) {
		ptr[i] = (unsigned char)fgetc(fp);
	}
		
	fclose(fp);
	
	*pal = ptr;
	*width = x;
	*height = y;
	
	return buf;
}

unsigned char *LoadPCX_RGB(char *filename, int *width, int *height)
{
	unsigned char *pal;
	unsigned char *dat, *dat2;
	
	dat = LoadPCX(filename, &pal, width, height);
	
	if (dat == NULL)
		return NULL;
	
	dat2 = Pixel_256_RGB(dat, pal, *width * *height);
	free(dat);
	free(pal);
	
	return dat2;
}	

unsigned char *LoadPCX_RGBA(char *filename, int *width, int *height)
{
	unsigned char *pal;
	unsigned char *dat, *dat2;
	
	dat = LoadPCX(filename, &pal, width, height);
	
	if (dat == NULL)
		return NULL;
	
	dat2 = Pixel_256_RGBA(dat, pal, *width * *height, 0);
	free(dat);
	free(pal);
	
	return dat2;
}	

void SavePCX(unsigned char *buf, int width, int height, unsigned char *pal, char *name)
{
	FILE *fp;
	pcx_header ph;
	unsigned char *dat, *ptr, *ptrd, ch;
	int x, y, z;

	ph.manufacturer = 10;
	ph.version = 5;
	ph.encoding = 1;
	ph.bits_per_pixel = 8;
	ph.x = ph.y = 0;
	ph.width = width - 1;
	ph.height = height - 1;
	ph.horz_res = ph.virt_res = 0; /* ? */
	for (x = 0; x < sizeof(ph.ega_palette); x++) 
		ph.ega_palette[x] = 0;
	ph.reserved = 0;
	ph.num_color_planes = 1;
	ph.byte_per_line = width;
	ph.palette_type = 1;
	ph.hscreen_size = width;
	ph.vscreen_size = height;
	for (x = 0; x < sizeof(ph.padding); x++)
		ph.padding[x] = 0;
	
#if 0
	dat = malloc(width * height * 2);
	for (x = 0; x < width * height; x++) {
		*(dat + x*2) = 0xC1;
		*(dat + x*2+1) = *(buf + x);
	} 	
#endif

	dat = malloc(width * height * 2);
	ptr = buf; ptrd = dat;
	x = 0; z = 0;
	while (x < width * height) {
		ch = *ptr;
		ptr++;
		x++;
		y = 0xC1;
		while((x < width * height) && (*ptr == ch) && (y < 0xFF)) {
			x++; y++; ptr++;
		}
		*ptrd = y;
		ptrd++;
		*ptrd = ch;
		ptrd++;
		z += 2;
	}
	dat = realloc(dat, z);
	
	fp = fopen(name, "w");	
	fwrite(&ph, sizeof(ph), 1, fp);
	fwrite(dat, 1, z, fp);
	fputc(12, fp);
	fwrite(pal, 1, 768, fp);
	fclose(fp);
	
	free(dat);
	
	return;
}

void SavePCX256ToFile(unsigned char *buf, int width, int height, unsigned char *pal, char *name)
{
	FILE *fp;
	pcx_header ph;
	unsigned char *dat, *ptr, *ptrd, ch;
	int x, y, z;

	memset(&ph, 0, sizeof(ph));
	
	ph.manufacturer		= 10;
	ph.version		= 5;
	ph.encoding		= 1;
	ph.bits_per_pixel	= 8;
	ph.x = ph.y		= 0;
	ph.width		= width - 1;
	ph.height		= height - 1;
	ph.horz_res = ph.virt_res = 0;
	ph.reserved		= 0;
	ph.num_color_planes	= 1;
	ph.byte_per_line	= width;
	ph.palette_type		= 1;
	ph.hscreen_size		= width;
	ph.vscreen_size		= height;

        dat = (unsigned char *)malloc(width * height * 2);

	ptr = buf; ptrd = dat;
	x = 0; z = 0;
	while (x < (width * height)) {
		ch = *ptr;
		ptr++;
		x++;
		y = 0xC1;
		while((x < (width * height)) && (*ptr == ch) && (y < 0xFF)) {
			x++; y++; ptr++;
		}
		if ((y == 0xC1) && (ch < 0xC0)) {
			*ptrd = ch;
			ptrd++;
			z++;			
		} else {
			*ptrd = y;
			ptrd++;
			*ptrd = ch;
			ptrd++;
			z += 2;
		}
	}

	fp = fopen(name, "wb");	
	fwrite(&ph, sizeof(ph), 1, fp);
	fwrite(dat, 1, z, fp);
	fputc(12, fp);
	fwrite(pal, 1, 768, fp);
	fclose(fp);
	
	free(dat);	
}

/* TODO: make sure RGB version works */
void SavePCXRGBToFile(unsigned char *buf, int width, int height, char *name)
{
	FILE *fp;
	pcx_header ph;
	unsigned char *dat;
	int x, y, s;
	
	memset(&ph, 0, sizeof(ph));
	
	ph.manufacturer 	= 10;
	ph.version 		= 5;
	ph.encoding		= 1;
	ph.bits_per_pixel 	= 8;
	ph.x = ph.y 		= 0;
	ph.width		= width - 1;
	ph.height		= height - 1;
	ph.horz_res = ph.virt_res = 0;
	ph.num_color_planes 	= 3;
	ph.byte_per_line 	= width;
	ph.palette_type 	= 1;
	ph.hscreen_size 	= width;
	ph.vscreen_size 	= height;

	dat = malloc(width * height * 2 * 3);
	for (y = 0; y < height; y++) {
		for (s = 0; s < 3; s++) {
			for (x = 0; x < width; x++) {
				*(dat + (y*(width*3) + (width*s) + x)*2) = 0xC1;
				*(dat + (y*(width*3) + (width*s) + x)*2+1) = *(buf + y*(width*3) + x*3 + s);
			}
		}
	}
	
	fp = fopen(name, "wb");	
	fwrite(&ph, sizeof(ph), 1, fp);
	fwrite(dat, 1, width * height * 2 * 3, fp);
	fclose(fp);
	
	free(dat);
}

void LoadILBM()
{
}
