#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "inline.h"
#include "gamedef.h"
#include "module.h"
#include "stratdef.h"
#include "projfont.h"
#include "krender.h"
#include "kshape.h"
#include "prototyp.h"
#include "d3d_hud.h"
#include "bh_types.h"
#include "equipmnt.h"
#include "bh_marin.h"
#include "bh_alien.h"
#include "pldghost.h"

#define UseLocalAssert Yes
#include "ourasert.h"

DISPLAYBLOCK PlayersMirrorImage;
STRATEGYBLOCK PlayersMirrorImageSB;		  
NETGHOSTDATABLOCK PlayersMirrorGhost;
DYNAMICSBLOCK PlayersMirrorDynBlock;

extern int LastHand;

/* support function for addnetmsg_playerstate() */	
static int MyPlayerHasAMuzzleFlash(STRATEGYBLOCK *sbPtr)
{
	PLAYER_WEAPON_DATA *weaponPtr;
	TEMPLATE_WEAPON_DATA *twPtr;
 	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	LOCALASSERT(playerStatusPtr);
    	        
    weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
		
	/* first check if we are displaying a muzle flash ourselves */
	if(twPtr->MuzzleFlashShapeName == NULL) return 0;
	if(twPtr->PrimaryIsMeleeWeapon) return 0;
	
	if (weaponPtr->WeaponIDNumber==WEAPON_TWO_PISTOLS) {
		if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY) {
			if (LastHand) {
				return 2;
			} else {
				return 1;
			}
		} else if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY) {
			if (LastHand) {
				return 2;
			} else {
				return 1;
			}
			return 0;
		}
	}
	
	if (weaponPtr->WeaponIDNumber==WEAPON_MARINE_PISTOL) {
		if ((weaponPtr->CurrentState==WEAPONSTATE_FIRING_PRIMARY) 
			||(weaponPtr->CurrentState==WEAPONSTATE_FIRING_SECONDARY)) {
			//ReleasePrintDebuggingText("Pistol Muzzle Flash 1\n");
			return 1;
		} else {
			//ReleasePrintDebuggingText("Pistol Muzzle Flash 0\n");
			return 0;
		}
	}
	
	if (weaponPtr->CurrentState != WEAPONSTATE_FIRING_PRIMARY) {
		return 0;
	}

	/* even if we are displaying our own muzzle flash, we don't neccessarily want it to
	be visible to other players (because it looks stupid) */
	if((weaponPtr->WeaponIDNumber==WEAPON_PULSERIFLE)||
	   (weaponPtr->WeaponIDNumber==WEAPON_MARINE_PISTOL)||
	   (weaponPtr->WeaponIDNumber==WEAPON_AUTOSHOTGUN)||
	   (weaponPtr->WeaponIDNumber==WEAPON_SMARTGUN)||
	   (weaponPtr->WeaponIDNumber==WEAPON_MINIGUN)||
	   (weaponPtr->WeaponIDNumber==WEAPON_FRISBEE_LAUNCHER)||
	   (weaponPtr->WeaponIDNumber==WEAPON_PRED_PISTOL)||
	   (weaponPtr->WeaponIDNumber==WEAPON_PRED_RIFLE))
	{
		/* if we get this far, we want to display a muzzle flash */
		return 1;		
	}
	return 0;
}

/* Patrick 11/7/97 ----------------------------------------------
Functions for determining our sequence for player update messages
-----------------------------------------------------------------*/

