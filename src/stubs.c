#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "psndplat.h"
#include "module.h"
#include "stratdef.h"
#include "avp_userprofile.h"
#include "projfont.h"
#include "savegame.h"
#include "pldnet.h"
#include "kshape.h"
#include "d3d_hud.h"


/* winmain.c */
BOOL KeepMainRifFile = FALSE;
int HWAccel = 1;
int VideoModeNotAvailable=0;


/* videomodes.cpp */
void LoadDeviceAndVideoModePreferences()
{
	fprintf(stderr, "LoadDeviceAndVideoModePreferences()\n");
}

void SaveDeviceAndVideoModePreferences()
{
	fprintf(stderr, "SaveDeviceAndVideoModePreferences()\n");
}

void PreviousVideoMode2()
{
	fprintf(stderr, "PreviousVideoMode2()\n");
}

void NextVideoMode2()
{
	fprintf(stderr, "NextVideoMode2()\n");
}

char *GetVideoModeDescription2()
{
	fprintf(stderr, "GetVideoModeDescription2()\n");
	
	return "";
}

char *GetVideoModeDescription3()
{
	fprintf(stderr, "GetVideoModeDescription3()\n");
	
	return "";
}


/* cd_player.cpp */
int CDPlayerVolume;

void CheckCDVolume()
{
	fprintf(stderr, "CheckCDVolume()\n");
}

/* bink.c */
void PlayBinkedFMV(char *filenamePtr)
{
	fprintf(stderr, "PlayBinkedFMV(%s)\n", filenamePtr);
}

void StartMenuBackgroundBink()
{
	fprintf(stderr, "StartMenuBackgroundBink()\n");
}

#if 0
int PlayMenuBackgroundBink()
{
	fprintf(stderr, "PlayMenuBackgroundBink()\n");
	
	return 0;
}
#endif

void EndMenuBackgroundBink()
{
	fprintf(stderr, "EndMenuBackgroundBink()\n");
}


/* smacker.c */
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;
int IntroOutroMoviesAreActive = 1;
int MoviesAreActive;
int SmackerSoundVolume;

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

void EndMenuMusic()
{
	fprintf(stderr, "EndMenuMusic()\n");
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


/* avpreg.cpp */
char *AvpCDPath = 0;

void GetPathFromRegistry()
{
extern char * SecondTex_Directory;
extern char * SecondSoundDir;

	fprintf(stderr, "GetPathFromRegistry()\n");
	
	SecondTex_Directory = "./graphics";
	SecondSoundDir = "./sounds";
}


/* d3_func.cpp */
int GetTextureHandle(IMAGEHEADER *imageHeaderPtr)
{
	fprintf(stderr, "GetTextureHandle(%p)\n", imageHeaderPtr);
	
	return 1;
}

/* d3d_render.cpp -- some of these got mixed in with d3_func.cpp! */
int NumberOfLandscapePolygons;
int FMVParticleColour;
int WireFrameMode;
int WaterFallBase;

void InitDrawTest()
{
	fprintf(stderr, "InitDrawTest()\n");
}

void CheckWireFrameMode(int shouldBeOn)
{
//	fprintf(stderr, "CheckWireFrameMode(%d)\n", shouldBeOn);
	if (shouldBeOn)
		fprintf(stderr, "CheckWireFrameMode(%d)\n", shouldBeOn);
}

void WriteEndCodeToExecuteBuffer()
{
	fprintf(stderr, "WriteEndCodeToExecuteBuffer()\n");
}

BOOL UnlockExecuteBufferAndPrepareForUse()
{
	fprintf(stderr, "UnlockExecuteBufferAndPrepareForUse()\n");
	
	return FALSE;
}

void ReloadImageIntoD3DImmediateSurface(IMAGEHEADER* iheader)
{
	fprintf(stderr, "ReloadImageIntoD3DImmediateSurface(%p)\n", iheader);
}

void ReleaseDirect3DNotDDOrImages()
{
	fprintf(stderr, "ReleaseDirect3DNotDDOrImages()\n");
}

void ReleaseDirect3DNotDD()
{
	fprintf(stderr, "ReleaseDirect3DNotDD()\n");
}

void ReleaseDirect3D()
{
	fprintf(stderr, "ReleaseDirect3D()\n");
}

BOOL ExecuteBuffer()
{
//	fprintf(stderr, "ExecuteBuffer()\n");
	
	return FALSE;
}

BOOL EndD3DScene()
{
	fprintf(stderr, "EndD3DScene()\n");
	
	return FALSE;
}

/* ddplat.cpp */
void MinimizeAllDDGraphics()
{
	fprintf(stderr, "MinimizeAllDDGraphics()\n");
}

        
/* dd_func.cpp */
long BackBufferPitch;
int VideoModeColourDepth;

int ChangePalette (unsigned char* NewPalette)
{
	fprintf(stderr, "ChangePalette(%p)\n", NewPalette);
	
	return 0;
}

void BlitWin95Char(int x, int y, unsigned char toprint)
{
	fprintf(stderr, "BlitWin95Char(%d, %d, %d)\n", x, y, toprint);
}

void LockSurfaceAndGetBufferPointer()
{
	fprintf(stderr, "LockSurfaceAndGetBufferPointer()\n");
}

void finiObjectsExceptDD()
{
	fprintf(stderr, "finiObjectsExceptDD()\n");
}

void finiObjects()
{
	fprintf(stderr, "finiObjects()\n");
}

void UnlockSurface()
{
	fprintf(stderr, "UnlockSurface()\n");
}

void ReleaseDDSurface(void* DDSurface)
{
	fprintf(stderr, "ReleaseDDSurface(%p)\n", DDSurface);
}

BOOL ChangeDirectDrawObject()
{
	fprintf(stderr, "ChangeDirectDrawObject()\n");
	
	return FALSE;
}

int SelectDirectDrawObject(void *pGUID)
{
	fprintf(stderr, "SelectDirectDrawObject(%p)\n", pGUID);

	return 0;
}

void GenerateDirectDrawSurface()
{
	fprintf(stderr, "GenerateDirectDrawSurface()\n");
}


/* dx_proj.cpp */
int use_mmx_math = 0;



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
	fprintf(stderr, "\n");
        va_end(ap);
}

void dx_line_log(int line, char const * file)
{
	fprintf(stderr, "dx_line_log: %s/%d\n", file, line);
}
