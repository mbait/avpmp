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
	int texoffset;
	void *TextureHandle;
	int i;
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DHandle;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	fprintf(stderr, "D3D_ZBufferedGouraudTexturedPolygon_Output(%p, %p)\n", inputPolyPtr, renderVerticesPtr);
	fprintf(stderr, "\tRenderPolygon.NumberOfVertices = %d\n", RenderPolygon.NumberOfVertices);
	fprintf(stderr, "\ttexoffset = %d (ptr = %p)\n", texoffset, texoffset ? (void *)ImageHeaderArray[texoffset].D3DHandle : CurrTextureHandle);
	
	glBegin(GL_POLYGON);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		GLfloat x, y, z;
		int x1, y1;
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
/* this is just random garbage */
//		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
//		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;
		x = vertices->X;
		y = vertices->Y;
				
		x = x/32768.0;
		y = y/32768.0;
		z = -1+-1.0f/vertices->Z;
		
		glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);
		glVertex3f(x, y, z);
		fprintf(stderr, "Vertex %d: (%f, %f, %f) [%d, %d, %d] (%d, %d, %d, %d)\n", i, x, y, z, vertices->X, vertices->Y, vertices->Z, vertices->R, vertices->G, vertices->B, vertices->A);
	}
	glEnd();
	
	CurrTextureHandle = TextureHandle;
}
