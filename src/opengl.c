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
#include "savegame.h"
#include "krender.h"
#include "kshape.h"
#include "prototyp.h"
#include "d3d_hud.h"


extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;

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

	glBegin(GL_POLYGON);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		GLfloat x, y, z;
		int x1, y1;
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
/* this is just random garbage */
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
		x = x1;
		y = y1;
						
		x =  (x - 320.0)/320.0;
		y = -(y - 240.0)/240.0;
		
		zvalue = 65536 - vertices->Z+HeadUpDisplayZOffset;
		zvalue = 1.0 - ZNear/zvalue;
		z = -zvalue;
		
		glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);
		glVertex3f(x, y, z);
//		fprintf(stderr, "Vertex %d: (%f, %f, %f)\n\t[%d, %d, %d]->[%d, %d] (%d, %d, %d, %d)\n", i, x, y, z, vertices->X, vertices->Y, vertices->Z, x1, y1, vertices->R, vertices->G, vertices->B, vertices->A);
//		fprintf(stderr, "znear = %f, zvalue = %f, z = %f\n", ZNear, zvalue, z);
	}
	glEnd();
	
	CurrTextureHandle = TextureHandle;
#endif	
}