static MARINE_SEQUENCE GetMyMarineSequence(void)
{
	int playerIsMoving = 0;
	int playerIsFiring = 0;
	int playerIsCrouching = 0;
	int playerIsAlive = 0;
	int playerIsJumping = 0;
	int usingCloseAttackWeapon;
	extern int StaffAttack;		
		
	/* sort out what state we're in */
	if(PlayerStatusPtr->IsAlive) playerIsAlive = 1;
	else playerIsAlive = 0;					

	if (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward) {
		playerIsMoving=-1;
	} else if((PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight)) {
	   playerIsMoving = 1;
	} else {
		playerIsMoving = 0;
	}

	if ( (Player->ObStrategyBlock->DynPtr->Position.vx==Player->ObStrategyBlock->DynPtr->PrevPosition.vx)
		&& (Player->ObStrategyBlock->DynPtr->Position.vy==Player->ObStrategyBlock->DynPtr->PrevPosition.vy)
		&& (Player->ObStrategyBlock->DynPtr->Position.vz==Player->ObStrategyBlock->DynPtr->PrevPosition.vz) ) {
		/* Actually not moving - overruled! */
		playerIsMoving=0;
	}

	if(PlayerStatusPtr->ShapeState!=PMph_Standing) 
	{
		playerIsCrouching = 1;
	}
	else 
	{
		playerIsCrouching = 0;		
	}
	
	if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_PRIMARY)||
	   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_PRIMARY)) {
			playerIsFiring = 1;
	} else {
		if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_MARINE_PISTOL) {
			if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_SECONDARY)||
			   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_SECONDARY)) {
				playerIsFiring = 1;
			} else {
				playerIsFiring = 0;
			}
		} else {
			playerIsFiring = 0;
		}
	}

	if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_CUDGEL)
		usingCloseAttackWeapon = 1;
	else usingCloseAttackWeapon = 0;	

	/* Fix cudgel. */
	if (usingCloseAttackWeapon) {
		if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_SECONDARY)||
		   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_SECONDARY)) {
			playerIsFiring = 1;
		}
	}

	/* KJL 14:27:14 10/29/97 - deal with jumping & falling */
	{
		DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
		if (!dynPtr->IsInContactWithFloor && (dynPtr->TimeNotInContactWithFloor==0))
			playerIsJumping=1;
	}	

	/* and deduce the sequence */
	if(playerIsAlive==0) 
	{
		if(playerIsCrouching) {
			return MSQ_CrouchDie;
		} else {
			return MSQ_StandDieFront;
		}
	}

	if(playerIsJumping) {
		return MSQ_Jump;
	}

	/* Put this in here... no running cudgel attacks yet. */
	if(playerIsFiring&&usingCloseAttackWeapon) {
		/* Deal with cudgel case! */
		if (StaffAttack>=0) {
			return(MSQ_BaseOfCudgelAttacks+StaffAttack);
		}
	}

	if(playerIsCrouching)
	{
		if(playerIsMoving>0) {
			return MSQ_Crawl;
		} else if (playerIsMoving<0) {
			return MSQ_Crawl_Backwards;
		} else {
			return MSQ_Crouch;
		}
	}

	if(playerIsMoving>0)
	{
		if(playerIsFiring) {
			if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_TWO_PISTOLS)
				&&(LastHand)) {
				return MSQ_RunningFireSecondary;
			} else {
				return MSQ_RunningFire;
			}
		} else {
			return MSQ_Walk;
		}
	} else if (playerIsMoving<0) {
		if(playerIsFiring) {
			if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_TWO_PISTOLS)
				&&(LastHand)) {
				return MSQ_RunningFireSecondary_Backwards;
			} else {
				return MSQ_RunningFire_Backwards;
			}
		} else {
			return MSQ_Walk_Backwards;
		}
	}

	if(playerIsFiring) {
		if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_TWO_PISTOLS)
			&&(LastHand)) {
			return MSQ_StandingFireSecondary;
		} else {
			return MSQ_StandingFire;
		}
	} else {
		if (PlayerStatusPtr->tauntTimer!=0) {
			return MSQ_Taunt;
		} else {
			return MSQ_Stand;
		}
	}
} 

