#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "gamedef.h"
#include "gameplat.h"
#include "ffstdio.h"
#include "vision.h"
#include "comp_shp.h"
#include "avp_envinfo.h"
#include "cdtrackselection.h"

char LevelName[] = {"predbit6\0QuiteALongNameActually"}; /* the real way to load levels */

extern int ScanDrawMode; /* to fix image loading */
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock; /* this should be put in a header file */

PROCESSORTYPES ReadProcessorType()
{
	return PType_PentiumMMX;
}

int InitialiseWindowsSystem()
{
	ScanDrawMode = ScanDrawD3DHardwareRGB;
	
	return 0;
}

int ExitWindowsSystem()
{
	return 0;
}

int main(int argc, char *argv[])
{
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
//	Env_List[0]->main = &(ELOLevelToLoad); /* overwrite the first entry of crappy env_list with LevelName */
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
			break;
		case I_GM_Menus:
			AvP.GameMode = I_GM_Playing;
			break;
		case I_GM_Paused:
			break;
		default:
			fprintf(stderr, "AvP.MainLoopRunning: gamemode = %d\n", AvP.GameMode);
			exit(EXIT_FAILURE);
		}
		break; /* TODO -- remove when loop works */
	}
		
// }

	fprintf(stderr, "Now exiting Aliens vs Predator!  At least it didn't crash!\n");
	
	return 0;
}
