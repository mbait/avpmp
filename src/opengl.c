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
#include "module.h"
#include "stratdef.h"
#include "projfont.h"
#include "krender.h"
#include "kshape.h"
#include "prototyp.h"
#include "d3d_hud.h"
#include "avp_userprofile.h"


extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int SpecialFXImageNumber;

static void *CurrTextureHandle;

#define TRANSLUCENCY_ONEONE 33
void CheckTranslucencyModeIsCorrect(int mode) /* TODO: use correct enum */
{	
	switch(RenderPolygon.TranslucencyMode) {
		case TRANSLUCENCY_OFF:
			glBlendFunc(GL_ONE, GL_ZERO);
			break;
		case TRANSLUCENCY_NORMAL:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case TRANSLUCENCY_COLOUR:
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			break;
		case TRANSLUCENCY_INVCOLOUR:
			glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			break;
		case TRANSLUCENCY_GLOWING:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case TRANSLUCENCY_DARKENINGCOLOUR:
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
			break;
		case TRANSLUCENCY_JUSTSETZ:
			glBlendFunc(GL_ZERO, GL_ONE);
			break;
		case TRANSLUCENCY_ONEONE:
			glBlendFunc(GL_ONE, GL_ONE);
			break;	
		default:
			fprintf(stderr, "RenderPolygon.TranslucencyMode: invalid %d\n", RenderPolygon.TranslucencyMode);
	}
}

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
#if 1
	int texoffset;
	void *TextureHandle;
	int i;
	GLfloat ZNear, zvalue;
//	GLflaot ZFar;
	
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
//	ZFar = 18000.0f; /* TODO: is this good enough? */
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DHandle;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
//	fprintf(stderr, "D3D_ZBufferedGouraudTexturedPolygon_Output(%p, %p)\n", inputPolyPtr, renderVerticesPtr);
//	fprintf(stderr, "\tRenderPolygon.NumberOfVertices = %d\n", RenderPolygon.NumberOfVertices);
//	fprintf(stderr, "\ttexoffset = %d (ptr = %p)\n", texoffset, texoffset ? (void *)ImageHeaderArray[texoffset].D3DHandle : CurrTextureHandle);

	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);

/*
	if (SecondaryColorExt)
		glEnable(GL_COLOR_SUM_EXT);
*/

	glBegin(GL_POLYGON);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		GLfloat x, y, z;
		int x1, y1;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
		x = x1;
		y = y1;
						
//		x =  (x - 319.0)/319.0;
//		y = -(y - 239.0)/239.0;
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		zvalue = 1.0 - 2*ZNear/zvalue; /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
//		zvalue = 2.0 * (zvalue - ZNear) / (ZFar - ZNear) - 1.0;
		z = zvalue;
		
		glColor4ub(GammaValues[vertices->R], GammaValues[vertices->G], GammaValues[vertices->B], vertices->A);
/*
		if (SecondaryColorExt)
			glSecondaryColor3ub(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB]);
*/	
		glVertex3f(x, y, z);

//		fprintf(stderr, "Vertex %d: (%f, %f, %f)\n\t[%d, %d, %d]->[%d, %d] (%d, %d, %d, %d)\n", i, x, y, z, vertices->X, vertices->Y, vertices->Z, x1, y1, vertices->R, vertices->G, vertices->B, vertices->A);
//		fprintf(stderr, "GREP: z = %d, znear = %f, zvalue = %f, z = %f\n", vertices->Z, ZNear, zvalue, z);
	}
	glEnd();
	
	CurrTextureHandle = TextureHandle;

return;
/* This *tries* to emulate SecondaryColorExt */
/*	if (!SecondaryColorExt || WantSecondaryColorHack) */ {
		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_ONEONE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		glDepthMask(GL_FALSE);
		
		glBegin(GL_POLYGON);
		for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
			RENDERVERTEX *vertices = &renderVerticesPtr[i];
			GLfloat x, y, z;
			int x1, y1;
		
			x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
			y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
			x = x1;
			y = y1;
						
			x =  (x - 320.0)/320.0;
			y = -(y - 240.0)/240.0;
		
			zvalue = vertices->Z+HeadUpDisplayZOffset;
			zvalue = 1.0 - 2*ZNear/zvalue; /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
			z = zvalue;
		
			glColor4ub(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB], 255);
			glVertex3f(x, y, z);
		}
		glEnd();
		
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);	
	}
#endif	
}

void D3D_Particle_Output(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];	
	int texoffset = SpecialFXImageNumber;
	GLfloat ZNear;
	int i;
	
	if (particleDescPtr->IsLit && !(particlePtr->ParticleID==PARTICLE_ALIEN_BLOOD && CurrentVisionMode==VISION_MODE_PRED_SEEALIENS) )
	{
		int intensity = LightIntensityAtPoint(&particlePtr->Position);
		
		if (particlePtr->ParticleID==PARTICLE_SMOKECLOUD || particlePtr->ParticleID==PARTICLE_ANDROID_BLOOD)
		{
			int r, g, b, a;
	
			r = (particlePtr->Colour >> 24) & 0xFF;
			g = (particlePtr->Colour >> 16) & 0xFF;
			b = (particlePtr->Colour >> 8)  & 0xFF;
			a = (particlePtr->Colour >> 0)  & 0xFF;
	
			glColor4ub(
				MUL_FIXED(intensity,r),
				MUL_FIXED(intensity,g),
				MUL_FIXED(intensity,g),
				a
				);
		} else {
			glColor4ub(
				MUL_FIXED(intensity,particleDescPtr->RedScale[CurrentVisionMode]),
				MUL_FIXED(intensity,particleDescPtr->GreenScale[CurrentVisionMode]),
				MUL_FIXED(intensity,particleDescPtr->BlueScale[CurrentVisionMode]),
				particleDescPtr->Alpha
				);
		}
	} else {
		int r, g, b, a;
		
		r = (particlePtr->Colour >> 24) & 0xFF;
		g = (particlePtr->Colour >> 16) & 0xFF;
		b = (particlePtr->Colour >> 8)  & 0xFF;
		a = (particlePtr->Colour >> 0)  & 0xFF;
		
		glColor4ub(r, g, b, a);
	}
	if (RAINBOWBLOOD_CHEATMODE) {
		glColor4ub(FastRandom()&255, FastRandom()&255, FastRandom()&255, particleDescPtr->Alpha);
	}
	
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	CheckTranslucencyModeIsCorrect(particleDescPtr->TranslucencyType);
	
	glBegin(GL_POLYGON);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		int x1, y1;
		GLfloat x, y, z, zvalue;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		if (x1<Global_VDB_Ptr->VDB_ClipLeft) {
			x1=Global_VDB_Ptr->VDB_ClipLeft;
		} else if (x1>Global_VDB_Ptr->VDB_ClipRight) {
			x1=Global_VDB_Ptr->VDB_ClipRight;
		}
		
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
		if (y1<Global_VDB_Ptr->VDB_ClipUp) {
			y1=Global_VDB_Ptr->VDB_ClipUp;
		} else if (y1>Global_VDB_Ptr->VDB_ClipDown) {
			y1=Global_VDB_Ptr->VDB_ClipDown;
		}
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		zvalue = 1.0 - 2*ZNear/zvalue; /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
//		zvalue = 2.0 * (zvalue - ZNear) / (ZFar - ZNear) - 1.0;
		z = zvalue;
		
		glVertex3f(x, y, z);
	}
	glEnd();
}