static ALIEN_SEQUENCE GetMyAlienSequence(void)
{
	extern STRATEGYBLOCK *Biting;
	extern int Bit;
	int playerIsMoving = 0;
	int playerIsFiring = 0;
	int playerIsCrouching = 0;
	int playerIsAlive = 0;
	int playerIsJumping = 0;
		
	/* sort out what state we're in */
	if(PlayerStatusPtr->IsAlive) playerIsAlive = 1;
	else playerIsAlive = 0;					

	if (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward) {
		playerIsMoving =-1;
	} else if ((PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight)) {
	   	playerIsMoving = 1;
	} else {
		playerIsMoving = 0;
	}

	if ( (Player->ObStrategyBlock->DynPtr->Position.vx==Player->ObStrategyBlock->DynPtr->PrevPosition.vx)
		&& (Player->ObStrategyBlock->DynPtr->Position.vy==Player->ObStrategyBlock->DynPtr->PrevPosition.vy)
		&& (Player->ObStrategyBlock->DynPtr->Position.vz==Player->ObStrategyBlock->DynPtr->PrevPosition.vz) ) {
		/* Actually not moving - overruled! */
		playerIsMoving=0;
	}

	if(PlayerStatusPtr->ShapeState!=PMph_Standing) playerIsCrouching = 1;
	else playerIsCrouching = 0;		

	/* ChrisF 20/4/98: playerIsFiring now specifies alien weapon behaviour. */
	//if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_PRIMARY)||
	//   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_PRIMARY))
	//		playerIsFiring = 1;
	//else playerIsFiring = 0;	
	//
	//if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber!=WEAPON_ALIEN_SPIT) {
	//	usingCloseAttackWeapon = 1;
	//} else {
	//	usingCloseAttackWeapon = 0;
	//}

	switch(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState) {
		case (WEAPONSTATE_FIRING_PRIMARY):
			if(Biting) {
				playerIsFiring=4; //Eat.
			} else {
				playerIsFiring=1; //Claw.
			}
			break;
		case (WEAPONSTATE_FIRING_SECONDARY):
			playerIsFiring=2; //Tail Poise.
			break;
		case (WEAPONSTATE_RECOIL_SECONDARY):
			playerIsFiring=3; //Tail Strike.
			break;
		default:
			playerIsFiring=0; //Nothing.
			break;
	}
			

	/* KJL 14:27:14 10/29/97 - deal with jumping & falling */
	{
		DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
		if (!dynPtr->IsInContactWithFloor && (dynPtr->TimeNotInContactWithFloor==0))
			playerIsJumping=1;
	}	

	/* and deduce the sequence */
	if(playerIsAlive==0) 
	{
		return ASQ_Stand; /* kind of irrelevant really */
	}

	if(playerIsJumping) /* TODO: consider jump & crouch */
	{
		switch(playerIsFiring) {
			case 1:
				return ASQ_Pounce;
				break;
			case 2:
				return ASQ_JumpingTailPoise;
				break;
			case 3:
				return ASQ_JumpingTailStrike;
				break;
			case 4:
				/* What the hell? */
				return ASQ_Eat;
				break;
			default:
				return ASQ_Jump;
				break;
		}
	}

	if(playerIsCrouching)
	{
		if(playerIsMoving>0)
		{
			switch(playerIsFiring) {
				case 1:
					return ASQ_CrawlingAttack_Claw;
					break;
				case 2:
					return ASQ_CrawlingTailPoise;
					break;
				case 3:
					return ASQ_CrawlingTailStrike;
					break;
				case 4:
					/* What the hell? */
					return ASQ_CrouchEat;
					break;
				default:
					if(Player->ObStrategyBlock->DynPtr->OrientMat.mat22>50000)
						return ASQ_Scamper;
					else
						return ASQ_Crawl;
					break;
			}
		} else if(playerIsMoving<0)	{
			switch(playerIsFiring) {
				case 1:
					return ASQ_CrawlingAttack_Claw_Backwards;
					break;
				case 2:
					return ASQ_CrawlingTailPoise_Backwards;
					break;
				case 3:
					return ASQ_CrawlingTailStrike_Backwards;
					break;
				case 4:
					/* What the hell? */
					return ASQ_CrouchEat;
					break;
				default:
					if(Player->ObStrategyBlock->DynPtr->OrientMat.mat22>50000)
						return ASQ_Scamper_Backwards;
					else
						return ASQ_Crawl_Backwards;
					break;
			}
		} 		
		
		switch(playerIsFiring) {
			case 1:
				return ASQ_CrouchedAttack_Claw;
				break;
			case 2:
				return ASQ_CrouchedTailPoise;
				break;
			case 3:
				return ASQ_CrouchedTailStrike;
				break;
			case 4:
				return ASQ_Eat;
				break;
			default:
				return ASQ_Crouch;
				break;
		}
	}
	
	if(playerIsMoving>0)
	{
		switch(playerIsFiring) {
			case 1:
				return ASQ_RunningAttack_Claw;
				break;
			case 2:
				return ASQ_RunningTailPoise;
				break;
			case 3:
				return ASQ_RunningTailStrike;
				break;
			case 4:
				/* What the hell? */
				return ASQ_Eat;
				break;
			default:
				return ASQ_Run;
				break;
		}
	} else if(playerIsMoving<0) {
		switch(playerIsFiring) {
			case 1:
				return ASQ_RunningAttack_Claw_Backwards;
				break;
			case 2:
				return ASQ_RunningTailPoise_Backwards;
				break;
			case 3:
				return ASQ_RunningTailStrike_Backwards;
				break;
			case 4:
				/* What the hell? */
				return ASQ_Eat;
				break;
			default:
				return ASQ_Run_Backwards;
				break;
		}
	} 		

	switch(playerIsFiring) {
		case 1:
			return ASQ_StandingAttack_Claw;
			break;
		case 2:
			return ASQ_StandingTailPoise;
			break;
		case 3:
			return ASQ_StandingTailStrike;
			break;
		case 4:
			return ASQ_Eat;
			break;
		default:
			if (PlayerStatusPtr->tauntTimer!=0) {
				/* Second lowest priority ever. */
				return ASQ_Taunt;
			} else {
				return ASQ_Stand;
			}
			break;
	}

} 

