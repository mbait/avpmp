#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"

/* avpreg.cpp */
char* AvpCDPath = 0;
void GetPathFromRegistry()
{
extern char * SecondTex_Directory;
extern char * SecondSoundDir;

	fprintf(stderr, "GetPathFromRegistry()\n");
	
	SecondTex_Directory = "./graphics";
	SecondSoundDir = "./sounds";
}


/* dd_func.cpp */
void FlipBuffers()
{
	fprintf(stderr, "FlipBuffers()\n");
}


/* dx_proj.cpp */
int use_mmx_math = 1;


/* cd_player.c */
void CDDA_ChangeVolume(int volume)
{
	fprintf(stderr, "CDDA_ChangeVolume(%d)\n", volume);
}

int CDDA_CheckNumberOfTracks()
{
	fprintf(stderr, "CDDA_CheckNumberOfTracks()\n");
	
	return 0;
}

int CDDA_IsOn()
{
	fprintf(stderr, "CDDA_IsOn()\n");
	
	return 0;
}

int CDDA_IsPlaying()
{
	fprintf(stderr, "CDDA_IsPlaying()\n");
	
	return 0;
}

void CDDA_Play(int CDDATrack)
{
	fprintf(stderr, "CDDA_Play(%d)\n", CDDATrack);
}

void CDDA_PlayLoop(int CDDATrack)
{
	fprintf(stderr, "CDDA_PlayLoop(%d)\n", CDDATrack);
}

void CDDA_Stop()
{
	fprintf(stderr, "CDDA_Stop()\n");
}

void CDDA_SwitchOn()
{
	fprintf(stderr, "CDDA_SwitchOn()\n");
}


#if 0
/* pldnet.c */

#include "pldnet.h"

MULTIPLAYER_START* marineStartPositions=0;
#endif

/* win32 api */
size_t _mbclen(const unsigned char *s)
{
	return strlen((const char *)s);
}

HANDLE CreateFile(const char *file, int write, int x, int y, int flags, int flags2, int z)
{
	fprintf(stderr, "CreateFile(%s, %d, %d, %d, %d, %d, %d)\n", file, write, x, y, flags, flags2, z);
	
	return -1;
}

HANDLE CreateFileA(const char *file, int write, int x, int y, int flags, int flags2, int z)
{
	return CreateFileA(file, write, x, y, flags, flags2, z);
}

int WriteFile(HANDLE file, const void *data, int len, unsigned long *byteswritten, int x)
{
	fprintf(stderr, "WriteFile(%d, %p, %d, %p, %d)\n", file, data, len, byteswritten, x);

	return -1;
}

int ReadFile(HANDLE file, void *data, int len, /* unsigned long */ void *bytesread, int x)
{
	fprintf(stderr, "ReadFile(%d, %p, %d, %p, %d)\n", file, data, len, bytesread, x);

	return -1;
}

int GetFileSize(HANDLE file, int x)
{
	fprintf(stderr, "GetFileSize(%d, %d)\n", file, x);
	
	return -1;
}

int CloseHandle(HANDLE file)
{
	fprintf(stderr, "CloseHandle(%d)\n", file);
	
	return -1;
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
int timeGetTime()
{
	fprintf(stderr, "timeGetTime()\n");
	
	return 0;
}
