#ifndef __FIXER_H__
#define __FIXER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // int64_t

#define PACKED __attribute__((packed))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define __cdecl

#define TRUE    1
#define FALSE   0

#define stricmp         strcasecmp
#define _stricmp        strcasecmp

/* windows junk */
typedef int GUID;
typedef int DPID;
typedef int HINSTANCE;
typedef int WPARAM;
typedef int LPARAM;
typedef int HANDLE;

typedef int BOOL;

typedef struct RECT
{
	int left;
	int top;
	int right;
	int bottom;
} RECT;

typedef RECT RECT_AVP;

typedef int64_t __int64;

typedef int FILETIME;

typedef struct SYSTEMTIME
{
//#warning "TODO: SYSTEMTIME format is not correct"
	int wHour;
	int wMinute;
	int wSecond;
	int wYear;
	int wMonth;
	int wDay;
} SYSTEMTIME;
                                                
#endif
