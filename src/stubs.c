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


/* avp_mp_config.cpp */
char* GetCustomMultiplayerLevelName(int index, int gameType)
{
	fprintf(stderr, "GetCustomMultiplayerLevelName(%d, %d)\n", index, gameType);
	
	return NULL;
}

void BuildMultiplayerLevelNameArray()
{
	fprintf(stderr, "BuildMultiplayerLevelNameArray()\n");
}

BOOL BuildLoadIPAddressMenu()
{
	fprintf(stderr, "BuildLoadIPAddressMenu()\n");
	
	return FALSE;
}


/* avp_intro.cpp */
void DrawMainMenusBackdrop()
{
	fprintf(stderr, "DrawMainMenusBackdrop()\n");
}


/* cd_player.cpp */
int CDPlayerVolume;

void CheckCDVolume()
{
	fprintf(stderr, "CheckCDVolume()\n");
}


/* smacker.c */
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;
int IntroOutroMoviesAreActive = 1;
int MoviesAreActive;

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

void FadedScreen(int alpha)
{
	fprintf(stderr, "FadedScreen(%d)\n", alpha);
}

void ClearScreenToBlack()
{
	fprintf(stderr, "ClearScreenToBlack()\n");
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

int RenderMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "RenderMenuText(%s, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format);
	
	return 0;
}

void LoadAllSplashScreenGfx()
{
	fprintf(stderr, "LoadAllSplashScreenGfx()\n");
}

void LoadAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	fprintf(stderr, "LoadAvPMenuGfx(%d)\n", menuGfxID);
}


/* avp_menus.cpp */
AVP_USER_PROFILE *UserProfilePtr;
CONTROL_METHODS PlayerControlMethods;
int NumberOfSessionsFound;
JOYSTICK_CONTROL_METHODS PlayerJoystickControlMethods;
SESSION_DESC SessionData[MAX_NO_OF_SESSIONS];
SAVE_SLOT_HEADER SaveGameSlot[NUMBER_OF_SAVE_SLOTS];

void GetFilenameForSaveSlot(int i, unsigned char *filenamePtr)
{
	fprintf(stderr, "GetFilenameForSaveSlot(%d, %p)\n", i, filenamePtr);
}

void RenderBriefingText(int centreY, int brightness)
{
	fprintf(stderr, "RenderBriefingText(%d, %d)\n", centreY, brightness);
}

void LoadLevelHeader(SAVE_BLOCK_HEADER* header)
{
	fprintf(stderr, "LoadLevelHeader(%p)\n", header);
}

void AvP_TriggerInGameMenus()
{
	fprintf(stderr, "AvP_TriggerInGameMenus()\n");
}

void ScanSaveSlots()
{
	fprintf(stderr, "ScanSaveSlots()\n");
}

void SaveLevelHeader()
{
	fprintf(stderr, "SaveLevelHeader()\n");
}

int InGameMenusAreRunning()
{
	fprintf(stderr, "InGameMenusAreRunning()\n");
	
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


/* avp_userprofile.c */
int SmackerSoundVolume;

int NumberOfUserProfiles()
{
	fprintf(stderr, "NumberOfUserProfiles()\n");
	
	return 0;
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
