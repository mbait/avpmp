#ifndef __FIXER_H__
#define __FIXER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
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

#define MAX_PATH	PATH_MAX

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
typedef DWORD * LPDWORD;
typedef char * LPTSTR;

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

#define INVALID_HANDLE_VALUE		-1
#define GENERIC_WRITE			1
#define CREATE_ALWAYS			2
#define FILE_FLAG_RANDOM_ACCESS		3
#define GENERIC_READ			4
#define OPEN_EXISTING			5
#define FILE_ATTRIBUTE_READONLY		6
#define FILE_CURRENT			7
#define FILE_BEGIN			8	
#define FILE_END			9
#define FILE_SHARE_READ			10
#define FILE_ATTRIBUTE_DIRECTORY	11
#define FILE_SHARE_WRITE		12
#define OPEN_ALWAYS			13
#define FILE_ATTRIBUTE_NORMAL		14

HANDLE CreateFile(const char *file, int write, int x, int y, int flags, int flags2, int z);
HANDLE CreateFileA(const char *file, int write, int x, int y, int flags, int flags2, int z);
int WriteFile(HANDLE file, const void *data, int len, /* unsigned long */ void *byteswritten, int x);
int ReadFile(HANDLE file, void *data, int len, /* unsigned long */ void *bytesread, int x);
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

#ifdef __cplusplus
};
#endif

#endif
