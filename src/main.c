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
#include "gammacontrol.h"

#define MyWidth		1024
#define MyHeight	768

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

#if 1	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL Init failed: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	if (SDL_SetVideoMode(MyWidth, MyHeight, 0, SDL_OPENGL) == NULL) {
		fprintf(stderr, "SDL SetVideoMode failed: %s\n", SDL_GetError());
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
	
	glViewport(0, 0, MyWidth, MyHeight);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
	glEnable(GL_TEXTURE_2D);

/*	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
*/	
#endif		
	return 0;
}

static int KeySymToKey(int keysym)
{
	switch(keysym) {
		case SDLK_ESCAPE:
			return KEY_ESCAPE;
			
		case SDLK_0:
			return KEY_0;
		case SDLK_1:
			return KEY_1;
		case SDLK_2:
			return KEY_2;
		case SDLK_3:
			return KEY_3;
		case SDLK_4:
			return KEY_4;
		case SDLK_5:
			return KEY_5;
		case SDLK_6:
			return KEY_6;
		case SDLK_7:
			return KEY_7;
		case SDLK_8:
			return KEY_8;
		case SDLK_9:
			return KEY_9;
		
		case SDLK_a:
			return KEY_A;
		case SDLK_b:
			return KEY_B;
		case SDLK_c:
			return KEY_C;
		case SDLK_d:
			return KEY_D;
		case SDLK_e:
			return KEY_E;
		case SDLK_f:
			return KEY_F;
		case SDLK_g:
			return KEY_G;
		case SDLK_h:
			return KEY_H;
		case SDLK_i:
			return KEY_I;
		case SDLK_j:
			return KEY_J;
		case SDLK_k:
			return KEY_K;
		case SDLK_l:
			return KEY_L;
		case SDLK_m:
			return KEY_M;
		case SDLK_n:
			return KEY_N;
		case SDLK_o:
			return KEY_O;
		case SDLK_p:
			return KEY_P;
		case SDLK_q:
			return KEY_Q;
		case SDLK_r:
			return KEY_R;
		case SDLK_s:
			return KEY_S;
		case SDLK_t:
			return KEY_T;
		case SDLK_u:
			return KEY_U;
		case SDLK_v:
			return KEY_V;
		case SDLK_w:
			return KEY_W;
		case SDLK_x:
			return KEY_X;
		case SDLK_y:
			return KEY_Y;
		case SDLK_z:
			return KEY_Z;
				
		case SDLK_LEFT:
			return KEY_LEFT;
		case SDLK_RIGHT:
			return KEY_RIGHT;
		case SDLK_UP:
			return KEY_UP;
		case SDLK_DOWN:
			return KEY_DOWN;		
		case SDLK_RETURN:
			return KEY_CR;
		case SDLK_TAB:
			return KEY_TAB;
		case SDLK_INSERT:
			return KEY_INS;
		case SDLK_DELETE:
			return KEY_DEL;
		case SDLK_END:
			return KEY_END;
		case SDLK_HOME:
			return KEY_HOME;
		case SDLK_PAGEUP:
			return KEY_PAGEUP;
		case SDLK_PAGEDOWN:
			return KEY_PAGEDOWN;
		case SDLK_BACKSPACE:
			return KEY_BACKSPACE;
		case SDLK_COMMA:
			return KEY_COMMA;
		case SDLK_PERIOD:
			return KEY_FSTOP; /* fstop? */
		case SDLK_SPACE:
			return KEY_SPACE;
			
		case SDLK_LSHIFT:
			return KEY_LEFTSHIFT;
		case SDLK_RSHIFT:
			return KEY_RIGHTSHIFT;
		case SDLK_LALT:
			return KEY_LEFTALT;
		case SDLK_RALT:
			return KEY_RIGHTALT;
		case SDLK_LCTRL:
			return KEY_LEFTCTRL;
		case SDLK_RCTRL:
			return KEY_RIGHTCTRL;

		case SDLK_CAPSLOCK:
			return KEY_CAPS;
		case SDLK_NUMLOCK:
			return KEY_NUMLOCK;
		case SDLK_SCROLLOCK:
			return KEY_SCROLLOK;
			
		case SDLK_KP0:
			return KEY_NUMPAD0;
		case SDLK_KP1:
			return KEY_NUMPAD1;
		case SDLK_KP2:
			return KEY_NUMPAD2;
		case SDLK_KP3:
			return KEY_NUMPAD3;
		case SDLK_KP4:
			return KEY_NUMPAD4;
		case SDLK_KP5:
			return KEY_NUMPAD5;
		case SDLK_KP6:
			return KEY_NUMPAD6;
		case SDLK_KP7:
			return KEY_NUMPAD7;
		case SDLK_KP8:
			return KEY_NUMPAD8;
		case SDLK_KP9:
			return KEY_NUMPAD9;
		case SDLK_KP_MINUS:
			return KEY_NUMPADSUB;
		case SDLK_KP_PLUS:
			return KEY_NUMPADADD;
		case SDLK_KP_PERIOD:
			return KEY_NUMPADDEL;
		case SDLK_KP_ENTER:
			return KEY_NUMPADENTER;
		case SDLK_KP_DIVIDE:
			return KEY_NUMPADDIVIDE;
		case SDLK_KP_MULTIPLY:
			return KEY_NUMPADMULTIPLY;
	
		case SDLK_LEFTBRACKET:
			return KEY_LBRACKET;
		case SDLK_RIGHTBRACKET:
			return KEY_RBRACKET;
		case SDLK_SEMICOLON:
			return KEY_SEMICOLON;
		case SDLK_QUOTE:
			return KEY_APOSTROPHE;
		case SDLK_BACKQUOTE:
			return KEY_GRAVE;
		case SDLK_BACKSLASH:
			return KEY_BACKSLASH;
		case SDLK_SLASH:
			return KEY_SLASH;
/*		case SDLK_
			return KEY_CAPITAL; */
		case SDLK_MINUS:
			return KEY_MINUS;
		case SDLK_EQUALS:
			return KEY_EQUALS;
		case SDLK_LSUPER:
			return KEY_LWIN;
		case SDLK_RSUPER:
			return KEY_RWIN;
/*		case SDLK_
			return KEY_APPS; */
		
		case SDLK_F1:
			return KEY_F1;
		case SDLK_F2:
			return KEY_F2;
		case SDLK_F3:
			return KEY_F3;
		case SDLK_F4:
			return KEY_F4;
		case SDLK_F5:
			return KEY_F5;
		case SDLK_F6:
			return KEY_F6;
		case SDLK_F7:
			return KEY_F7;
		case SDLK_F8:
			return KEY_F8;
		case SDLK_F9:
			return KEY_F9;
		case SDLK_F10:
			return KEY_F10;
		case SDLK_F11:
			return KEY_F11;
		case SDLK_F12:
			return KEY_F12;

/* finish foreign keys */

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
#if 1
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
#endif	
}
        
void InGameFlipBuffers()
{
#if 1
	SDL_GL_SwapBuffers();
#endif	
}

void ThisFramesRenderingHasBegun()
{
/*	fprintf(stderr, "ThisFramesRenderingHasBegun()\n"); */

/* TODO: this should be in D3D_DrawBackdrop */
#if 1	
	glClear(GL_COLOR_BUFFER_BIT);
#endif	
}

void ThisFramesRenderingHasFinished()
{
/*	fprintf(stderr, "ThisFramesRenderingHasFinished()\n"); */

/* This is where the queued drawing commands' execution takes place */

	LightBlockDeallocation();
}
                
int ExitWindowsSystem()
{
#if 1
	SDL_Quit();
#endif	
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
	
	RequestedGammaSetting = 128;
	
/*	InitOptionsMenu();  NOT YET */
	
//	LoadDefaultPrimaryConfigs(); /* load the configs! yes! */
	MarineInputPrimaryConfig = DefaultMarineInputPrimaryConfig;
	PredatorInputPrimaryConfig = DefaultPredatorInputPrimaryConfig;
	AlienInputPrimaryConfig = DefaultAlienInputPrimaryConfig;
	MarineInputSecondaryConfig = DefaultMarineInputSecondaryConfig;
	PredatorInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
	AlienInputSecondaryConfig = DefaultAlienInputSecondaryConfig;
		
	LoadKeyConfiguration();
	
	
	SoundSys_Start();
	CDDA_Start();
	
	InitTextStrings();
	
	BuildMultiplayerLevelNameArray();
	
	ChangeDirectDrawObject();
	AvP.LevelCompleted = 0;
	LoadSounds("PLAYER");

	AvP.CurrentEnv = AvP.StartingEnv = 0; /* are these even used? */
	
	AvP.PlayerType = I_Alien;
	SetLevelToLoad(AVP_ENVIRONMENT_FERARCO); /* starting alien level */

//	AvP.PlayerType = I_Marine;
//	SetLevelToLoad(AVP_ENVIRONMENT_DERELICT); /* starting marine level */
//	SetLevelToLoad(AVP_ENVIRONMENT_INVASION); /* because the menus aren't implemented */


//	AvP.PlayerType = I_Predator;
//	SetLevelToLoad(AVP_ENVIRONMENT_WATERFALL); /* starting predator level */

//	SetLevelToLoad(AVP_ENVIRONMENT_LEADWORKS_MP); /* multiplayer */

//	SetLevelToLoad(AVP_ENVIRONMENT_E3DEMOSP); /* demo level */

// while(AvP_MainMenus()) {

	d3d_light_ctrl.ctrl = LCCM_NORMAL;
	d3d_overlay_ctrl.ctrl = OCCM_NORMAL;
	
	/* this was in windows SetGameVideoMode: */
	ScreenDescriptorBlock.SDB_Width     = MyWidth;
	ScreenDescriptorBlock.SDB_Height    = MyHeight;
	ScreenDescriptorBlock.SDB_CentreX   = MyWidth/2;
	ScreenDescriptorBlock.SDB_CentreY   = MyHeight/2;
	ScreenDescriptorBlock.SDB_ProjX     = MyWidth/2;
	ScreenDescriptorBlock.SDB_ProjY     = MyHeight/2;
	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = MyWidth;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = MyHeight;
	
	// GetCorrectDirectDrawObject();
	
	InitialiseGammaSettings(RequestedGammaSetting);
	
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
