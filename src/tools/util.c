#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "util.h"

int32_t ReadInt32M(FILE *fp)
{
	unsigned char d[4];
	
	fread(d, 1, 4, fp);
	
	return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
}

int32_t ReadInt24M(FILE *fp)
{
	unsigned char d[3];
	
	fread(d, 1, 3, fp);
	
	return (d[0] << 16) | (d[1] << 8) | (d[2] << 0);
}

int16_t ReadInt16M(FILE *fp)
{
	unsigned char d[2];
	
	fread(d, 1, 2, fp);
	
	return (d[0] << 8) | (d[1] << 0);
}

int32_t ReadInt32L(FILE *fp)
{
	unsigned char d[4];
	
	fread(d, 1, 4, fp);
	
	return (d[3] << 24) | (d[2] << 16) | (d[1] << 8) | (d[0] << 0);
}

int32_t ReadInt24L(FILE *fp)
{
	unsigned char d[3];
	
	fread(d, 1, 3, fp);
	
	return (d[2] << 16) | (d[1] << 8) | (d[0] << 0);
}

int16_t ReadInt16L(FILE *fp)
{
	unsigned char d[2];
	
	fread(d, 1, 2, fp);
	
	return (d[1] << 8) | (d[0] << 0);
}

int8_t ReadInt8(FILE *fp)
{
	unsigned char d[1];
	
	fread(d, 1, 1, fp);
	
	return d[0];
}

int filelength(int filedes)
{
	struct stat buf;
	
	if (fstat(filedes, &buf) == -1) 
		return -1;
	
	return buf.st_size;
}

int fsize(char *file_name)
{
	struct stat buf;
	
	if (stat(file_name, &buf) == -1)
		return -1;
	
	return buf.st_size;
}
