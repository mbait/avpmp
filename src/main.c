#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <GL/gl.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "gamedef.h"
#include "gameplat.h"
#include "ffstdio.h"
#include "vision.h"
#include "comp_shp.h"
#include "avp_envinfo.h"
#include "stratdef.h"
#include "bh_types.h"
#include "avp_userprofile.h"
#include "pldnet.h"
#include "cdtrackselection.h"

char LevelName[] = {"predbit6\0QuiteALongNameActually"}; /* the real way to load levels */

extern int ScanDrawMode; /* to fix image loading */
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock; /* this should be put in a header file */
extern unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
extern unsigned char KeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
extern int DebouncedGotAnyKey;
extern unsigned char GotAnyKey;

PROCESSORTYPES ReadProcessorType()
{
	return PType_PentiumMMX;
}

int InitialiseWindowsSystem()
{
	ScanDrawMode = ScanDrawD3DHardwareRGB;
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL Init failed: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	if (SDL_SetVideoMode(640, 480, 0, SDL_OPENGL) == NULL) {
		fprintf(stderr, "SDL SetVideoMode failed: %s\n", SDL_GetError());
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	
	glViewport(0, 0, 640, 480);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
		
	return 0;
}

static int KeySymToKey(int keysym)
{
	switch(keysym) {
		case SDLK_ESCAPE:
			return KEY_ESCAPE;
		case SDLK_RETURN:
			return KEY_CR;
			
		case SDLK_LEFT:
			return KEY_LEFT;
		case SDLK_RIGHT:
			return KEY_RIGHT;
		case SDLK_UP:
			return KEY_UP;
		case SDLK_DOWN:
			return KEY_DOWN;

		default:
			return -1;
	}
}

static void handle_keypress(int keysym, int press)
{
	int key = KeySymToKey(keysym);
	
	if (key == -1)
		return;

	if (press && !KeyboardInput[key]) {
		DebouncedKeyboardInput[key] = 1;
		DebouncedGotAnyKey = 1;
	}
	
	GotAnyKey = 1;
	KeyboardInput[key] = press;
}

void CheckForWindowsMessages()
{
	SDL_Event event;
	
	GotAnyKey = 0;
	DebouncedGotAnyKey = 0;
	memset(DebouncedKeyboardInput, 0, sizeof(DebouncedKeyboardInput));
	
	if (SDL_PollEvent(&event)) {
		do {
			switch(event.type) {
				case SDL_KEYDOWN:
					handle_keypress(event.key.keysym.sym, 1);
					break;
				case SDL_KEYUP:
					handle_keypress(event.key.keysym.sym, 0);
					break;
				case SDL_QUIT:
					SDL_Quit();
					exit(17); /* TODO tempy! */
					break;
			}
		} while (SDL_PollEvent(&event));
	}
}
        
void InGameFlipBuffers()
{
	SDL_GL_SwapBuffers();
}

void ThisFramesRenderingHasBegun()
{
	fprintf(stderr, "ThisFramesRenderingHasBegun()\n");
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ThisFramesRenderingHasFinished()
{
	fprintf(stderr, "ThisFramesRenderingHasFinished()\n");
}
                
int ExitWindowsSystem()
{
	SDL_Quit();
	
	return 0;
}

int main(int argc, char *argv[])
{
	int menusActive = 0;
	int thisLevelHasBeenCompleted = 0;
	
	LoadCDTrackList();
	
	SetFastRandom();
	
	GetPathFromRegistry();

#if MARINE_DEMO
	ffInit("fastfile/mffinfo.txt","fastfile/");
#elif ALIEN_DEMO
	ffInit("alienfastfile/ffinfo.txt","alienfastfile/");
#else
	ffInit("fastfile/ffinfo.txt","fastfile/");
#endif
	InitGame();

	InitialVideoMode();

	/* Env_List can probably be removed */
//	Env_List[0] = &(ELOLevelToLoad); /* overwrite the first entry of crappy env_list with LevelName */
	Env_List[0]->main = LevelName;
	
	InitialiseSystem();
	InitialiseRenderer();
	
/*	InitOptionsMenu();  NOT YET */
	
	LoadKeyConfiguration();
	
	SoundSys_Start();
	CDDA_Start();
	
	InitTextStrings();
	
	BuildMultiplayerLevelNameArray();
	
	ChangeDirectDrawObject();
	AvP.LevelCompleted = 0;
	LoadSounds("PLAYER");

	AvP.CurrentEnv = AvP.StartingEnv = 0; /* are these even used? */
	SetLevelToLoad(AVP_ENVIRONMENT_INVASION); /* because the menus aren't implemented */
	 
// while(AvP_MainMenus()) {

	d3d_light_ctrl.ctrl = LCCM_NORMAL;
	d3d_overlay_ctrl.ctrl = OCCM_NORMAL;
	
	/* this was in windows SetGameVideoMode: */
	ScreenDescriptorBlock.SDB_Width     = 640;
	ScreenDescriptorBlock.SDB_Height    = 480;
	ScreenDescriptorBlock.SDB_CentreX   = 640/2;
	ScreenDescriptorBlock.SDB_CentreY   = 480/2;
	ScreenDescriptorBlock.SDB_ProjX     = 640/2;
	ScreenDescriptorBlock.SDB_ProjY     = 480/2;
	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = 640;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = 480;
	
	// GetCorrectDirectDrawObject();
	
	start_of_loaded_shapes = load_precompiled_shapes();
	
	InitCharacter();
	
	LoadRifFile(); /* sets up a map */
	
	AssignAllSBNames();
	
	StartGame();
	
	ffcloseall();
	
	AvP.MainLoopRunning = 1;
	
/*	ScanImagesForFMVs(); NOT YET */
	
	ResetFrameCounter();
	
/*	Game_Has_Loaded(); NOT YET */
	
	ResetFrameCounter();
	
/*	IngameKeyboardInput_ClearBuffer(); NOT YET */
	
	while(AvP.MainLoopRunning) {
		CheckForWindowsMessages();
		
		switch(AvP.GameMode) {
		case I_GM_Playing:
			if ((!menusActive || (AvP.Network!=I_No_Network && !netGameData.skirmishMode)) && !AvP.LevelCompleted) {
				/* TODO: print some debugging stuff */
				
				DoAllShapeAnimations();
				
				UpdateGame();
				
				AvpShowViews();
				
				MaintainHUD();
				
				CheckCDAndChooseTrackIfNeeded();
				
				if(InGameMenusAreRunning() && ( (AvP.Network!=I_No_Network && netGameData.skirmishMode) || (AvP.Network==I_No_Network)) ) {
					SoundSys_StopAll();
				}
			} else {
				ReadUserInput();
				
				/* UpdateAllFMVTextures(); NOT YET */
			
				SoundSys_Management();
				
				FlushD3DZBuffer();
				
				ThisFramesRenderingHasBegun();
			}

/*	NOT YET
			menusActive = AvP_InGameMenus();
			if (AvP.RestartLevel) menusActive=0;
*/
			
			if (AvP.LevelCompleted) {
				SoundSys_FadeOutFast();
				DoCompletedLevelStatisticsScreen();
				thisLevelHasBeenCompleted = 1;
			}

			ThisFramesRenderingHasFinished();

			InGameFlipBuffers();
			
			FrameCounterHandler();
			{
				PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
				
				if (!menusActive && playerStatusPtr->IsAlive && !AvP.LevelCompleted) {
					DealWithElapsedTime();
				}
			}
			break;
			
		case I_GM_Menus:
			AvP.GameMode = I_GM_Playing;
			break;
		case I_GM_Paused:
//			break;
		default:
			fprintf(stderr, "AvP.MainLoopRunning: gamemode = %d\n", AvP.GameMode);
			exit(EXIT_FAILURE);
		}
		
		if (AvP.RestartLevel) {
			AvP.RestartLevel = 0;
			AvP.LevelCompleted = 0;
/* NOT YET
			FixCheatModesInUserProfile(UserProfilePtr);
*/			
			RestartLevel();
		}
		
//		break; /* TODO -- remove when loop works */
	}
	
	AvP.LevelCompleted = thisLevelHasBeenCompleted;

/* NOT YET	
	FixCheatModesInUserProfile(UserProfilePtr);
*/

/*	NOT YET
	CloseFMV();
	ReleaseAllFMVTextures();
*/

	CONSBIND_WriteKeyBindingsToConfigFile();
	
	DeInitialisePlayer();
	
	DeallocatePlayersMirrorImage();
	
	KillHUD();
	
	Destroy_CurrentEnvironment();
	
	DeallocateAllImages();
	
	EndNPCs();
	
	ExitGame();
	
	SoundSys_StopAll();
	
	SoundSys_ResetFadeLevel();
	
	CDDA_Stop();
	
	if (AvP.Network != I_No_Network) {
/* NOT YET
		EndAVPNetGame();
*/
	}
	
	ClearMemoryPool();
		
// }

	SoundSys_StopAll();
	SoundSys_RemoveAll();
	
	ExitSystem();
	
	CDDA_End();
	ClearMemoryPool();
	
	fprintf(stderr, "Now exiting Aliens vs Predator!  At least it didn't crash!\n");
	
	return 0;
}