static PREDATOR_SEQUENCE GetMyPredatorSequence(void)
{
	int playerIsMoving = 0;
	int playerIsFiring = 0;
	int playerIsCrouching = 0;
	int playerIsAlive = 0;
	int playerIsJumping = 0;
	int usingCloseAttackWeapon;
	extern int StaffAttack;		

	/* sort out what state we're in */
	if(PlayerStatusPtr->IsAlive) playerIsAlive = 1;
	else playerIsAlive = 0;					

	if (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Backward) {
		playerIsMoving=-1;
	} else if((PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_Forward)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepLeft)||
	   (PlayerStatusPtr->Mvt_InputRequests.Flags.Rqst_SideStepRight)) {
	   	playerIsMoving = 1;
	} else {
		playerIsMoving = 0;
	}

	if ( (Player->ObStrategyBlock->DynPtr->Position.vx==Player->ObStrategyBlock->DynPtr->PrevPosition.vx)
		&& (Player->ObStrategyBlock->DynPtr->Position.vy==Player->ObStrategyBlock->DynPtr->PrevPosition.vy)
		&& (Player->ObStrategyBlock->DynPtr->Position.vz==Player->ObStrategyBlock->DynPtr->PrevPosition.vz) ) {
		/* Actually not moving - overruled! */
		playerIsMoving=0;
	}

	if(PlayerStatusPtr->ShapeState!=PMph_Standing) playerIsCrouching = 1;
	else playerIsCrouching = 0;		

	playerIsFiring = 0;	
	
	if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_PRED_SHOULDERCANNON)
	{
		//the shoulder cannon is fired during recoil (I think)
		if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_PRIMARY)
		{
			playerIsFiring = 1;
		}
	}
	else if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_PRED_WRISTBLADE)
	{
		if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_PRIMARY)||
		   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_PRIMARY) ||
		   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_SECONDARY))
		{
			if(StaffAttack!=-1)
			{
				playerIsFiring = 1;
			}
		}
	}
	else
	{
		if((PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_FIRING_PRIMARY)||
		   (PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].CurrentState==WEAPONSTATE_RECOIL_PRIMARY))
		{
				playerIsFiring = 1;
		}
	}

	if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_PRED_WRISTBLADE)
		usingCloseAttackWeapon = 3;
	else if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_PRED_DISC)
		usingCloseAttackWeapon = 1;
	else if(PlayerStatusPtr->WeaponSlot[PlayerStatusPtr->SelectedWeaponSlot].WeaponIDNumber==WEAPON_PRED_STAFF)
		usingCloseAttackWeapon = 2;
	else usingCloseAttackWeapon = 0;	

	/* KJL 14:27:14 10/29/97 - deal with jumping & falling */
	{
		DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
		if (!dynPtr->IsInContactWithFloor && (dynPtr->TimeNotInContactWithFloor==0))
			playerIsJumping=1;
	}	

	/* and deduce the sequence */
	if(playerIsAlive==0) 
	{
		if(playerIsCrouching) {
			return PredSQ_CrouchDie;
		} else {
			return PredSQ_StandDie;
		}
	}

	if(playerIsJumping) {
		return(PredSQ_Jump);
	}

	if(playerIsCrouching)
	{
		if(playerIsMoving>0)
		{
			if(playerIsFiring&&usingCloseAttackWeapon) {
				/* Deal with staff case! */
				if (usingCloseAttackWeapon==2) {
					if (StaffAttack>=0) {
						return(PredSQ_BaseOfStaffAttacks+StaffAttack);
					}
				} else if (usingCloseAttackWeapon==3) {
					if (StaffAttack>=0) {
						return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
					}
				}
				return PredSQ_CrawlingSwipe;
			} else {
				return PredSQ_Crawl;
			}
		} else if (playerIsMoving<0) {
			if(playerIsFiring&&usingCloseAttackWeapon) {
				/* Deal with staff case! */
				if (usingCloseAttackWeapon==2) {
					if (StaffAttack>=0) {
						return(PredSQ_BaseOfStaffAttacks+StaffAttack);
					}
				} else if (usingCloseAttackWeapon==3) {
					if (StaffAttack>=0) {
						return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
					}
				}
				return PredSQ_CrawlingSwipe_Backwards;
			} else {
				return PredSQ_Crawl_Backwards;
			}
		}
		if(playerIsFiring&&usingCloseAttackWeapon) {
			/* Deal with staff case! */
			if (usingCloseAttackWeapon==2) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfStaffAttacks+StaffAttack);
				}
			} else if (usingCloseAttackWeapon==3) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
				}
			}
			return PredSQ_CrouchedSwipe;
		} else {
			return PredSQ_Crouch;
		}
	}
	
	if(playerIsMoving>0)
	{
		if(playerIsFiring&&usingCloseAttackWeapon) {
			/* Deal with staff case! */
			if (usingCloseAttackWeapon==2) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfStaffAttacks+StaffAttack);
				}
			} else if (usingCloseAttackWeapon==3) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
				}
			}
			return PredSQ_RunningSwipe;
		} else {
			return PredSQ_Run;
		}
	} else if (playerIsMoving<0) {
		if(playerIsFiring&&usingCloseAttackWeapon) {
			/* Deal with staff case! */
			if (usingCloseAttackWeapon==2) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfStaffAttacks+StaffAttack);
				}
			} else if (usingCloseAttackWeapon==3) {
				if (StaffAttack>=0) {
					return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
				}
			}
			return PredSQ_RunningSwipe_Backwards;
		} else {
			return PredSQ_Run_Backwards;
		}
	}

	if(playerIsFiring&&usingCloseAttackWeapon) {
		/* Deal with staff case! */
		if (usingCloseAttackWeapon==2) {
			if (StaffAttack>=0) {
				return(PredSQ_BaseOfStaffAttacks+StaffAttack);
			}
		} else if (usingCloseAttackWeapon==3) {
			if (StaffAttack>=0) {
				return(PredSQ_BaseOfWristbladeAttacks+StaffAttack);
			}
		}
		return PredSQ_StandingSwipe;
	} else {
		if (PlayerStatusPtr->tauntTimer!=0) {
			return PredSQ_Taunt;
		} else {
			return PredSQ_Stand;
		}
	}
}

