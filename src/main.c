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

char LevelName[] = {"predbit6\0QuiteALongNameActually"};
static ELO ELOLevelToLoad = {&LevelName};

extern int ScanDrawMode; /* to fix image loading */

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
	int level_to_load = I_Num_Environments;
	
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

	Env_List[0] = &(ELOLevelToLoad);
	level_to_load = 0;
	
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

	AvP.CurrentEnv = AvP.StartingEnv = 0; /* ??? */
	SetLevelToLoad(AVP_ENVIRONMENT_INVASION); /* because the menus aren't implemented */
	 
// while(AvP_MainMenus()) {

	d3d_light_ctrl.ctrl = LCCM_NORMAL;
	d3d_overlay_ctrl.ctrl = OCCM_NORMAL;
	
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
	return 0;
}
