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


/* directplay.c */
int DirectPlay_ConnectingToLobbiedGame(char* playerName)
{
	fprintf(stderr, "DirectPlay_ConnectingToLobbiedGame(%s)\n", playerName);
	
	return 0;
}

int DirectPlay_ConnectingToSession()
{
	fprintf(stderr, "DirectPlay_ConnectingToSession()\n");
	
	return 0;
}

BOOL DirectPlay_UpdateSessionList(int *SelectedItem)
{
	fprintf(stderr, "DirectPlay_UpdateSessionList(%p)\n", SelectedItem);
	
	return 0;
}

int DirectPlay_JoinGame()
{
	fprintf(stderr, "DirectPlay_JoinGame()\n");
	
	return 0;
}

void DirectPlay_EnumConnections()
{
	fprintf(stderr, "DirectPlay_EnumConnections()\n");
}

int DirectPlay_HostGame(char *playerName, char *sessionName,int species,int gamestyle,int level)
{
	fprintf(stderr, "DirectPlay_HostGame(%s, %s, %d, %d, %d)\n", playerName, sessionName, species, gamestyle, level);

	return 0;
}

int DirectPlay_ConnectToSession(int sessionNumber, char *playerName)
{
	fprintf(stderr, "DirectPlay_ConnectToSession(%d, %s)\n", sessionNumber, playerName);
	
	return 0;
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

int PlayMenuBackgroundBink()
{
	fprintf(stderr, "PlayMenuBackgroundBink()\n");
	
	return 0;
}

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


/* avp_menugfx.cpp */
/* char AAFontWidths[256]; */
AVPMENUGFX AvPMenuGfxStorage[MAX_NO_OF_AVPMENUGFXS]; /* TODO: this is initialized in avp_menugfx.cpp */

void InitialiseMenuGfx()
{
	fprintf(stderr, "InitialiseMenuGfx()\n");
}

void LoadAllAvPMenuGfx()
{
	fprintf(stderr, "LoadAllAvPMenuGfx()\n");
}

void ReleaseAllAvPMenuGfx()
{
	fprintf(stderr, "ReleaseAllAvPMenuGfx()\n");
}

void FadedScreen(int alpha)
{
	fprintf(stderr, "FadedScreen(%d)\n", alpha);
}

void ClearScreenToBlack()
{
	fprintf(stderr, "ClearScreenToBlack()\n");
}

void LoadAllSplashScreenGfx()
{
	fprintf(stderr, "LoadAllSplashScreenGfx()\n");
}

void LoadAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	fprintf(stderr, "LoadAvPMenuGfx(%d)\n", menuGfxID);
}

void DrawAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "DrawAvPMenuGfx(%d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format);
}

void DrawAvPMenuGfx_CrossFade(enum AVPMENUGFX_ID menuGfxID,enum AVPMENUGFX_ID menuGfxID2,int alpha)
{
	fprintf(stderr, "DrawAvPMenuGfx_CrossFade(%d, %d, %d)\n", menuGfxID, menuGfxID2, alpha);
}

void DrawAvPMenuGfx_Faded(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "DrawAvPMenuGfx_Faded(%d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format);
}

void DrawAvPMenuGfx_Clipped(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{
	fprintf(stderr, "DrawAvPMenuGfx_Clipped(%d, %d, %d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format, topY, bottomY);
}

int RenderMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "RenderMenuText(%s, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format);
	
	return 0;
}

int RenderMenuText_Clipped(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{
	fprintf(stderr, "RenderMenuText_Clipped(%s, %d, %d, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format, topY, bottomY);

	return 0;
}

int RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{
/*
	return Hardware_RenderSmallMenuText(textPtr, x, y, alpha, format);
*/
	fprintf(stderr, "RenderSmallMenuText(%s, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format);

	return 0;
}

int RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{
/*
	return Hardware_RenderSmallMenuText_Coloured(textPtr, x, y, alpha, format, red, green, blue);
*/
	fprintf(stderr, "RenderSmallMenuText_Coloured(%s, %d, %d, %d, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format, red, green, blue);
	
	return 0;
}

void RenderSmallFontString_Wrapped(char *textPtr,RECT* area,int alpha,int* output_x,int* output_y)
{
	fprintf(stderr, "RenderSmallFontString_Wrapped(%s, %p, %d, %p, %p)\n", textPtr, area, alpha, output_x, output_y);
}

void RenderKeyConfigRectangle(int alpha)
{
	void Hardware_RenderKeyConfigRectangle(int alpha);
	Hardware_RenderKeyConfigRectangle(alpha);
/*
	fprintf(stderr, "RenderKeyConfigRectangle(%d)\n", alpha);
*/	
}

void RenderHighlightRectangle(int x1,int y1,int x2,int y2, int r, int g, int b)
{
	void Hardware_RenderHighlightRectangle(int x1,int y1,int x2,int y2,int r, int g, int b);
	Hardware_RenderHighlightRectangle(x1, y1, x2, y2, r, g, b);
/*
	fprintf(stderr, "RenderHighlightRectangle(%d, %d, %d, %d, %d, %d, %d)\n", x1, y1, x2, y2, r, g, b);
*/	
}

int LengthOfMenuText(char *textPtr)
{
	fprintf(stderr, "LengthOfMenuText(%s)\n", textPtr);
	
	return 0;
}

int HeightOfMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	fprintf(stderr, "HeightOfMenuGfx(%d)\n", menuGfxID);
	
	return 0;
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


/* cd_player.c */
void CDDA_Start()
{
	fprintf(stderr, "CDDA_Start()\n");
}

void CDDA_End()
{
	fprintf(stderr, "CDDA_End()\n");
}

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

#include "aafont.h"