BOOL Current_Level_Requires_Mirror_Image()
{
	extern char LevelName[];
	if ( (!stricmp(LevelName,"e3demo")) || (!stricmp(LevelName,"e3demosp")) || (!stricmp(LevelName,"derelict")) )
	{
 		return TRUE;
	}
	return FALSE;
}


void CreatePlayersImageInMirror(void)
{
	AVP_BEHAVIOUR_TYPE type;
	STRATEGYBLOCK *sbPtr = &PlayersMirrorImageSB;
	NETGHOSTDATABLOCK *ghostData = &PlayersMirrorGhost; 
	PlayersMirrorImage.ObStrategyBlock = sbPtr;
	
	sbPtr->SBdptr = &PlayersMirrorImage;

	sbPtr->SBdataptr = (void *)ghostData;
	sbPtr->DynPtr = &PlayersMirrorDynBlock;
	
	switch(AvP.PlayerType)
	{
		case(I_Marine):
		{
			type = I_BehaviourMarinePlayer;
			break;
		}
		case(I_Predator):
		{
			type = I_BehaviourPredatorPlayer;
			break;
		}
		case(I_Alien):
		{
			type = I_BehaviourAlienPlayer;
			break;
		}
	}
	ghostData->type = type;
	ghostData->IOType=IOT_Non;
	ghostData->subtype=0;
	ghostData->myGunFlash = NULL;
	ghostData->SoundHandle = SOUND_NOACTIVEINDEX;
	ghostData->currentAnimSequence = 0;
	ghostData->CloakingEffectiveness = 0;
	ghostData->IgnitionHandshaking = 0;
	ghostData->soundStartFlag = 0;
	
	if(AvP.Network == I_No_Network)
	{
		ghostData->playerId=0;
	}
	else
	{
//		ghostData->playerId=AVPDPNetID;
		fprintf(stderr, "CreatePlayersImageInMirror: ghostData->playerId=AVPDPNetID\n");
	}

	/* set the shape */

	switch(type)
	{
		case I_BehaviourMarinePlayer:
		{
			CreateMarineHModel(ghostData,WEAPON_PULSERIFLE);
			break;
		}
		case I_BehaviourAlienPlayer:
		{
			CreateAlienHModel(ghostData);
			break;
		}
		case I_BehaviourPredatorPlayer:
		{
			CreatePredatorHModel(ghostData,WEAPON_PRED_WRISTBLADE);
			break;
		}
		default:
			break;
	}

	sbPtr->SBdptr->HModelControlBlock=&ghostData->HModelController;
	ProveHModel(sbPtr->SBdptr->HModelControlBlock,sbPtr->SBdptr);
}

