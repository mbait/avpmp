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
#include "aw.h"


extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int SpecialFXImageNumber;

static D3DTexture *CurrTextureHandle;


#define TRANSLUCENCY_ONEONE 33

static enum TRANSLUCENCY_TYPE CurrentTranslucencyMode = TRANSLUCENCY_OFF; /* opengl state variable */
static GLuint CurrentlyBoundTexture = 0; /* opengl state variable */

static void CheckBoundTextureIsCorrect(GLuint tex)
{
	if (tex == CurrentlyBoundTexture)
		return;

	glBindTexture(GL_TEXTURE_2D, tex);
	
	CurrentlyBoundTexture = tex;
}
		
static void CheckTranslucencyModeIsCorrect(enum TRANSLUCENCY_TYPE mode)
{	
	if (CurrentTranslucencyMode == mode)
		return;

	switch(mode) {
		case TRANSLUCENCY_OFF:
			if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE) {
			//	glBlendMode(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
			/* TODO: this may not be properly set... */
			} else {
				glDisable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ZERO);
			}
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
			return;
	}
	
	if (mode != TRANSLUCENCY_OFF && CurrentTranslucencyMode == TRANSLUCENCY_OFF)
		glEnable(GL_BLEND);
		
	CurrentTranslucencyMode = mode;
}

static void SelectPolygonBeginType(int points)
{
/* switch code that uses this to a triangle only drawer */
	switch(points) {
		case 1:
			glBegin(GL_POINTS);
			break;
		case 2:
			glBegin(GL_LINES);
			break;
		case 3:
			glBegin(GL_TRIANGLES);
			break;
		case 4:
			glBegin(GL_QUADS);
			break;
		default:
			glBegin(GL_POLYGON);
			break;
	}
}

GLuint CreateOGLTexture(D3DTexture *tex, unsigned char *buf)
{
	GLuint h;
	
	glGenTextures(1, &h);

/* TODO: d3d code doesn't explicitly enable repeating but some levels (namely predator beginning level waterfall) have clamped textures */
	glBindTexture(GL_TEXTURE_2D, h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	tex->id = h;
	
	glBindTexture(GL_TEXTURE_2D, CurrentlyBoundTexture); /* restore current */
	
	return h;
}

/* ** */

void FlushD3DZBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void SecondFlushD3DZBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void D3D_DecalSystem_Setup()
{
	glDepthMask(GL_FALSE);
}

void D3D_DecalSystem_End()
{
	glDepthMask(GL_TRUE);	
}

/* ** */

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	GLfloat ZNear, zvalue;
	float RecipW, RecipH;

		
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DTexture;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);

/*
	if (SecondaryColorExt)
		glEnable(GL_COLOR_SUM_EXT);
*/
	RecipW = (1.0f/65536.0f)/128.0f;
	RecipH = (1.0f/65536.0f)/128.0f;
	
	CheckBoundTextureIsCorrect(TextureHandle->id);
	
	// glBegin(GL_POLYGON);
	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		GLfloat x, y, z;
		int x1, y1;
		GLfloat s, t;
		GLfloat rhw = 1.0/(float)vertices->Z;
		
		s = ((float)vertices->U) * RecipW + (1.0f/256.0f);
		t = ((float)vertices->V) * RecipH + (1.0f/256.0f);

//		if (s < 0.0 || t < 0.0 || s >= 1.0 || t >= 1.0)
//			fprintf(stderr, "HEY! s = %f, t = %f (%d, %d)\n", s, t, vertices->U, vertices->V);
			
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

#if 0
		if (x1<Global_VDB_Ptr->VDB_ClipLeft) {
			x1=Global_VDB_Ptr->VDB_ClipLeft;
		} else if (x1>Global_VDB_Ptr->VDB_ClipRight) {
			x1=Global_VDB_Ptr->VDB_ClipRight;
		}
		
		if (y1<Global_VDB_Ptr->VDB_ClipUp) {
			y1=Global_VDB_Ptr->VDB_ClipUp;
		} else if (y1>Global_VDB_Ptr->VDB_ClipDown) {
			y1=Global_VDB_Ptr->VDB_ClipDown;
		}
#endif
		x = x1;
		y = y1;
						
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;

		glColor4ub(GammaValues[vertices->R], GammaValues[vertices->G], GammaValues[vertices->B], vertices->A);
/*
		if (SecondaryColorExt)
			glSecondaryColor3ub(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB]);
*/	

	/* they both work. */
#if 0		
		glTexCoord4f(s*rhw, t*rhw, 0, rhw);
		glVertex3f(x, y, z);
#else
		glTexCoord2f(s, t);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
#endif
		
	}
	glEnd();
	
	CurrTextureHandle = TextureHandle;

#if 0
/* note: the specular color doesn't seem to be enabled in the original d3d code? */
/* couldn't find the feature in the voodoo 1 specs (the minimum required card) */
/* This *tries* to emulate SecondaryColorExt */
/*	if (!SecondaryColorExt || WantSecondaryColorHack) */ {
		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_ONEONE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		
		glBegin(GL_POLYGON);
		for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
			RENDERVERTEX *vertices = &renderVerticesPtr[i];
			GLfloat x, y, z;
			int x1, y1;
		
			x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
			y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
			x = x1;
			y = y1;
						
			x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
			y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;		
			
			zvalue = vertices->Z+HeadUpDisplayZOffset;
			zvalue = 1.0 - 2*ZNear/zvalue; /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
			z = zvalue;
		
			glColor4b(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB], 255);
			glVertex3f(x, y, z);
		}
		glEnd();
		
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glEnable(GL_TEXTURE_2D);
	}
#endif	
}

