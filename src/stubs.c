#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "psndplat.h"

/* smacker.c */
int FmvColorRed;
int FmvColorGreen;
int FmvColourBlue;

void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
	fprintf(stderr, "GetFMVInformation(%p, %p)\n", messageNumberPtr, frameNumberPtr);
}

void InitialiseTriggeredFMVs()
{
	fprintf(stderr, "InitialiseTriggeredFMVs()\n");
}

void StartFMVAtFrame(int number, int frame)
{
	fprintf(stderr, "StartFMVAtFrame(%d, %d)\n", number, frame);
}

void StartTriggerPlotFMV(int number)
{
	fprintf(stderr, "StartTriggerPlotFMV(%d)\n", number);
}

void UpdateAllFMVTextures()
{
	fprintf(stderr, "UpdateAllFMVTextures()\n");
}


/* win_func.cpp */
void CheckForWindowsMessages()
{
	fprintf(stderr, "CheckForWindowsMessages()\n");
}


/* psndplat.cpp */
ACTIVESOUNDSAMPLE ActiveSounds[SOUND_MAXACTIVE];
ACTIVESOUNDSAMPLE BlankActiveSound = {SID_NOSOUND,ASP_Minimum,0,0,NULL,0,0,0,0,0,{{0,0,0},0,0},NULL, NULL, NULL};
SOUNDSAMPLEDATA BlankGameSound = {0,0,0,0,NULL,0,NULL};
SOUNDSAMPLEDATA GameSounds[SID_MAXIMUM];

int PlatStartSoundSys()
{
	fprintf(stderr, "PlatStartSoundSys()\n");
	
	return 0;
}

void PlatEndSoundSys()
{
	fprintf(stderr, "PlatEndSoundSys()\n");
}

int PlatChangeGlobalVolume(int volume)
{
	fprintf(stderr, "PlatChangeGlobalVolume(%d)\n", volume);
	
	return 1;
}

int PlatPlaySound(int activeIndex)
{
	fprintf(stderr, "PlatPlaySound(%d)\n", activeIndex);
		
	return 1;
}

void PlatStopSound(int activeIndex)
{
	fprintf(stderr, "PlatStopSound(%d)\n", activeIndex);
}

int PlatChangeSoundVolume(int activeIndex, int volume)
{
	fprintf(stderr, "PlatChangeSoundVolume(%d, %d)\n", activeIndex, volume);
	
	return 1;
}

int PlatChangeSoundPitch(int activeIndex, int pitch)
{
	fprintf(stderr, "PlatChangeSoundPitch(%d, %d)\n", activeIndex, pitch);
	
	return 1;
}

int PlatSoundHasStopped(int activeIndex)
{
	fprintf(stderr, "PlatSoundHasStopped(%d)\n", activeIndex);
	
	return 1;
}

int PlatDo3dSound(int activeIndex)
{
	fprintf(stderr, "PlatDo3dSound(%d)\n", activeIndex);
	
	return 1;
}

void PlatEndGameSound(SOUNDINDEX index)
{
	fprintf(stderr, "PlatEndGameSound(%d)\n", index);
}

unsigned int PlatMaxHWSounds()
{
	fprintf(stderr, "PlatMaxHWSounds()\n");
	
	return 256;
}

void InitialiseBaseFrequency(SOUNDINDEX soundNum)
{
	fprintf(stderr, "InitialiseBaseFrequency(%d)\n", soundNum);
}

int LoadWavFile(int soundNum, char * wavFileName)
{
	fprintf(stderr, "LoadWavFile(%d, %s)\n", soundNum, wavFileName);
	
	return 0;
}

unsigned char *ExtractWavFile(int soundIndex, unsigned char *bufferPtr)
{
	fprintf(stderr, "ExtractWavFile(%d, %p)\n", soundIndex, bufferPtr);
	
	return 0;
}

int LoadWavFromFastFile(int soundNum, char * wavFileName)
{
	fprintf(stderr, "LoadWavFromFastFile(%d, %s)\n", soundNum, wavFileName);
	
	return 0;
}

void PlatUpdatePlayer()
{
	fprintf(stderr, "PlatUpdatePlayer()\n");
}

void PlatSetEnviroment(unsigned int env_index, float reverb_mix)
{
	fprintf(stderr, "PlatSetEnvironment(%d, %f)\n", env_index, reverb_mix);
}

void UpdateSoundFrequencies()
{
	fprintf(stderr, "UpdateSoundFreqncies()\n");
}


/* alt_tab.cpp */
void ATIncludeSurface(void * pSurface, void * hBackup)
{
	fprintf(stderr, "ATIncludeSurface(%p, %p)\n", pSurface, hBackup);
}

void ATRemoveSurface(void * pSurface)
{
	fprintf(stderr, "ATRemoveSurface(%p)\n", pSurface);
}

void ATRemoveTexture(void * pTexture)
{
	fprintf(stderr, "ATRemoveTexture(%p)\n", pTexture);
}

/* avp_menugfx.cpp */
char AAFontWidths[256];

void FadedScreen(int alpha)
{
	fprintf(stderr, "FadedScreen(%d)\n", alpha);
}


/* winmain.c */
BOOL KeepMainRifFile = FALSE;


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


/* d3d_render.cpp */
int NumberOfLandscapePolygons;
int FMVParticleColour;

void InitDrawTest()
{
	fprintf(stderr, "InitDrawTest()\n");
}

void InitForceField()
{
	fprintf(stderr, "InitForceField()\n");
}


/* avp_userprofile.c */
int SmackerSoundVolume;


/* dd_func.cpp */
long BackBufferPitch;

void FlipBuffers()
{
	fprintf(stderr, "FlipBuffers()\n");
}

int ChangePalette (unsigned char* NewPalette)
{
	fprintf(stderr, "ChangePalette(%p)\n", NewPalette);
	
	return 0;
}


/* di_func.cpp */
unsigned char DebouncedGotAnyKey;
unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
int GotJoystick;
int GotMouse;
int JoystickEnabled;
int MouseVelX;
int MouseVelY;

void DirectReadKeyboard()
{
	fprintf(stderr, "DirectReadKeyboard()\n");
}

void DirectReadMouse()
{
	fprintf(stderr, "DirectReadMouse()\n");
}

void ReadJoysticks()
{
	fprintf(stderr, "ReadJoysticks()\n");
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


/* winmain.c */
int HWAccel = 0;

/* dxlog.c */
void dx_str_log(char const * str, int line, char const * file)
{
	fprintf(stderr, "dx_str_log: %s/%d: %s\n", file, line, str);
}

void dx_strf_log(char const * fmt, ... )
{
	va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "dx_strf_log: ");
	vfprintf(stderr,fmt,ap);
        va_end(ap);
}

void dx_line_log(int line, char const * file)
{
	fprintf(stderr, "dx_line_log: %s/%d\n", file, line);
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

int WriteFile(HANDLE file, const void *data, int len, /* unsigned long */ void *byteswritten, int x)
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

int GetTickCount()
{
	fprintf(stderr, "GetTickCount()\n");
	
	return 0;
}
