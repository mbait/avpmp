#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>

/* TODO: need unsigned version, way to signify success/EOF */
/* ReadBytes */

int32_t ReadInt32M(FILE *fp);
int32_t ReadInt24M(FILE *fp);
int16_t ReadInt16M(FILE *fp);
int32_t ReadInt32L(FILE *fp);
int32_t ReadInt24L(FILE *fp);
int16_t ReadInt16L(FILE *fp);

int8_t ReadInt8(FILE *fp);

int filelength(int filedes);
int fsize(char *file_name);

#endif
