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

#define strnicmp	strncasecmp
#define _strnicmp	strncasecmp

#define	lstrlen		strlen
#define lstrcat		strcat

#define _tcslen		strlen
#define _tcscpy		strcpy

/* #define _mbclen		strlen */
size_t _mbclen(const unsigned char *s);

#define RGBA_MAKE(r,g,b,a) (((r) << 24) | ((g) << 16) | ((b) << 8) | (a))

/* windows junk */
typedef int GUID;
typedef int DPID;
typedef int HINSTANCE;
typedef int WPARAM;
typedef int LPARAM;
typedef int HANDLE;

typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef long LONG;
typedef char TCHAR;

#define TEXT(x) x

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

#define INVALID_HANDLE_VALUE	-1
#define GENERIC_WRITE		0
#define CREATE_ALWAYS		1
#define FILE_FLAG_RANDOM_ACCESS	2
#define GENERIC_READ		3
#define OPEN_EXISTING		4
#define FILE_ATTRIBUTE_READONLY	5
#define FILE_CURRENT		6
#define FILE_BEGIN		7
#define FILE_END		8

HANDLE CreateFile(const char *file, int write, int x, int y, int flags, int flags2, int z);
HANDLE CreateFileA(const char *file, int write, int x, int y, int flags, int flags2, int z);
int WriteFile(HANDLE file, const void *data, int len, unsigned long *byteswritten, int x);
int ReadFile(HANDLE file, void *data, int len, unsigned long *bytesread, int x);
int GetFileSize(HANDLE file, int x);
int CloseHandle(HANDLE file);
int DeleteFile(const char *file);
int DeleteFileA(const char *file);
int GetDiskFreeSpace(int x, unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *d);                                                
int CreateDirectory(char *dir, int x);
int MoveFile(const char *newfile, const char *oldfile);
int MoveFileA(const char *newfile, const char *oldfile);
int CopyFile(const char *newfile, const char *oldfile, int x);
int GetFileAttributes(const char *file);
int GetFileAttributesA(const char *file);
int SetFilePointer(HANDLE file, int x, int y, int z);
int SetEndOfFile(HANDLE file);

#endif