void DeallocatePlayersMirrorImage()
{
#if MIRRORING_ON
	if(Current_Level_Requires_Mirror_Image())
	{
		Dispel_HModel(&PlayersMirrorGhost.HModelController);
	}
#endif
}

void RenderPlayersImageInMirror(void)
{
	STRATEGYBLOCK *sbPtr = &PlayersMirrorImageSB;
	NETGHOSTDATABLOCK *ghostData = &PlayersMirrorGhost; 
	
	int sequence;
	int weapon;
	int firingPrimary;
	int firingSecondary;

	switch(AvP.PlayerType)
	{
		case I_Marine:
		{
			sequence = (unsigned char)GetMyMarineSequence();
			//check for change of charcter type
			if(ghostData->type!=I_BehaviourMarinePlayer)
			{
				ghostData->type=I_BehaviourMarinePlayer;
				//settings currentweapon to -1 will forec the hmodel to be updated
				ghostData->CurrentWeapon=-1;
			}
			
			break;
		}
		case I_Predator:
		{
			sequence = (unsigned char)GetMyPredatorSequence();
			//check for change of charcter type
			if(ghostData->type!=I_BehaviourPredatorPlayer)
			{
				ghostData->type=I_BehaviourPredatorPlayer;
				//settings currentweapon to -1 will forec the hmodel to be updated
				ghostData->CurrentWeapon=-1;
			}
			break;
		}
		case I_Alien:
		{
			sequence = (unsigned char)GetMyAlienSequence();
			//check for change of charcter type
			if(ghostData->type!=I_BehaviourAlienPlayer)
			{
				ghostData->type=I_BehaviourAlienPlayer;
				//setting currentweapon to -1 will force the hmodel to be updated
				ghostData->CurrentWeapon=-1;
			}
			break;
		}
		default:
		{
			LOCALASSERT(1==0);
			break;
		}
	}
		/* my current weapon id, and whether I am firing it... */
	{
		PLAYER_WEAPON_DATA *weaponPtr;
 		PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
		LOCALASSERT(playerStatusPtr);    	        
		weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
		weapon = (signed char)(weaponPtr->WeaponIDNumber);	

		if((weaponPtr->CurrentState==WEAPONSTATE_FIRING_PRIMARY)&&(playerStatusPtr->IsAlive))		
			firingPrimary = 1;
		else firingPrimary = 0;
		if((weaponPtr->CurrentState==WEAPONSTATE_FIRING_SECONDARY)&&(playerStatusPtr->IsAlive)) 
			firingSecondary = 1;
		else firingSecondary = 0;
	}

//		if(!(((!(messagePtr->IAmAlive)))&&(netGameData.playerData[playerIndex].characterType==NGCT_Alien)))
	{
		{
			PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
			HandleWeaponElevation(sbPtr,playerStatusPtr->ViewPanX,weapon);
		
			UpdateGhost(sbPtr,&(Player->ObStrategyBlock->DynPtr->Position),&(Player->ObStrategyBlock->DynPtr->OrientEuler),sequence,AreTwoPistolsInTertiaryFire());


			MaintainGhostCloakingStatus(sbPtr,(int)playerStatusPtr->cloakOn);
		}
	}
	{
		extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
		DISPLAYBLOCK *dPtr = &PlayersMirrorImage;
		dPtr->ObWorld = PlayersMirrorDynBlock.Position;
		dPtr->ObMat = PlayersMirrorDynBlock.OrientMat;	
		ReflectObject(dPtr);	 

		PlayersMirrorImage.ObStrategyBlock = 0;
		
		AddShape(dPtr,Global_VDB_Ptr);
		PlayersMirrorImage.ObStrategyBlock = &PlayersMirrorImageSB;
	
	}
	HandleGhostGunFlashEffect(sbPtr,MyPlayerHasAMuzzleFlash(sbPtr));
}