void D3D_Particle_Output(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];
	int texoffset = SpecialFXImageNumber;
	GLfloat ZNear;
	int i;
	float RecipW, RecipH;
	
	D3DTexture *TextureHandle;

	TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
	
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	CheckBoundTextureIsCorrect(TextureHandle->id);
	CheckTranslucencyModeIsCorrect(particleDescPtr->TranslucencyType);

//	if(ImageHeaderArray[texoffset].ImageWidth==256) {
	if (TextureHandle->w == 256) {
		RecipW = 1.0 / 256.0;
	} else {
//		float width = (float) ImageHeaderArray[texoffset].ImageWidth;
		float width = (float) TextureHandle->w;
		
		RecipW = (1.0 / width);
	}
	
//	if(ImageHeaderArray[texoffset].ImageHeight==256) {
	if (TextureHandle->h == 256) {
		RecipH = 1.0 / 256.0;
	} else {
//		float height = (float) ImageHeaderArray[texoffset].ImageHeight;
		float height = (float) TextureHandle->h;
		
		RecipH = (1.0 / height);
	}
	
	if (particleDescPtr->IsLit && !(particlePtr->ParticleID==PARTICLE_ALIEN_BLOOD && CurrentVisionMode==VISION_MODE_PRED_SEEALIENS) )
	{
		int intensity = LightIntensityAtPoint(&particlePtr->Position);
		
		if (particlePtr->ParticleID==PARTICLE_SMOKECLOUD || particlePtr->ParticleID==PARTICLE_ANDROID_BLOOD)
		{
			int r, g, b, a;
	
			/* this should be OK. (ColourComponents was RGBA while RGBA_MAKE is BGRA (little endian) */
			r = (particlePtr->Colour >> 0)  & 0xFF;
			g = (particlePtr->Colour >> 8)  & 0xFF;
			b = (particlePtr->Colour >> 16) & 0xFF;
			a = (particlePtr->Colour >> 24) & 0xFF;
	
			glColor4ub(
				MUL_FIXED(intensity,r),
				MUL_FIXED(intensity,g),
				MUL_FIXED(intensity,b),
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
		
		b = (particlePtr->Colour >> 0)  & 0xFF;
		g = (particlePtr->Colour >> 8)  & 0xFF;
		r = (particlePtr->Colour >> 16) & 0xFF;
		a = (particlePtr->Colour >> 24) & 0xFF;
		
		glColor4ub(r, g, b, a);
	}
	if (RAINBOWBLOOD_CHEATMODE) {
		glColor4ub(FastRandom()&255, FastRandom()&255, FastRandom()&255, particleDescPtr->Alpha);
	}

	//glBegin(GL_POLYGON);
	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		int x1, y1;
		GLfloat x, y, z;
		GLfloat s, t;
		GLfloat rhw = 1/(float)vertices->Z;
		
		s = ((float)(vertices->U>>16)+.5) * RecipW;
		t = ((float)(vertices->V>>16)+.5) * RecipH;
		
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

#if 0
		if (x1<Global_VDB_Ptr->VDB_ClipLeft) {
			x1=Global_VDB_Ptr->VDB_ClipLeft;
		} else if (x1>Global_VDB_Ptr->VDB_ClipRight) {
			x1=Global_VDB_Ptr->VDB_ClipRight;
		}
				
		if (y1<Global_VDB_Ptr->VDB_ClipUp) {
			y1=Global_VDB_Ptr->VDB_ClipUp;
		} else if (y1>Global_VDB_Ptr->VDB_ClipDown) {
			y1=Global_VDB_Ptr->VDB_ClipDown;
		}
#endif
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		if (particleDescPtr->IsDrawnInFront) {
			z = -1.0f;
		} else if (particleDescPtr->IsDrawnAtBack) {
			z = 1.0f;
		} else {
			z = 1.0 - 2*ZNear/((float)vertices->Z); /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
		}

#if 0		
		glTexCoord4f(s*rhw, t*rhw, 0, rhw);
		glVertex3f(x, y, z);
#else
		glTexCoord2f(s, t);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
#endif		
	}
	glEnd();
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	float ZNear;
	int i;
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	CheckBoundTextureIsCorrect(0); /* disable texturing */
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_OFF);
	
	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		int x1, y1;
		GLfloat x, y, z;
		float rhw, zvalue;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

#if 0
		if (x1<Global_VDB_Ptr->VDB_ClipLeft) {
			x1=Global_VDB_Ptr->VDB_ClipLeft;
		} else if (x1>Global_VDB_Ptr->VDB_ClipRight) {
			x1=Global_VDB_Ptr->VDB_ClipRight;
		}
				
		if (y1<Global_VDB_Ptr->VDB_ClipUp) {
			y1=Global_VDB_Ptr->VDB_ClipUp;
		} else if (y1>Global_VDB_Ptr->VDB_ClipDown) {
			y1=Global_VDB_Ptr->VDB_ClipDown;
		}
#endif
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
		
//		zvalue = vertices->Z+HeadUpDisplayZOffset;
//		zvalue = ((zvalue-ZNear)/zvalue);
		
		rhw = 1.0/(float)vertices->Z;
		
		glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
	}
	glEnd();
}

void D3D_PredatorScreenInversionOverlay()
{
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_DARKENINGCOLOUR);
	CheckBoundTextureIsCorrect(0);
	glDepthFunc(GL_ALWAYS);
	
	SelectPolygonBeginType(3); /* triangles */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f,  1.0f, 1.0f);
	
	glVertex3f( 1.0f, -1.0f, 0.0f);
	glVertex3f( 1.0f,  1.0f, 0.0f);
	glVertex3f(-1.0f,  1.0f, 0.0f);
	
	glEnd();
	
	glDepthFunc(GL_LEQUAL);
}
