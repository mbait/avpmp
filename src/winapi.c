#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "fixer.h"

void FixFilename(char *str)
{
	int len = strlen(str);
	int i;
	
	for (i = 0; i < len; i++) {
		if (str[i] == '\\')
			str[i] = '/';
		else if (str[i] == '\r')
			str[i] = 0;
		else if (str[i] == '\n')
			str[i] = 0;
		else
			str[i] = tolower(str[i]);
	}
}

size_t _mbclen(const unsigned char *s)
{
	return strlen((const char *)s);
}

HANDLE CreateFile(const char *file, int mode, int x, int y, int flags, int flags2, int z)
{
	int fd;
	
	fprintf(stderr, "CreateFile(%s, %d, %d, %d, %d, %d, %d)\n", file, mode, x, y, flags, flags2, z);

	switch(mode) {
		case GENERIC_READ:
			if (flags != OPEN_EXISTING) {
				fprintf(stderr, "CreateFile: GENERIC_READ flags = %d\n", flags);
				exit(EXIT_FAILURE);
			}
 			fd = open(file, O_RDONLY);
 			if (fd == -1) {
 				perror("CreateFile");
 				return INVALID_HANDLE_VALUE;
 			}
			break;
		case GENERIC_WRITE:
			if (flags != CREATE_ALWAYS) {
				fprintf(stderr, "CreateFile: GENERIC_WRITE flags = %d\n", flags);
				exit(EXIT_FAILURE);
			}
			fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
			if (fd == -1) {
				perror("CreateFile");
				return INVALID_HANDLE_VALUE;
			}
			break;
		case GENERIC_READ|GENERIC_WRITE:
//			break;
		default:
			fprintf(stderr, "CreateFile: unknown mode %d\n", mode);
			exit(EXIT_FAILURE);
	}
		
	return (HANDLE)fd;
}

HANDLE CreateFileA(const char *file, int write, int x, int y, int flags, int flags2, int z)
{
	return CreateFile(file, write, x, y, flags, flags2, z);
}

int WriteFile(HANDLE file, const void *data, int len, /* unsigned long */ void *byteswritten, int x)
{
	unsigned long *bw;
	
	fprintf(stderr, "WriteFile(%d, %p, %d, %p, %d)\n", file, data, len, byteswritten, x);

	bw = (unsigned long *)byteswritten;
	
	*bw = write(file, data, len);
	
	return 0;
}

int ReadFile(HANDLE file, void *data, int len, /* unsigned long */ void *bytesread, int x)
{
	unsigned long *br;
	
	fprintf(stderr, "ReadFile(%d, %p, %d, %p, %d)\n", file, data, len, bytesread, x);

	br = (unsigned long *)bytesread;
	
	*br = read(file, data, len);
	
	return 1; /* TODO: what is the correct return value? */
}

int GetFileSize(HANDLE file, int x)
{
	struct stat buf;
	
	fprintf(stderr, "GetFileSize(%d, %d)\n", file, x);
	
	if (fstat(file, &buf) == -1)
		return -1;
	return buf.st_size;
}

int CloseHandle(HANDLE file)
{
	fprintf(stderr, "CloseHandle(%d)\n", file);
	
	close(file);
	
	return 0;
}

int DeleteFile(const char *file)
{
	fprintf(stderr, "DeleteFile(%s)\n", file);
	
	return -1;
}

int DeleteFileA(const char *file)
{
	return DeleteFile(file);
}

int GetDiskFreeSpace(int x, unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *d)
{
	fprintf(stderr, "GetDiskFreeSpace(%d, %p, %p, %p, %p)\n", x, a, b, c, d);

	return -1;
}

int CreateDirectory(char *dir, int x)
{
	fprintf(stderr, "CreateDirectory(%s, %d)\n", dir, x);
	
	return -1;
}

int MoveFile(const char *newfile, const char *oldfile)
{
	fprintf(stderr, "MoveFile(%s, %s)\n", newfile, oldfile);
	
	return -1;
}

int MoveFileA(const char *newfile, const char *oldfile)
{
	return MoveFile(newfile, oldfile);
}

int CopyFile(const char *newfile, const char *oldfile, int x)
{
	fprintf(stderr, "CopyFile(%s, %s, %d)\n", newfile, oldfile, x);
	
	return -1;
}

int GetFileAttributes(const char *file)
{
	fprintf(stderr, "GetFileAttributes(%s)\n", file);
	
	return -1;
}

int GetFileAttributesA(const char *file)
{
	return GetFileAttributes(file);
}

int SetFilePointer(HANDLE file, int x, int y, int z)
{
	fprintf(stderr, "SetFilePointer(%d, %d, %d, %d)\n", file, x, y, z);
	
	return -1;
}

int SetEndOfFile(HANDLE file)
{
	fprintf(stderr, "SetEndOfFile(%d)\n", file);
	
	return -1;
}

/* time in miliseconds */
struct timeval tv0;

unsigned int timeGetTime()
{
	struct timeval tv1;
	int secs, usecs;
	
	if (tv0.tv_sec == 0) {
		gettimeofday(&tv0, NULL);
	
		return 0;
	}
	
	gettimeofday(&tv1, NULL);
	
	secs = tv1.tv_sec - tv0.tv_sec;
	usecs = tv1.tv_usec - tv0.tv_usec;
	if (usecs < 0) {
		usecs += 1000000;
		secs--;
	}

	return secs * 1000 + (usecs / 1000);
}

unsigned int GetTickCount()
{
	return timeGetTime();
}
