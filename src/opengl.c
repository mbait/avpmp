#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "module.h"
#include "stratdef.h"
#include "projfont.h"
#include "krender.h"
#include "kshape.h"
#include "prototyp.h"
#include "d3d_hud.h"


extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

static void *CurrTextureHandle;

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
#if 1
	int texoffset;
	void *TextureHandle;
	int i;
	GLfloat ZNear, zvalue;
	
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DHandle;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
//	fprintf(stderr, "D3D_ZBufferedGouraudTexturedPolygon_Output(%p, %p)\n", inputPolyPtr, renderVerticesPtr);
//	fprintf(stderr, "\tRenderPolygon.NumberOfVertices = %d\n", RenderPolygon.NumberOfVertices);
//	fprintf(stderr, "\ttexoffset = %d (ptr = %p)\n", texoffset, texoffset ? (void *)ImageHeaderArray[texoffset].D3DHandle : CurrTextureHandle);

switch(RenderPolygon.TranslucencyMode)
{
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
	default:
		fprintf(stderr, "RenderPolygon.TranslucencyMode: invalid %d\n", RenderPolygon.TranslucencyMode);
}

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
		glBlendFunc(GL_ONE, GL_ONE);
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
}
