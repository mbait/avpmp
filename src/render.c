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
#include "pldghost.h"

#define UseLocalAssert Yes
#include "ourasert.h"

extern DISPLAYBLOCK *ActiveBlockList[];
extern int NumActiveBlocks;
extern int GlobalAmbience;

int LightIntensityAtPoint(VECTORCH *pointPtr)
{
	int intensity = 0;
	int i, j;
	
	DISPLAYBLOCK **activeBlockListPtr = ActiveBlockList;
	for(i = NumActiveBlocks; i != 0; i--) {
		DISPLAYBLOCK *dispPtr = *activeBlockListPtr++;
		
		if (dispPtr->ObNumLights) {
			for(j = 0; j < dispPtr->ObNumLights; j++) {
				LIGHTBLOCK *lptr = dispPtr->ObLights[j];
				VECTORCH disp = lptr->LightWorld;
				int dist;
				
				disp.vx -= pointPtr->vx;
				disp.vy -= pointPtr->vy;
				disp.vz -= pointPtr->vz;
				
				dist = Approximate3dMagnitude(&disp);
				
				if (dist<lptr->LightRange) {
					intensity += WideMulNarrowDiv(lptr->LightBright,lptr->LightRange-dist,lptr->LightRange);
				}
			}
		}
	}
	if (intensity>ONE_FIXED) intensity=ONE_FIXED;
	else if (intensity<GlobalAmbience) intensity=GlobalAmbience;
	
	/* KJL 20:31:39 12/1/97 - limit how dark things can be so blood doesn't go green */
	if (intensity<10*256) intensity = 10*256;

	return intensity;
}
