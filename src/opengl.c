#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "frustum.h"
#include "lighting.h"
#include "bh_types.h"
#include "showcmds.h"
#include "d3d_hud.h"
#include "hud_layout.h"
#include "avp_userprofile.h"
#include "aw.h"

int LightIntensityAtPoint(VECTORCH *pointPtr);

extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern int SpecialFXImageNumber;
extern int StaticImageNumber;
extern int PredatorNumbersImageNumber;
extern int BurningImageNumber;
extern int ChromeImageNumber;
extern int WaterShaftImageNumber;
extern int HUDFontsImageNumber;
extern int AAFontImageNumber;

extern int FMVParticleColour;
extern int HUDScaleFactor;
extern int CloakingPhase;

static D3DTexture *CurrTextureHandle;


static enum TRANSLUCENCY_TYPE CurrentTranslucencyMode = TRANSLUCENCY_OFF; /* opengl state variable */
static enum FILTERING_MODE_ID CurrentFilteringMode = FILTERING_BILINEAR_OFF;

static D3DTexture *CurrentlyBoundTexture = NULL; /* opengl state variable (->id) */

static void CheckBoundTextureIsCorrect(D3DTexture *tex)
{
	if (tex == CurrentlyBoundTexture)
		return;

	if (tex == NULL) {
		glBindTexture(GL_TEXTURE_2D, 0);
		
		CurrentlyBoundTexture = NULL;
		
		return;
	} 
	
	glBindTexture(GL_TEXTURE_2D, tex->id);

	if (tex->filter != CurrentFilteringMode) {
		switch(CurrentFilteringMode) {
			case FILTERING_BILINEAR_OFF:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTERING_BILINEAR_ON:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;
			default:
		}
		
		tex->filter = CurrentFilteringMode;
	}
	
	CurrentlyBoundTexture = tex;
}

static void CheckFilteringModeIsCorrect(enum FILTERING_MODE_ID filter)
{
	CurrentFilteringMode = filter;
	
	if (CurrentlyBoundTexture && CurrentlyBoundTexture->filter != CurrentFilteringMode) {
		switch(CurrentFilteringMode) {
			case FILTERING_BILINEAR_OFF:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				break;
			case FILTERING_BILINEAR_ON:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				break;
			default:
		}
		
		CurrentlyBoundTexture->filter = CurrentFilteringMode;
	}
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
		default:
			fprintf(stderr, "RenderPolygon.TranslucencyMode: invalid %d\n", RenderPolygon.TranslucencyMode);
			return;
	}
	
	if (mode != TRANSLUCENCY_OFF && CurrentTranslucencyMode == TRANSLUCENCY_OFF)
		glEnable(GL_BLEND);
		
	CurrentTranslucencyMode = mode;
}

#define TA_MAXVERTICES		500
#define TA_MAXTRIANGLES		500

typedef struct VertexArray
{
	GLfloat v[4];
	
	GLfloat t[3]; /* 3rd float is padding */
	
	GLubyte c[4];
} VertexArray;

typedef struct TriangleArray
{
	int a;
	int b;
	int c;
} TriangleArray;

static VertexArray tarr[TA_MAXVERTICES];
static TriangleArray tris[TA_MAXTRIANGLES];

static void DrawTriangleArray_T2F_C4UB_V4F(int len)
{	
	int i;
	
#define OUTPUT_VERTEX(d) \
{ \
	glColor4ubv	(tarr[(d)].c);	\
	glTexCoord2fv	(tarr[(d)].t);	\
	glVertex4fv	(tarr[(d)].v);	\
}
#define OUTPUT_TRIANGLE(a, b, c) \
{ \
	OUTPUT_VERTEX((a));	\
	OUTPUT_VERTEX((b));	\
	OUTPUT_VERTEX((c));	\
}

	glBegin(GL_TRIANGLES);
	for (i = 0; i < len; i++) {
		OUTPUT_TRIANGLE(tris[i].a, tris[i].b, tris[i].c);
	}
	glEnd();
	
#undef OUTPUT_TRIANGLE	
#undef OUTPUT_VERTEX
}

static void DrawTriangles_T2F_C4UB_V4F(int vertices)
{	
#define OUTPUT_VERTEX(d) \
{ \
	glColor4ubv	(tarr[(d)].c);	\
	glTexCoord2fv	(tarr[(d)].t);	\
	glVertex4fv	(tarr[(d)].v);	\
}
#define OUTPUT_TRIANGLE(a, b, c) \
{ \
	OUTPUT_VERTEX((a));	\
	OUTPUT_VERTEX((b));	\
	OUTPUT_VERTEX((c));	\
}

	glBegin(GL_TRIANGLES);
	switch(vertices) {
		case 3:
			OUTPUT_TRIANGLE(0, 2, 1);
			break;
		case 5:
			OUTPUT_TRIANGLE(0, 1, 4);
			OUTPUT_TRIANGLE(1, 3, 4);
			OUTPUT_TRIANGLE(1, 2, 3);
			break;
		case 8:
			OUTPUT_TRIANGLE(0, 6, 7);
		case 7:
			OUTPUT_TRIANGLE(0, 5, 6);
		case 6:
			OUTPUT_TRIANGLE(0, 4, 5);
			OUTPUT_TRIANGLE(0, 3, 4);
		case 4:
			OUTPUT_TRIANGLE(0, 2, 3);
			OUTPUT_TRIANGLE(0, 1, 2);			
			break;
		default:
			fprintf(stderr, "DrawTriangles_T2F_C4UB_V4F: vertices = %d\n", vertices);
	}
	glEnd();
	
#undef OUTPUT_TRIANGLE	
#undef OUTPUT_VERTEX
}

static void DrawTriangles_T2F_V4F(int vertices)
{	
#define OUTPUT_VERTEX(d) \
{ \
	glTexCoord2fv	(tarr[(d)].t);	\
	glVertex4fv	(tarr[(d)].v);	\
}
#define OUTPUT_TRIANGLE(a, b, c) \
{ \
	OUTPUT_VERTEX((a));	\
	OUTPUT_VERTEX((b));	\
	OUTPUT_VERTEX((c));	\
}

	glBegin(GL_TRIANGLES);
	switch(vertices) {
		case 3:
			OUTPUT_TRIANGLE(0, 2, 1);
			break;
		case 5:
			OUTPUT_TRIANGLE(0, 1, 4);
			OUTPUT_TRIANGLE(1, 3, 4);
			OUTPUT_TRIANGLE(1, 2, 3);
			break;
		case 8:
			OUTPUT_TRIANGLE(0, 6, 7);
		case 7:
			OUTPUT_TRIANGLE(0, 5, 6);
		case 6:
			OUTPUT_TRIANGLE(0, 4, 5);
			OUTPUT_TRIANGLE(0, 3, 4);
		case 4:
			OUTPUT_TRIANGLE(0, 2, 3);
			OUTPUT_TRIANGLE(0, 1, 2);			
			break;
		default:
			fprintf(stderr, "DrawTriangles_T2F_V4F: vertices = %d\n", vertices);
	}
	glEnd();
	
#undef OUTPUT_TRIANGLE	
#undef OUTPUT_VERTEX
}

static void DrawTriangles_C4UB_V4F(int vertices)
{	
#define OUTPUT_VERTEX(d) \
{ \
	glColor4ubv	(tarr[(d)].c);	\
	glVertex4fv	(tarr[(d)].v);	\
}
#define OUTPUT_TRIANGLE(a, b, c) \
{ \
	OUTPUT_VERTEX((a));	\
	OUTPUT_VERTEX((b));	\
	OUTPUT_VERTEX((c));	\
}

	glBegin(GL_TRIANGLES);
	switch(vertices) {
		case 3:
			OUTPUT_TRIANGLE(0, 2, 1);
			break;
		case 5:
			OUTPUT_TRIANGLE(0, 1, 4);
			OUTPUT_TRIANGLE(1, 3, 4);
			OUTPUT_TRIANGLE(1, 2, 3);
			break;
		case 8:
			OUTPUT_TRIANGLE(0, 6, 7);
		case 7:
			OUTPUT_TRIANGLE(0, 5, 6);
		case 6:
			OUTPUT_TRIANGLE(0, 4, 5);
			OUTPUT_TRIANGLE(0, 3, 4);
		case 4:
			OUTPUT_TRIANGLE(0, 2, 3);
			OUTPUT_TRIANGLE(0, 1, 2);			
			break;
		default:
			fprintf(stderr, "DrawTriangles_C4UB_V4F: vertices = %d\n", vertices);
	}
	glEnd();
	
#undef OUTPUT_TRIANGLE	
#undef OUTPUT_VERTEX
}
		
static void SelectPolygonBeginType(int points)
{
	switch(points) {
		case 3:
			glBegin(GL_TRIANGLES);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			glBegin(GL_TRIANGLE_FAN);
			break;
		default:
			fprintf(stderr, "SelectPolygonBeginType: points = %d\n", points);
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
	tex->filter = FILTERING_BILINEAR_ON;

	if (CurrentlyBoundTexture)
		glBindTexture(GL_TEXTURE_2D, CurrentlyBoundTexture->id); /* restore current */
	
	return h;
}

/* ** */

void ThisFramesRenderingHasBegun()
{
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);	        
}

void ThisFramesRenderingHasFinished()
{
/* This is where the queued drawing commands' execution takes place */

	LightBlockDeallocation();
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

	/* this does stop zfighting with bulletmarks on walls... */
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-10.0, -10.0);
}

void D3D_DecalSystem_End()
{
	glDepthMask(GL_TRUE);
	
	glDisable(GL_POLYGON_OFFSET_FILL);
}

/* ** */

void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
	GLfloat x[4], y[4];
	
	if (y1 <= y0)
		return;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	CheckBoundTextureIsCorrect(NULL);
	
	glColor4ub(r, g, b, a);

	x[0] = x0;
	x[0] =  (x[0] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[0] = y0;
	y[0] = -(y[0] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[1] = x1 - 1;
	x[1] =  (x[1] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[1] = y0;
	y[1] = -(y[1] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[2] = x1 - 1;
	x[2] =  (x[2] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[2] = y1 - 1;
	y[2] = -(y[2] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
	
	x[3] = x0;
	x[3] =  (x[3] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
	y[3] = y1 - 1;
	y[3] = -(y[3] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

	SelectPolygonBeginType(3); /* triangles */
	
	glVertex3f(x[0], y[0], -1.0f);
	glVertex3f(x[1], y[1], -1.0f);
	glVertex3f(x[3], y[3], -1.0f);
	
	glVertex3f(x[1], y[1], -1.0f);
	glVertex3f(x[2], y[2], -1.0f);
	glVertex3f(x[3], y[3], -1.0f);
	
	glEnd();
}

/* ** */

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	GLfloat ZNear;
	float RecipW, RecipH;

		
	ZNear = (GLfloat) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	if (texoffset) {
		TextureHandle = (void *)ImageHeaderArray[texoffset].D3DTexture;
		
		CurrTextureHandle = TextureHandle;
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);

	if (TextureHandle->w == 128) {
		RecipW = (1.0f / 128.0f) / 65536.0f;
	} else {
		float width = TextureHandle->w;
		RecipW = (1.0f / width) / 65536.0f;
	}
	if (TextureHandle->h == 128) {
		RecipH = (1.0f / 128.0f) / 65536.0f;
	} else {
		float height = TextureHandle->h;
		RecipH = (1.0f / height) / 65536.0f;
	}
	
	CheckBoundTextureIsCorrect(TextureHandle);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		GLfloat x, y, z;
		int x1, y1;
		GLfloat s, t;
		GLfloat rhw = 1.0/(float)vertices->Z, zvalue;
		
		s = ((float)vertices->U) * RecipW + (1.0f/256.0f);
		t = ((float)vertices->V) * RecipH + (1.0f/256.0f);

//		if (s < 0.0 || t < 0.0 || s >= 1.0 || t >= 1.0)
//			fprintf(stderr, "HEY! s = %f, t = %f (%d, %d)\n", s, t, vertices->U, vertices->V);

		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

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

		x = x1;
		y = y1;
						
		x =  (x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
		y = -(y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);

		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;

		
		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].t[0] = s;
		tarr[i].t[1] = t;
		
		tarr[i].c[0] = GammaValues[vertices->R];
		tarr[i].c[1] = GammaValues[vertices->G];
		tarr[i].c[2] = GammaValues[vertices->B];
		tarr[i].c[3] = vertices->A;
	}

	DrawTriangles_T2F_C4UB_V4F(RenderPolygon.NumberOfVertices);

	CheckBoundTextureIsCorrect(NULL);
		
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		tarr[i].c[0] = GammaValues[vertices->SpecularR];
		tarr[i].c[1] = GammaValues[vertices->SpecularG];
		tarr[i].c[2] = GammaValues[vertices->SpecularB];
		tarr[i].c[3] = 255;
	}
	DrawTriangles_C4UB_V4F(RenderPolygon.NumberOfVertices);
}

void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	float RecipW, RecipH;

	texoffset = inputPolyPtr->PolyColour & ClrTxDefn;
	TextureHandle = (void *)ImageHeaderArray[texoffset].D3DTexture;		
	CurrTextureHandle = TextureHandle;
	
	
	if (TextureHandle->w == 128) {
		RecipW = (1.0f / 128.0f) / 65536.0f;
	} else {
		float width = TextureHandle->w;
		RecipW = (1.0f / width) / 65536.0f;
	}
	if (TextureHandle->h == 128) {
		RecipH = (1.0f / 128.0f) / 65536.0f;
	} else {
		float height = TextureHandle->h;
		RecipH = (1.0f / height) / 65536.0f;
	}
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);
	CheckBoundTextureIsCorrect(TextureHandle);
	
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

		x = x1;
		y = y1;
						
		x =  (x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
		y = -(y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);

		z = 1.0f;

		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].t[0] = s;
		tarr[i].t[1] = t;
		
		tarr[i].c[0] = vertices->R;
		tarr[i].c[1] = vertices->G;
		tarr[i].c[2] = vertices->B;
		tarr[i].c[3] = vertices->A;		
	}

	DrawTriangles_T2F_C4UB_V4F(RenderPolygon.NumberOfVertices);	
}

void D3D_ZBufferedCloakedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int flags;
	int texoffset;
	int i;
	D3DTexture *TextureHandle;
	
	float ZNear;
	float RecipW, RecipH;
	
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	flags = inputPolyPtr->PolyFlags;
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);
	
	TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
	CurrTextureHandle = TextureHandle;
	
	CheckBoundTextureIsCorrect(TextureHandle);
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
	
	if (TextureHandle->w == 128) {
		RecipW = 1.0f / 128.0f;
	} else {
		float width = (float) TextureHandle->w;
		RecipW = 1.0f / width;
	}
	
	if (TextureHandle->h == 128) {
		RecipH = 1.0f / 128.0f;
	} else {
		float height = (float) TextureHandle->h;
		RecipH = 1.0f / height;
	}
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z;
		int x1, y1;
		GLfloat s, t;
		GLfloat rhw = 1.0/(float)vertices->Z;
		GLfloat zvalue;
		
		s = ((float)(vertices->U>>16)+0.5) * RecipW;
		t = ((float)(vertices->V>>16)+0.5) * RecipH;
		
//		if (s < 0.0 || t < 0.0 || s >= 1.0 || t >= 1.0)
//			fprintf(stderr, "HEY! s = %f, t = %f (%d, %d)\n", s, t, vertices->U, vertices->V);
			
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

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

		x = x1;
		y = y1;
						
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;

		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;

		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].t[0] = s;
		tarr[i].t[1] = t;
		
		tarr[i].c[0] = vertices->R;
		tarr[i].c[1] = vertices->G;
		tarr[i].c[2] = vertices->B;
		tarr[i].c[3] = vertices->A;
	}

	DrawTriangles_T2F_C4UB_V4F(RenderPolygon.NumberOfVertices);
}

void D3D_Decal_Output(DECAL *decalPtr, RENDERVERTEX *renderVerticesPtr)
{
	DECAL_DESC *decalDescPtr = &DecalDescription[decalPtr->DecalID];
	int texoffset;
	D3DTexture *TextureHandle;
	int i;
	
	float ZNear;
	float RecipW, RecipH;
	int r, g, b, a;
	
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	CheckTranslucencyModeIsCorrect(decalDescPtr->TranslucencyType);
	
	if (decalPtr->DecalID == DECAL_FMV) {
		/* not (yet) implemented */
		return;
	} else if (decalPtr->DecalID == DECAL_SHAFTOFLIGHT||decalPtr->DecalID == DECAL_SHAFTOFLIGHT_OUTER) {
		CheckBoundTextureIsCorrect(NULL);
		
		RecipW = 1.0 / 256.0; /* ignored */
		RecipH = 1.0 / 256.0;
	} else {
		texoffset = SpecialFXImageNumber;
		
		TextureHandle = ImageHeaderArray[texoffset].D3DTexture;
		
		if (TextureHandle->w == 256) {
			RecipW = 1.0 / 256.0;
		} else {
			float width = (float) TextureHandle->w; 
			RecipW = 1.0 / width;
		}
		
		if (TextureHandle->h == 256) {
			RecipH = 1.0 / 256.0;
		} else {
			float height = (float) TextureHandle->h;
			RecipH = 1.0 / height;
		}
		
		CheckBoundTextureIsCorrect(TextureHandle);
	}
	
	if (decalDescPtr->IsLit) {
		int intensity = LightIntensityAtPoint(decalPtr->Vertices);
		
		r = MUL_FIXED(intensity,decalDescPtr->RedScale[CurrentVisionMode]);
		g = MUL_FIXED(intensity,decalDescPtr->GreenScale[CurrentVisionMode]);
		b = MUL_FIXED(intensity,decalDescPtr->BlueScale[CurrentVisionMode]);
		a = decalDescPtr->Alpha;
	} else {
		r = decalDescPtr->RedScale[CurrentVisionMode];
		g = decalDescPtr->GreenScale[CurrentVisionMode];
		b = decalDescPtr->BlueScale[CurrentVisionMode];
		a = decalDescPtr->Alpha;
	}
	
	if (RAINBOWBLOOD_CHEATMODE) {
		r = FastRandom()&255;
		g = FastRandom()&255;
		b = FastRandom()&255;
		a = decalDescPtr->Alpha;
	}
	
	glColor4ub(r, g, b, a);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z, zvalue;
		GLfloat s, t, rhw;
		int x1, y1;
		
		rhw = 1.0 / vertices->Z;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

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
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		s = ((float)(vertices->U>>16)+.5) * RecipW;
		t = ((float)(vertices->V>>16)+.5) * RecipH;

		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
		
		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].t[0] = s;
		tarr[i].t[1] = t;
	}
	
	DrawTriangles_T2F_V4F(RenderPolygon.NumberOfVertices);
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
	
	CheckBoundTextureIsCorrect(TextureHandle);
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
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		if (particleDescPtr->IsDrawnInFront) {
			z = -0.999f; /* ... */
		} else if (particleDescPtr->IsDrawnAtBack) {
			z = 0.999f;
		} else {
			z = 1.0 - 2.0*ZNear/((float)vertices->Z); /* currently maps [ZNear, inf) to [-1, 1], probably could be more precise with a ZFar */
		}

		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].t[0] = s;
		tarr[i].t[1] = t;
	}

	DrawTriangles_T2F_V4F(RenderPolygon.NumberOfVertices);
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	float ZNear;
	int i;
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	CheckBoundTextureIsCorrect(NULL); /* disable texturing */
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_OFF);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		int x1, y1;
		GLfloat x, y, z;
		float rhw, zvalue;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

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
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
		
		rhw = 1.0/(float)vertices->Z;
		
		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].c[0] = vertices->R;
		tarr[i].c[1] = vertices->G;
		tarr[i].c[2] = vertices->B;
		tarr[i].c[3] = vertices->A;
	}

	DrawTriangles_C4UB_V4F(RenderPolygon.NumberOfVertices);
}

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int flags, i;
	float ZNear;
	
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	flags = inputPolyPtr->PolyFlags;
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);
	CheckBoundTextureIsCorrect(NULL);
	
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];	
		int x1, y1;
		GLfloat x, y, z;
		float rhw, zvalue;
		
		x1 = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;
		y1 = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

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
		
		x = x1;
		y = y1;
		
		x =  (x - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y = -(y - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
				
		rhw = 1.0/(float)vertices->Z;
		
		tarr[i].v[0] = x/rhw;
		tarr[i].v[1] = y/rhw;
		tarr[i].v[2] = z/rhw;
		tarr[i].v[3] = 1/rhw;
		
		tarr[i].c[0] = vertices->R;
		tarr[i].c[1] = vertices->G;
		tarr[i].c[2] = vertices->B;
		if (flags & iflag_transparent)
			tarr[i].c[3] = vertices->A;
		else
			tarr[i].c[3] = 255;
	}

	DrawTriangles_C4UB_V4F(RenderPolygon.NumberOfVertices);
}

void D3D_PlayerOnFireOverlay()
{
	int c = 128;
	int colour = (FMVParticleColour&0xffffff)+(c<<24);
	GLfloat x[4], y[4], s[4], t[4];
	float u, v;
	int r, g, b, a;
	D3DTexture *TextureHandle;

	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
	
	TextureHandle = ImageHeaderArray[BurningImageNumber].D3DTexture;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	CheckBoundTextureIsCorrect(TextureHandle);
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	
	glColor4ub(r, g, b, a);
	
	u = (FastRandom()&255)/256.0f;
	v = (FastRandom()&255)/256.0f;
	
	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = u;
	t[0] = v;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = u + 1.0f;
	t[1] = v;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = u + 1.0f;
	t[2] = v + 1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = u;
	t[3] = v + 1.0f;
	
	SelectPolygonBeginType(3); /* triangles */
	
	glTexCoord2f(s[0], t[0]);
	glVertex3f(x[0], y[0], -1.0f);
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], -1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], -1.0f);
	
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], -1.0f);
	glTexCoord2f(s[2], t[2]);
	glVertex3f(x[2], y[2], -1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], -1.0f);
	
	glEnd();
}

void D3D_PlayerDamagedOverlay(int intensity)
{
	D3DTexture *TextureHandle;
	int theta[2];
	int colour, baseColour;
	int r, g, b, a;
	int i;
	
	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;
	
	TextureHandle = ImageHeaderArray[SpecialFXImageNumber].D3DTexture;
	switch(AvP.PlayerType) {
		default:
			// LOCALASSERT(0);
		case I_Marine:
			baseColour = 0xff0000;
			break;
		case I_Alien:
			baseColour = 0xffff00;
			break;
		case I_Predator:
			baseColour = 0x00ff00;
			break;
	}
	
	CheckBoundTextureIsCorrect(TextureHandle);
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	
	colour = 0xffffff - baseColour + (intensity<<24);
	
	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
	
	glColor4ub(r, g, b, a);
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_INVCOLOUR);
	for (i = 0; i < 2; i++) {
		GLfloat x[4], y[4], s[4], t[4];
		
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;	

		x[0] = -1.0f;
		y[0] = -1.0f;
		s[0] = 0.875f + (cos*(-1) - sin*(-1));
		t[0] = 0.375f + (sin*(-1) + cos*(-1));
		x[1] =  1.0f;
		y[1] = -1.0f;
		s[1] = 0.875f + (cos*(+1) - sin*(-1));
		t[1] = 0.375f + (sin*(+1) + cos*(-1));
		x[2] =  1.0f;
		y[2] =  1.0f;
		s[2] = 0.875f + (cos*(+1) - sin*(+1));
		t[2] = 0.375f + (sin*(+1) + cos*(+1));
		x[3] = -1.0f;
		y[3] =  1.0f;
		s[3] = 0.875f + (cos*(-1) - sin*(+1));
		t[3] = 0.375f + (sin*(-1) + cos*(+1));
	
		SelectPolygonBeginType(3); /* triangles */
	
		glTexCoord2f(s[0], t[0]);
		glVertex3f(x[0], y[0], -1.0f);
		glTexCoord2f(s[1], t[1]);
		glVertex3f(x[1], y[1], -1.0f);
		glTexCoord2f(s[3], t[3]);
		glVertex3f(x[3], y[3], -1.0f);
	
		glTexCoord2f(s[1], t[1]);
		glVertex3f(x[1], y[1], -1.0f);
		glTexCoord2f(s[2], t[2]);
		glVertex3f(x[2], y[2], -1.0f);
		glTexCoord2f(s[3], t[3]);
		glVertex3f(x[3], y[3], -1.0f);
	
		glEnd();
	
		colour = baseColour + (intensity<<24);
		
		b = (colour >> 0)  & 0xFF;
		g = (colour >> 8)  & 0xFF;
		r = (colour >> 16) & 0xFF;
		a = (colour >> 24) & 0xFF;
	
		glColor4ub(r, g, b, a);
	
		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	}
}

void DrawNoiseOverlay(int tr)
{
	GLfloat x[4], y[4], s[4], t[4], u, v;
	int r, g, b;
	D3DTexture *tex;
	int size;
	
	r = 255;
	g = 255;
	b = 255;
	
	size = 256;
	
	tex = ImageHeaderArray[StaticImageNumber].D3DTexture;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	CheckBoundTextureIsCorrect(tex);
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	glDepthFunc(GL_ALWAYS);
	
	u = FastRandom()&255;
	v = FastRandom()&255;
	
	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = u / 256.0f;
	t[0] = v / 256.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = (u + size) / 256.0f;
	t[1] = v / 256.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = (u + size) / 256.0f;
	t[2] = (v + size) / 256.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = u / 256.0f;
	t[3] = (v + size) / 256.0f;
	
	SelectPolygonBeginType(3); /* triangles */
	glColor4ub(r, g, b, tr);
		
	glTexCoord2f(s[0], t[0]);
	glVertex3f(x[0], y[0], 1.0f);
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], 1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], 1.0f);
	
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], 1.0f);
	glTexCoord2f(s[2], t[2]);
	glVertex3f(x[2], y[2], 1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], 1.0f);
	
	glEnd();
	
	glDepthFunc(GL_LEQUAL);
}

void D3D_ScreenInversionOverlay()
{
	D3DTexture *tex;
	int theta[2];
	int i;
	
	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;
		
	tex = ImageHeaderArray[SpecialFXImageNumber].D3DTexture;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_DARKENINGCOLOUR);
	CheckBoundTextureIsCorrect(tex);
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	for (i = 0; i < 2; i++) {
		GLfloat x[4], y[4], s[4], t[4];
		
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		
		x[0] = -1.0f;
		y[0] = -1.0f;
		s[0] = 0.375f + (cos*(-1) - sin*(-1));
		t[0] = 0.375f + (sin*(-1) + cos*(-1));
		x[1] =  1.0f;
		y[1] = -1.0f;
		s[1] = 0.375f + (cos*(+1) - sin*(-1));
		t[1] = 0.375f + (sin*(+1) + cos*(-1));
		x[2] =  1.0f;
		y[2] =  1.0f;
		s[2] = 0.375f + (cos*(+1) - sin*(+1));
		t[2] = 0.375f + (sin*(+1) + cos*(+1));
		x[3] = -1.0f;
		y[3] =  1.0f;
		s[3] = 0.375f + (cos*(-1) - sin*(+1));
		t[3] = 0.375f + (sin*(-1) + cos*(+1));

		SelectPolygonBeginType(3); /* triangles */
		
		glTexCoord2f(s[0], t[0]);
		glVertex3f(x[0], y[0], 1.0f);
		glTexCoord2f(s[1], t[1]);
		glVertex3f(x[1], y[1], 1.0f);
		glTexCoord2f(s[3], t[3]);
		glVertex3f(x[3], y[3], 1.0f);
	
		glTexCoord2f(s[1], t[1]);
		glVertex3f(x[1], y[1], 1.0f);
		glTexCoord2f(s[2], t[2]);
		glVertex3f(x[2], y[2], 1.0f);
		glTexCoord2f(s[3], t[3]);
		glVertex3f(x[3], y[3], 1.0f);
	
		glEnd();
		
		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_COLOUR);
	}
}

void D3D_PredatorScreenInversionOverlay()
{
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_DARKENINGCOLOUR);
	CheckBoundTextureIsCorrect(NULL);
	glDepthFunc(GL_ALWAYS);
	
	SelectPolygonBeginType(3); /* triangles */
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f,  1.0f, 1.0f);
	
	glVertex3f( 1.0f, -1.0f, 1.0f);
	glVertex3f( 1.0f,  1.0f, 1.0f);
	glVertex3f(-1.0f,  1.0f, 1.0f);
	
	glEnd();
	
	glDepthFunc(GL_LEQUAL);
}

void DrawScanlinesOverlay(float level)
{
	D3DTexture *tex;
	GLfloat x[4], y[4], s[4], t[4];
	float v, size;
	int c;
	int a;

	tex = ImageHeaderArray[PredatorNumbersImageNumber].D3DTexture;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
	CheckBoundTextureIsCorrect(tex);
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	glDepthFunc(GL_ALWAYS);
	
	c = 255;
	a = 64.0f+level*64.0f;
	
	v = 128.0f;
	size = 128.0f*(1.0f-level*0.8f);
	
	glColor4ub(c, c, c, a);

	x[0] = -1.0f;
	y[0] = -1.0f;
	s[0] = (v - size) / 256.0f;
	t[0] = 1.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	s[1] = (v - size) / 256.0f;
	t[1] = 1.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	s[2] = (v + size) / 256.0f;
	t[2] = 1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	s[3] = (v + size) / 256.0f;
	t[3] = 1.0f;
	
	SelectPolygonBeginType(3); /* triangles */
		
	glTexCoord2f(s[0], t[0]);
	glVertex3f(x[0], y[0], 1.0f);
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], 1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], 1.0f);
	
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], 1.0f);
	glTexCoord2f(s[2], t[2]);
	glVertex3f(x[2], y[2], 1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], 1.0f);
	
	glEnd();	
	glDepthFunc(GL_LEQUAL);
}

void D3D_FadeDownScreen(int brightness, int colour)
{
	int t, r, g, b, a;
	GLfloat x[4], y[4];
	
	t = 255 - (brightness>>8);
	if (t<0) t = 0;
	colour = (t<<24)+colour;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
	CheckBoundTextureIsCorrect(NULL);
	
	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
	
	glColor4ub(r, g, b, a);
	
	x[0] = -1.0f;
	y[0] = -1.0f;
	x[1] =  1.0f;
	y[1] = -1.0f;
	x[2] =  1.0f;
	y[2] =  1.0f;
	x[3] = -1.0f;
	y[3] =  1.0f;
	
	SelectPolygonBeginType(3); /* triangles */
	
	glVertex3f(x[0], y[0], -1.0f);
	glVertex3f(x[1], y[1], -1.0f);
	glVertex3f(x[3], y[3], -1.0f);
	
	glVertex3f(x[1], y[1], -1.0f);
	glVertex3f(x[2], y[2], -1.0f);
	glVertex3f(x[3], y[3], -1.0f);
	
	glEnd();
}

void D3D_HUD_Setup()
{
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	
	glDepthFunc(GL_LEQUAL);
}

void D3D_HUDQuad_Output(int imageNumber, struct VertexTag *quadVerticesPtr, unsigned int colour)
{
	float RecipW, RecipH;
	int i;
	D3DTexture *tex = ImageHeaderArray[imageNumber].D3DTexture;
	GLfloat x[4], y[4], s[4], t[4];
	int r, g, b, a;

/* possibly use polygon offset? (predator hud) */

	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	CheckBoundTextureIsCorrect(tex);
	
	if (tex->w == 128) {
		RecipW = 1.0f / 128.0f;
	} else {
		float width = (float) tex->w;
		RecipW = 1.0f / width;
	}
	
	if (tex->h == 128) {
		RecipH = 1.0f / 128.0f;
	} else {
		float height = (float) tex->h;
		RecipH = 1.0f / height;
	}
	
	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
		
	glColor4ub(r, g, b, a);
	
	for (i = 0; i < 4; i++) {
		x[i] = quadVerticesPtr[i].X;
		x[i] =  (x[i] - ScreenDescriptorBlock.SDB_CentreX)/ScreenDescriptorBlock.SDB_CentreX;
		y[i] = quadVerticesPtr[i].Y;
		y[i] = -(y[i] - ScreenDescriptorBlock.SDB_CentreY)/ScreenDescriptorBlock.SDB_CentreY;
		
		s[i] = ((float)quadVerticesPtr[i].U)*RecipW;
		t[i] = ((float)quadVerticesPtr[i].V)*RecipH;
	}
	
	SelectPolygonBeginType(3); /* triangles */
	
	glTexCoord2f(s[0], t[0]);
	glVertex3f(x[0], y[0], -1.0f);
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], -1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], -1.0f);
	
	glTexCoord2f(s[1], t[1]);
	glVertex3f(x[1], y[1], -1.0f);
	glTexCoord2f(s[2], t[2]);
	glVertex3f(x[2], y[2], -1.0f);
	glTexCoord2f(s[3], t[3]);
	glVertex3f(x[3], y[3], -1.0f);
	
	glEnd();
}

void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour)
{
	struct VertexTag quadVertices[4];
	int noOfDigits=3;
	int h = MUL_FIXED(HUDScaleFactor,HUD_DIGITAL_NUMBERS_HEIGHT);
	int w = MUL_FIXED(HUDScaleFactor,HUD_DIGITAL_NUMBERS_WIDTH);

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + h;
	quadVertices[3].Y = y + h;
	
	x += (3*w)/2;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	
	do {
		int topLeftU, topLeftV;
		
		int digit = number%10;
		number/=10;
		
		if (digit<8) {
			topLeftU = 1+(digit)*16;
			topLeftV = 1;
		} else {
			topLeftU = 1+(digit-8)*16;
			topLeftV = 1+24;
		}
		if (AvP.PlayerType == I_Marine) topLeftV+=80;
		
		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
		quadVertices[2].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;
		
		x -= 1+w;
		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + w;
		quadVertices[2].X = x + w;
		
		D3D_HUDQuad_Output(HUDFontsImageNumber, quadVertices, colour);
		
	} while (--noOfDigits);
}

void D3D_RenderHUDString(char *stringPtr,int x,int y,int colour)
{
	struct VertexTag quadVertices[4];

if (stringPtr == NULL) return;

	quadVertices[0].Y = y-1;
	quadVertices[1].Y = y-1;
	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	while( *stringPtr )
	{
		char c = *stringPtr++;

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;
			
			quadVertices[0].X = x - 1;
			quadVertices[3].X = x - 1;
			quadVertices[1].X = x + HUD_FONT_WIDTH + 1;
			quadVertices[2].X = x + HUD_FONT_WIDTH + 1;
				
			D3D_HUDQuad_Output
			(
				AAFontImageNumber,
				quadVertices,
				colour
			);
		}
		x += AAFontWidths[(int)c];
	}
}

void D3D_RenderHUDString_Clipped(char *stringPtr,int x,int y,int colour)
{
	struct VertexTag quadVertices[4];

// 	LOCALASSERT(y<=0);
if (stringPtr == NULL) return;

	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;
	
	quadVertices[0].Y = 0;
	quadVertices[1].Y = 0;

	while ( *stringPtr )
	{
		char c = *stringPtr++;

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - y;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH+1;
			quadVertices[1].V = topLeftV - y;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH+1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT+1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT+1;
			
			quadVertices[0].X = x - 1;
			quadVertices[3].X = x - 1;
			quadVertices[1].X = x + HUD_FONT_WIDTH + 1;
			quadVertices[2].X = x + HUD_FONT_WIDTH + 1;
				
			D3D_HUDQuad_Output
			(
				AAFontImageNumber,
				quadVertices,
				colour
			);
		}
		x += AAFontWidths[(int)c];
	}
}

void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour)
{
	int x, length = 0;
	char *ptr = stringPtr;
	struct VertexTag quadVertices[4];

if (stringPtr == NULL) return;
	
	while(*ptr)
	{
		length+=AAFontWidths[(int)*ptr++];
	}
	length = MUL_FIXED(HUDScaleFactor,length);

	x = centreX-length/2;

	quadVertices[0].Y = y-MUL_FIXED(HUDScaleFactor,1);
	quadVertices[1].Y = y-MUL_FIXED(HUDScaleFactor,1);
	quadVertices[2].Y = y + MUL_FIXED(HUDScaleFactor,HUD_FONT_HEIGHT + 1);
	quadVertices[3].Y = y + MUL_FIXED(HUDScaleFactor,HUD_FONT_HEIGHT + 1);
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);

	while( *stringPtr )
	{
		char c = *stringPtr++;

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].X = x - MUL_FIXED(HUDScaleFactor,1);
			quadVertices[3].X = x - MUL_FIXED(HUDScaleFactor,1);
			quadVertices[1].X = x + MUL_FIXED(HUDScaleFactor,HUD_FONT_WIDTH + 1);
			quadVertices[2].X = x + MUL_FIXED(HUDScaleFactor,HUD_FONT_WIDTH + 1);
				
			D3D_HUDQuad_Output
			(
				AAFontImageNumber,
				quadVertices,
				colour
			);
		}
		x += MUL_FIXED(HUDScaleFactor,AAFontWidths[(int)c]);
	}
}

void RenderString(char *stringPtr, int x, int y, int colour)
{
if (stringPtr == NULL) return;

	D3D_RenderHUDString(stringPtr,x,y,colour);
}

void RenderStringCentred(char *stringPtr, int centreX, int y, int colour)
{
	int length = 0;
	char *ptr = stringPtr;
	
if (stringPtr == NULL) return;

	while(*ptr)
	{
		length+=AAFontWidths[(int)*ptr++];
	}
	D3D_RenderHUDString(stringPtr,centreX-length/2,y,colour);
}

void RenderStringVertically(char *stringPtr, int centreX, int bottomY, int colour)
{
	struct VertexTag quadVertices[4];
	int y = bottomY;

if (stringPtr == NULL) return;
 
	quadVertices[0].X = centreX - (HUD_FONT_HEIGHT/2) - 1;
	quadVertices[1].X = quadVertices[0].X;
	quadVertices[2].X = quadVertices[0].X+2+HUD_FONT_HEIGHT*1;
	quadVertices[3].X = quadVertices[2].X;
	
	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	while( *stringPtr )
	{
		char c = *stringPtr++;

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].Y = y ;
			quadVertices[1].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[2].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[3].Y = y ;
				
			D3D_HUDQuad_Output
			(								  
				AAFontImageNumber,
				quadVertices,
				colour
			);
		}
	   	y -= AAFontWidths[(int)c];
	}
}

void ColourFillBackBuffer(int FillColour)
{
	float r, g, b, a;
	
	b = ((FillColour >> 0)  & 0xFF) / 255.0f;
	g = ((FillColour >> 8)  & 0xFF) / 255.0f;
	r = ((FillColour >> 16) & 0xFF) / 255.0f;
	a = ((FillColour >> 24) & 0xFF) / 255.0f;
	
	glClearColor(r, g, b, a);
	
	glClear(GL_COLOR_BUFFER_BIT);
}

void D3D_DrawBackdrop()
{
	extern int NumActiveBlocks;
	extern DISPLAYBLOCK *ActiveBlockList[];
	extern MODULE *playerPherModule;
	
	PLAYER_STATUS *playerStatusPtr;
	int numOfObjects = NumActiveBlocks;
	int needToDrawBackdrop = 0;
	
	if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE)
		return;
	
	if (ShowDebuggingText.Tears) {
		ColourFillBackBuffer((63<<5));
		return;
	}
	
	while(numOfObjects--) {
		DISPLAYBLOCK *objectPtr = ActiveBlockList[numOfObjects];
		MODULE *modulePtr = objectPtr->ObMyModule;
		
		if (modulePtr && (ModuleCurrVisArray[modulePtr->m_index] == 2) && modulePtr->m_flags&MODULEFLAG_SKY) {
			needToDrawBackdrop = 1;
			break;
		}	
	}

	if (needToDrawBackdrop) {
		extern BOOL LevelHasStars;
		extern void RenderSky(void);
		extern void RenderStarfield(void);
		
		ColourFillBackBuffer(0);
		
		if (LevelHasStars) {
			RenderStarfield();
		} else {
			RenderSky();
		}
		
		return;
	}
	
	if (!playerPherModule) {
		ColourFillBackBuffer(0);
		return;
	}
	
	playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	
	if (!playerStatusPtr->IsAlive || FREEFALL_CHEATMODE) {
		ColourFillBackBuffer(0);
		return;
	}
}

/* ** */

/* Hacked in special effects */

extern int sine[];
extern int cosine[];

void UpdateForceField(void);
void D3D_DrawForceField(int xOrigin, int yOrigin, int zOrigin, int fieldType);

void UpdateWaterFall(void);
void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin);
void D3D_DrawPowerFence(int xOrigin, int yOrigin, int zOrigin, int xScale, int yScale, int zScale);
void D3D_DrawExplosion(int xOrigin, int yOrigin, int zOrigin, int size);

void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin);

void D3D_DrawWaterOctagonPatch(int xOrigin, int yOrigin, int zOrigin, int xOffset, int zOffset);

int LightSourceWaterPoint(VECTORCH *pointPtr,int offset);
void D3D_DrawWaterMesh_Unclipped(void);
void D3D_DrawWaterMesh_Clipped(void);


void D3D_DrawMoltenMetal(int xOrigin, int yOrigin, int zOrigin);
void D3D_DrawMoltenMetalMesh_Unclipped(void);
void D3D_DrawMoltenMetalMesh_Clipped(void);

int MeshXScale;
int MeshZScale;
int WaterFallBase;

int WaterXOrigin;
int WaterZOrigin;
float WaterUScale;
float WaterVScale;

void D3D_DrawParticle_Rain(PARTICLE *particlePtr,VECTORCH *prevPositionPtr)
{
	VECTORCH vertices[3];
	float ZNear;
	
	vertices[0] = *prevPositionPtr;
	
	/* translate second vertex into view space */
	TranslatePointIntoViewspace(&vertices[0]);

	/* is particle within normal view frustrum ? */
	if((-vertices[0].vx <= vertices[0].vz)
	&&(vertices[0].vx <= vertices[0].vz)
	&&(-vertices[0].vy <= vertices[0].vz)
	&&(vertices[0].vy <= vertices[0].vz))
	{													

		vertices[1] = particlePtr->Position;
		vertices[2] = particlePtr->Position;
		vertices[1].vx += particlePtr->Offset.vx;
		vertices[2].vx -= particlePtr->Offset.vx;
		vertices[1].vz += particlePtr->Offset.vz;
		vertices[2].vz -= particlePtr->Offset.vz;

		/* translate particle into view space */
		TranslatePointIntoViewspace(&vertices[1]);
		TranslatePointIntoViewspace(&vertices[2]);

		ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		{
			int i = 3;
			VECTORCH *verticesPtr = vertices;
			do
			{
				GLfloat xf, yf, zf, rhw;
				
				int x = (verticesPtr->vx*(Global_VDB_Ptr->VDB_ProjX))/verticesPtr->vz+Global_VDB_Ptr->VDB_CentreX;
				int y = (verticesPtr->vy*(Global_VDB_Ptr->VDB_ProjY))/verticesPtr->vz+Global_VDB_Ptr->VDB_CentreY;

				if (x<Global_VDB_Ptr->VDB_ClipLeft)
				{
					x=Global_VDB_Ptr->VDB_ClipLeft;
				}	
				else if (x>Global_VDB_Ptr->VDB_ClipRight)
				{
					x=Global_VDB_Ptr->VDB_ClipRight;	
				}
					
				if (y<Global_VDB_Ptr->VDB_ClipUp)
				{
					y=Global_VDB_Ptr->VDB_ClipUp;
				}
				else if (y>Global_VDB_Ptr->VDB_ClipDown)
				{
					y=Global_VDB_Ptr->VDB_ClipDown;	
				}
		
				xf =  ((float)x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f)/((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
				yf = -((float)y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f)/((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);
				
				zf = 1.0f - 2.0f*ZNear/(float)verticesPtr->vz;
				rhw = 1.0f / (float)verticesPtr->vz;
				
				tarr[3-i].v[0] = xf/rhw;
				tarr[3-i].v[1] = yf/rhw;
				tarr[3-i].v[2] = zf/rhw;
				tarr[3-i].v[3] = 1.0f/rhw;
				
				if (i == 3) {
					tarr[3-i].c[0] = 0;
					tarr[3-i].c[1] = 255;
					tarr[3-i].c[2] = 255;
					tarr[3-i].c[3] = 32;
				} else {
					tarr[3-i].c[0] = 255;
					tarr[3-i].c[1] = 255;
					tarr[3-i].c[2] = 255;
					tarr[3-i].c[3] = 32;
				}

				verticesPtr++;
			}
		  	while(--i);
		}
		
		CheckBoundTextureIsCorrect(NULL);
		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);

		DrawTriangles_C4UB_V4F(3); /* single triangle */
	}
}

void PostLandscapeRendering()
{
	extern int NumOnScreenBlocks;
	extern DISPLAYBLOCK *OnScreenBlockList[];
	int numOfObjects = NumOnScreenBlocks;

	extern char LevelName[];

#if 0 /* not yet */
	if (!strcmp(LevelName,"fall")||!strcmp(LevelName,"fall_m"))
	{
		char drawWaterFall = 0;
		char drawStream = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"fall01"))
				  ||(!strcmp(modulePtr->name,"well01"))
				  ||(!strcmp(modulePtr->name,"well02"))
				  ||(!strcmp(modulePtr->name,"well03"))
				  ||(!strcmp(modulePtr->name,"well04"))
				  ||(!strcmp(modulePtr->name,"well05"))
				  ||(!strcmp(modulePtr->name,"well06"))
				  ||(!strcmp(modulePtr->name,"well07"))
				  ||(!strcmp(modulePtr->name,"well08"))
				  ||(!strcmp(modulePtr->name,"well")))
				{
					drawWaterFall = 1;
				}
				else if( (!strcmp(modulePtr->name,"stream02"))
				       ||(!strcmp(modulePtr->name,"stream03"))
				       ||(!strcmp(modulePtr->name,"watergate")))
				{
		   			drawStream = 1;
				}
			}
		}	

		if (drawWaterFall)
		{
			// Turn OFF texturing if it is on...
			if (CurrTextureHandle != NULL)
			{
				OP_STATE_RENDER(1, ExecBufInstPtr);
				STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, NULL, ExecBufInstPtr);
				CurrTextureHandle = NULL;
			}
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
			if (NumVertices)
			{
			   WriteEndCodeToExecuteBuffer();
		  	   UnlockExecuteBufferAndPrepareForUse();
			   ExecuteBuffer();
		  	   LockExecuteBuffer();
			}
			OP_STATE_RENDER(1, ExecBufInstPtr);
		    //STATE_DATA(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS, ExecBufInstPtr);
			STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, FALSE, ExecBufInstPtr);

	   		//UpdateWaterFall();
			WaterFallBase = 109952;
			
			MeshZScale = (66572-51026)/15;
			MeshXScale = (109952+3039)/45;

	   		D3D_DrawWaterFall(175545,-3039,51026);
//			MeshZScale = -(538490-392169);
//			MeshXScale = 55000;
	//		D3D_DrawWaterPatch(-100000, WaterFallBase, 538490);
										
			OP_STATE_RENDER(1, ExecBufInstPtr);
		    //STATE_DATA(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL, ExecBufInstPtr);
		    STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, TRUE, ExecBufInstPtr);
		}
		if (drawStream)
		{
			int x = 68581;
			int y = 12925;
			int z = 93696;
			MeshXScale = (87869-68581);
			MeshZScale = (105385-93696);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			// Turn OFF texturing if it is on...
			D3DTEXTUREHANDLE TextureHandle = (D3DTEXTUREHANDLE)ImageHeaderArray[ChromeImageNumber].D3DHandle;
			if (CurrTextureHandle != TextureHandle)
			{
				OP_STATE_RENDER(1, ExecBufInstPtr);
				STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
				CurrTextureHandle = TextureHandle;
			}	 
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
			if (NumVertices)
			{
			   WriteEndCodeToExecuteBuffer();
		  	   UnlockExecuteBufferAndPrepareForUse();
			   ExecuteBuffer();
		  	   LockExecuteBuffer();
			}
		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
	}
#endif /* not yet */	
	#if 0
	else if ( (!__stricmp(LevelName,"e3demo")) || (!__stricmp(LevelName,"e3demosp")) )
	{
		int drawOctagonPool = -1;
		int drawFMV = -1;
		int drawPredatorFMV = -1;
		int drawSwirlyFMV = -1;
		int drawSwirlyFMV2 = -1;
		int drawSwirlyFMV3 = -1;
		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if(!__stricmp(modulePtr->name,"water1"))
				{
					drawOctagonPool = modulePtr->m_index;
				}
				else if(!__stricmp(modulePtr->name,"marine01b"))
				{
					drawFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"predator01"))
				{
					drawPredatorFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"toptopgr01"))
				{
					drawSwirlyFMV = modulePtr->m_index;
				}
				else if(!_stricmp(modulePtr->name,"grille04"))
				{
					drawSwirlyFMV2 = modulePtr->m_index;
				}
				#if 0
				else if(!_stricmp(modulePtr->name,"marine05"))
				{
					drawSwirlyFMV3 = modulePtr->m_index;
				}
				#endif
			}
		}	
		#if FMV_ON
//		UpdateFMVTextures(3);
		

		if (drawFMV!=-1)
		{
			DECAL fmvDecal =
			{
				DECAL_FMV,
			};
			fmvDecal.ModuleIndex = drawFMV;
			fmvDecal.UOffset = 0;

			UpdateFMVTextures(4);
			
			for (int z=0; z<6; z++)
			{
				for (int y=0; y<3; y++)
				{	
					fmvDecal.Vertices[0].vx = -149;
					fmvDecal.Vertices[1].vx = -149;
					fmvDecal.Vertices[2].vx = -149;
					fmvDecal.Vertices[3].vx = -149;

					fmvDecal.Vertices[0].vy = -3254+y*744;
					fmvDecal.Vertices[1].vy = -3254+y*744;
					fmvDecal.Vertices[2].vy = -3254+y*744+744;
					fmvDecal.Vertices[3].vy = -3254+y*744+744;

					fmvDecal.Vertices[0].vz = 49440+z*993;
					fmvDecal.Vertices[1].vz = 49440+z*993+993;
					fmvDecal.Vertices[2].vz = 49440+z*993+993;
					fmvDecal.Vertices[3].vz = 49440+z*993;
					fmvDecal.Centre.vx = ((z+y)%3)+1;
					RenderDecal(&fmvDecal);
				}
			}
		}
		if (drawPredatorFMV!=-1)
		{
			DECAL fmvDecal =
			{
				DECAL_FMV,
			};
			fmvDecal.ModuleIndex = drawPredatorFMV;
			fmvDecal.UOffset = 0;

			UpdateFMVTextures(4);
			
			for (int z=0; z<12; z++)
			{
				for (int y=0; y<7; y++)
				{	
					fmvDecal.Vertices[0].vx = -7164;
					fmvDecal.Vertices[1].vx = -7164;
					fmvDecal.Vertices[2].vx = -7164;
					fmvDecal.Vertices[3].vx = -7164;

					fmvDecal.Vertices[0].vy = -20360+y*362;
					fmvDecal.Vertices[1].vy = -20360+y*362;
					fmvDecal.Vertices[2].vy = -20360+y*362+362;
					fmvDecal.Vertices[3].vy = -20360+y*362+362;

					fmvDecal.Vertices[0].vz = 1271+z*483+483;
					fmvDecal.Vertices[1].vz = 1271+z*483;
					fmvDecal.Vertices[2].vz = 1271+z*483;
					fmvDecal.Vertices[3].vz = 1271+z*483+483;
					fmvDecal.Centre.vx = (z+y)%3;
					RenderDecal(&fmvDecal);
				}
			}
		}
		
		#endif
		
		if (drawSwirlyFMV!=-1)
		{
			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(30000,-12500,0);
		}
		if (drawSwirlyFMV2!=-1)
		{
			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(2605,-6267-2000,17394-3200);
		}
		
		if (drawSwirlyFMV3!=-1)
		{
//			UpdateFMVTextures(1);
			D3D_DrawSwirlyFMV(5117,3456-3000,52710-2000);
		}
		if (drawOctagonPool!=-1)
		{
			#if FMV_ON
			UpdateFMVTextures(1);
			
			MeshXScale = (3000);
			MeshZScale = (4000);
			D3D_DrawFMVOnWater(-1000,3400,22000);
			{
				DECAL fmvDecal =
				{
					DECAL_FMV,
					{
					{0,-2500,29000},
					{2000,-2500,29000},
					{2000,-2500+750*2,29000},
					{0,-2500+750*2,29000}
					},
					0
				};
				fmvDecal.ModuleIndex = drawOctagonPool;
				fmvDecal.Centre.vx = 0;
				fmvDecal.UOffset = 0;

				RenderDecal(&fmvDecal);
			}
			#endif

			int highDetailRequired = 1;
			int x = 1023;
			int y = 3400;
			int z = 27536;
			
			{
				int dx = Player->ObWorld.vx - x;
				if (dx< -8000 || dx > 8000)
				{
					highDetailRequired = 0;
				}
				else
				{
					int dz = Player->ObWorld.vz - z;
					if (dz< -8000 || dz > 8000)
					{
						highDetailRequired = 0;
					}
				}
			}			
			MeshXScale = 7700;
			MeshZScale = 7700;
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x-MeshXScale, x+MeshXScale, z-MeshZScale, z+MeshZScale, y);
			}
			
			MeshXScale /=15;
			MeshZScale /=15;
			
			// Turn OFF texturing if it is on...
			D3DTEXTUREHANDLE TextureHandle = NULL;
			if (CurrTextureHandle != TextureHandle)
			{
				OP_STATE_RENDER(1, ExecBufInstPtr);
				STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
				CurrTextureHandle = TextureHandle;
			}	 
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
			if (NumVertices)
			{
			   WriteEndCodeToExecuteBuffer();
		  	   UnlockExecuteBufferAndPrepareForUse();
			   ExecuteBuffer();
		  	   LockExecuteBuffer();
			}
			if (highDetailRequired)
			{
				MeshXScale /= 2;
				MeshZScale /= 2;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshZScale = -MeshZScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				D3D_DrawWaterOctagonPatch(x,y,z,15,0);
				D3D_DrawWaterOctagonPatch(x,y,z,0,15);
				D3D_DrawWaterOctagonPatch(x,y,z,15,15);
			}
			else
			{
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshZScale = -MeshZScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
				MeshXScale = -MeshXScale;
				D3D_DrawWaterOctagonPatch(x,y,z,0,0);
			}

		}
	}
	#endif
#if 0 /* not yet */
	else if (!_stricmp(LevelName,"hangar"))
	{
	   	#if FMV_ON
		#if WIBBLY_FMV_ON
		UpdateFMVTextures(1);
	   	D3D_DrawFMV(FmvPosition.vx,FmvPosition.vy,FmvPosition.vz);
		#endif
		#endif
		#if 0
		{
			VECTORCH v = {49937,-4000,-37709};		// hangar
			D3D_DrawCable(&v);
		}
		#endif
	}
	else 
#endif /* not yet */	
	if (!strcasecmp(LevelName,"invasion_a"))
	{
		char drawWater = 0;
		char drawEndWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"hivepool"))
				  ||(!strcmp(modulePtr->name,"hivepool04")))
				{
					drawWater = 1;
					break;
				}
				else
				{
					if(!strcmp(modulePtr->name,"shaftbot"))
					{
						drawEndWater = 1;
					}
					if((!strcasecmp(modulePtr->name,"shaft01"))
					 ||(!strcasecmp(modulePtr->name,"shaft02"))
					 ||(!strcasecmp(modulePtr->name,"shaft03"))
					 ||(!strcasecmp(modulePtr->name,"shaft04"))
					 ||(!strcasecmp(modulePtr->name,"shaft05"))
					 ||(!strcasecmp(modulePtr->name,"shaft06")))
					{
						extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
						HandleRainShaft(modulePtr, -11726,-107080,10);
						drawEndWater = 1;
						break;
					}
				}
			}

		}	

		if (drawWater)
		{
			int x = 20767;
			int y = -36000+200;
			int z = 30238;
			MeshXScale = (36353-20767);
			MeshZScale = (41927-30238);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
		else if (drawEndWater)
		{
			int x = -15471;
			int y = -11720-500;
			int z = -55875;
			MeshXScale = (15471-1800);
			MeshZScale = (55875-36392);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}
			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)(MeshXScale+1800-3782);
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;
			

		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[WaterShaftImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
	}
	else 
	if (!strcasecmp(LevelName, "derelict"))
	{
		char drawMirrorSurfaces = 0;
		char drawWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
			  	if( (!strcasecmp(modulePtr->name,"start-en01"))
			  	  ||(!strcasecmp(modulePtr->name,"start")))
				{
					drawMirrorSurfaces = 1;
				}
				else if (!strcasecmp(modulePtr->name,"water-01"))
				{
					extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
					drawWater = 1;
					HandleRainShaft(modulePtr, 32000, 0, 16);
				}
			}
		}	

		if (drawMirrorSurfaces)
		{
			extern void RenderMirrorSurface(void);
			extern void RenderMirrorSurface2(void);
			extern void RenderParticlesInMirror(void);
			RenderParticlesInMirror();
			RenderMirrorSurface();
			RenderMirrorSurface2();
		}
		if (drawWater)
		{
			int x = -102799;
			int y = 32000;
			int z = -200964;
			MeshXScale = (102799-87216);
			MeshZScale = (200964-180986);
			{
				extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=2;
		 	MeshZScale/=2;
			
			
			/* TODO: this is a hack for the 2 pass specular color because it changes the texture/blend func */
			CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	
		 	CurrTextureHandle = ImageHeaderArray[ChromeImageNumber].D3DTexture;
			CheckBoundTextureIsCorrect(CurrTextureHandle);
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		}

	}
	else if (!strcasecmp(LevelName,"genshd1"))
	{
		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcasecmp(modulePtr->name,"largespace"))
				  ||(!strcasecmp(modulePtr->name,"proc13"))
				  ||(!strcasecmp(modulePtr->name,"trench01"))
				  ||(!strcasecmp(modulePtr->name,"trench02"))
				  ||(!strcasecmp(modulePtr->name,"trench03"))
				  ||(!strcasecmp(modulePtr->name,"trench04"))
				  ||(!strcasecmp(modulePtr->name,"trench05"))
				  ||(!strcasecmp(modulePtr->name,"trench06"))
				  ||(!strcasecmp(modulePtr->name,"trench07"))
				  ||(!strcasecmp(modulePtr->name,"trench08"))
				  ||(!strcasecmp(modulePtr->name,"trench09")))
				{
					extern void HandleRain(int numberOfRaindrops);
					HandleRain(999);
					break;
				}
			}

		}	
	}
}

void D3D_DrawWaterTest(MODULE *testModulePtr)
{
#if 0 /* not yet */
	extern char LevelName[];
	if (!strcmp(LevelName,"genshd1"))
	{
		extern DISPLAYBLOCK *Player;

//		DISPLAYBLOCK *objectPtr = OnScreenBlockList[numOfObjects];
		MODULE *modulePtr = testModulePtr;//objectPtr->ObMyModule;
#if 0
		if (testModulePtr && testModulePtr->name)
		if(!strcmp(testModulePtr->name,"LargeSpace"))
		{
			extern void HandleRain(int numberOfRaindrops);
			HandleRain(999);
		}
#endif
		if (modulePtr && modulePtr->name)
		{
			if (!strcmp(modulePtr->name,"05"))
			{
				int y = modulePtr->m_maxy+modulePtr->m_world.vy-500;
		   		int x = modulePtr->m_minx+modulePtr->m_world.vx;
		   		int z = modulePtr->m_minz+modulePtr->m_world.vz;
				MeshXScale = (7791 - -7794);
				MeshZScale = (23378 - 7793);
				{
					extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
					CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
				}
				D3DTEXTUREHANDLE TextureHandle = (D3DTEXTUREHANDLE)ImageHeaderArray[WaterShaftImageNumber].D3DHandle;
				if (CurrTextureHandle != TextureHandle)
				{
					OP_STATE_RENDER(1, ExecBufInstPtr);
					STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
					CurrTextureHandle = TextureHandle;
				}	 
				CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);
				if (NumVertices)
				{
				   WriteEndCodeToExecuteBuffer();
			  	   UnlockExecuteBufferAndPrepareForUse();
				   ExecuteBuffer();
			  	   LockExecuteBuffer();
				}
				WaterXOrigin=x;
				WaterZOrigin=z;
				WaterUScale = 4.0f/(float)(MeshXScale);
				WaterVScale = 4.0f/(float)MeshZScale;
				#if 1
				MeshXScale/=2;
				MeshZScale/=2;
				D3D_DrawWaterPatch(x, y, z);
				D3D_DrawWaterPatch(x+MeshXScale, y, z);
				D3D_DrawWaterPatch(x, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);

				extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
				HandleRainShaft(modulePtr, y,-21000,1);
				#else
				MeshXScale/=4;
				MeshZScale/=4;
				D3D_DrawWaterPatch(x, y, z);
				D3D_DrawWaterPatch(x, y, z+MeshZScale);
				D3D_DrawWaterPatch(x, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale, y, z);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale*3);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale*2);
				D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale*3);
				HandleRainDrops(modulePtr,2);
				#endif
			}
		}
	}
	#if 0
	else if ( (!_stricmp(LevelName,"e3demo")) || (!_stricmp(LevelName,"e3demosp")) )
	{
		if (testModulePtr && testModulePtr->name)
		{
			#if 0
			if(!_stricmp(testModulePtr->name,"watermid"))
			{
				DECAL fmvDecal =
				{
					DECAL_FMV,
					{
					{0,-2500,29000},
					{2000,-2500,29000},
					{2000,-2500+750*2,29000},
					{0,-2500+750*2,29000}
					},
					0
				};
				fmvDecal.ModuleIndex = testModulePtr->m_index;
				fmvDecal.Centre.vx = 0;
				fmvDecal.UOffset = 0;

				RenderDecal(&fmvDecal);
			}
			#endif
			if(!_stricmp(testModulePtr->name,"lowlowlo03"))
			{
				VECTORCH position = {6894,469,-13203};
				VECTORCH disp = position;
				int i,d;

				disp.vx -= Player->ObWorld.vx;
				disp.vy -= Player->ObWorld.vy;
				disp.vz -= Player->ObWorld.vz;
				d = ONE_FIXED - Approximate3dMagnitude(&disp)*2;
				if (d<0) d = 0;

				i = MUL_FIXED(10,d);
				while(i--)
				{
					VECTORCH velocity;
					velocity.vx = ((FastRandom()&1023) - 512);
					velocity.vy = ((FastRandom()&1023) - 512)+2000;
					velocity.vz = (1000+(FastRandom()&255))*2;
					MakeParticle(&(position),&(velocity),PARTICLE_STEAM);
				}
			}
		}
	}
	#endif
#endif /* not yet */
}


/* TODO: doubled this from 256 to 512 because of overflows in DrawMoltenMetal_Clipped */
VECTORCH MeshVertex[512];
#define TEXTURE_WATER 0

VECTORCH MeshWorldVertex[512];
unsigned int MeshVertexColour[512];
unsigned int MeshVertexSpecular[512];
char MeshVertexOutcode[512];

void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin)
{
	int i=0;
	int x;
	int offset;
	
	for (x=0; x<16; x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			
			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;


			offset=0;

		 #if 1
			/* basic noise ripples */
//		 	offset = MUL_FIXED(32,GetSin(  (point->vx+point->vz+CloakingPhase)&4095 ) );
//		 	offset += MUL_FIXED(16,GetSin(  (point->vx-point->vz*2+CloakingPhase/2)&4095 ) );

			{
 				offset += EffectOfRipples(point);
			}
		#endif
	//		if (offset>450) offset = 450;
	//		if (offset<-450) offset = -450;
			point->vy = yOrigin+offset;

			#if 0
			MeshVertexColour[i] = LightSourceWaterPoint(point,offset);
			#else
			{
				int alpha = 128-offset/4;
		//		if (alpha>255) alpha = 255;
		//		if (alpha<128) alpha = 128;
				switch (CurrentVisionMode)
				{
					default:
					case VISION_MODE_NORMAL:
					{
//						MeshVertexColour[i] = RGBALIGHT_MAKE(10,51,28,alpha);
						MeshVertexColour[i] = RGBA_MAKE(255,255,255,alpha);
						#if 0
						#if 1
						VECTORCH pos = {24087,yOrigin,39165};
						int c = (8191-VectorDistance(&pos,point));
						if (c<0) c=0;
						else
						{
							int s = GetSin((CloakingPhase/2)&4095);
							s = MUL_FIXED(s,s)/64;
							c = MUL_FIXED(s,c);
						}
						MeshVertexSpecular[i] = (c<<16)+(((c/4)<<8)&0xff00) + (c/4);
						#else 
						if (!(FastRandom()&1023))
						{
							MeshVertexSpecular[i] = 0xc04040;
						}
						else
						{
							MeshVertexSpecular[i] = 0;
						}
						#endif
						#endif
						break;
					}
					case VISION_MODE_IMAGEINTENSIFIER:
					{
						MeshVertexColour[i] = RGBA_MAKE(0,51,0,alpha);
						break;
					}
					case VISION_MODE_PRED_THERMAL:
					case VISION_MODE_PRED_SEEALIENS:
					case VISION_MODE_PRED_SEEPREDTECH:
					{
						MeshVertexColour[i] = RGBA_MAKE(0,0,28,alpha);
					  	break;
					}
				}

			}
			#endif

			#if 1
			MeshWorldVertex[i].vx = ((point->vx-WaterXOrigin)/4+MUL_FIXED(GetSin((point->vy*16)&4095),128));			
			MeshWorldVertex[i].vy = ((point->vz-WaterZOrigin)/4+MUL_FIXED(GetSin((point->vy*16+200)&4095),128));			
			#endif
			
			#if 1
			TranslatePointIntoViewspace(point);
			#else
			point->vx -= Global_VDB_Ptr->VDB_World.vx;
			point->vy -= Global_VDB_Ptr->VDB_World.vy;
			point->vz -= Global_VDB_Ptr->VDB_World.vz;
			RotateVector(point,&(Global_VDB_Ptr->VDB_Mat));
			point->vy = MUL_FIXED(point->vy,87381);

			#endif
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}

	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
//		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawMoltenMetalMesh_Clipped();
//		D3D_DrawWaterMesh_Clipped();
	}
		
	
}

#if 0 /* not yet */

void D3D_DrawWaterMesh_Unclipped(void)
{
	float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	/* OUTPUT VERTICES TO EXECUTE BUFFER */
	{
		D3DTLVERTEX *vertexPtr = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];
		VECTORCH *point = MeshVertex;
		#if TEXTURE_WATER
		VECTORCH *pointWS = MeshWorldVertex;
		#endif
		int i;
		for (i=0; i<256; i++)
		{

			if (point->vz<=1) point->vz = 1;
			int x = (point->vx*(Global_VDB_Ptr->VDB_ProjX))/point->vz+Global_VDB_Ptr->VDB_CentreX;
			int y = (point->vy*(Global_VDB_Ptr->VDB_ProjY))/point->vz+Global_VDB_Ptr->VDB_CentreY;
  //			textprint("%d, %d\n",x,y);
			#if 1
			{
				if (x<Global_VDB_Ptr->VDB_ClipLeft)
				{
					x=Global_VDB_Ptr->VDB_ClipLeft;
				}	
				else if (x>Global_VDB_Ptr->VDB_ClipRight)
				{
					x=Global_VDB_Ptr->VDB_ClipRight;	
				}
				
				vertexPtr->sx=x;
			}
			{
				if (y<Global_VDB_Ptr->VDB_ClipUp)
				{
					y=Global_VDB_Ptr->VDB_ClipUp;
				}
				else if (y>Global_VDB_Ptr->VDB_ClipDown)
				{
					y=Global_VDB_Ptr->VDB_ClipDown;	
				}
				vertexPtr->sy=y;
			}
			#else
			vertexPtr->sx=x;
			vertexPtr->sy=y;
			#endif
			#if FOG_ON
			{
				int fog = (point->vz)/FOG_SCALE;
				if (fog<0) fog=0;
			 	if (fog>254) fog=254;
				fog=255-fog;
			   	vertexPtr->specular=RGBALIGHT_MAKE(0,0,0,fog);
			}
			#endif
			point->vz+=HeadUpDisplayZOffset;
		  	float oneOverZ = ((float)(point->vz)-ZNear)/(float)(point->vz);
		  //vertexPtr->color = RGBALIGHT_MAKE(66,70,0,127+(FastRandom()&63));
			vertexPtr->color = MeshVertexColour[i];
			vertexPtr->sz = oneOverZ;
			#if TEXTURE_WATER
			vertexPtr->tu = pointWS->vx/128.0;
			vertexPtr->tv =	pointWS->vz/128.0;
			#endif


			NumVertices++;
			vertexPtr++;
			point++;
			#if TEXTURE_WATER
			pointWS++;
			#endif
		}
	}
 //	textprint("numvertices %d\n",NumVertices);
    
    
    /*
     * Make sure that the triangle data (not OP) will be QWORD aligned
     */
	if (QWORD_ALIGNED(ExecBufInstPtr))
    {
        OP_NOP(ExecBufInstPtr);
    }

  	OP_TRIANGLE_LIST(450, ExecBufInstPtr);
	/* CONSTRUCT POLYS */
	{
		int x;
		for (x=0; x<15; x++)
		{
			int y;
			for(y=0; y<15; y++)
			{
				OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
				OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);
			}
		}
	}
	#if 1
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	#endif
}
void D3D_DrawWaterMesh_Clipped(void)
{
	float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	/* OUTPUT VERTICES TO EXECUTE BUFFER */
	{
		D3DTLVERTEX *vertexPtr = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];
		VECTORCH *point = MeshVertex;
		#if TEXTURE_WATER
		VECTORCH *pointWS = MeshWorldVertex;
		#endif
		int i;
		for (i=0; i<256; i++)
		{
			{
				if (point->vz<=1) point->vz = 1;
				int x = (point->vx*(Global_VDB_Ptr->VDB_ProjX))/point->vz+Global_VDB_Ptr->VDB_CentreX;
				int y = (point->vy*(Global_VDB_Ptr->VDB_ProjY))/point->vz+Global_VDB_Ptr->VDB_CentreY;
				#if 1
				{
					if (x<Global_VDB_Ptr->VDB_ClipLeft)
					{
						x=Global_VDB_Ptr->VDB_ClipLeft;
					}	
					else if (x>Global_VDB_Ptr->VDB_ClipRight)
					{
						x=Global_VDB_Ptr->VDB_ClipRight;	
					}
					
					vertexPtr->sx=x;
				}
				{
					if (y<Global_VDB_Ptr->VDB_ClipUp)
					{
						y=Global_VDB_Ptr->VDB_ClipUp;
					}
					else if (y>Global_VDB_Ptr->VDB_ClipDown)
					{
						y=Global_VDB_Ptr->VDB_ClipDown;	
					}
					vertexPtr->sy=y;
				}
				#else
				vertexPtr->sx=x;
				vertexPtr->sy=y;
				#endif
				#if FOG_ON
				{
					int fog = ((point->vz)/FOG_SCALE);
					if (fog<0) fog=0;
				 	if (fog>254) fog=254;
					fog=255-fog;
				   	vertexPtr->specular=RGBALIGHT_MAKE(0,0,0,fog);
				}
				#endif
				#if TEXTURE_WATER
				vertexPtr->tu = pointWS->vx/128.0;
				vertexPtr->tv =	pointWS->vz/128.0;
				#endif
				point->vz+=HeadUpDisplayZOffset;
			  	float oneOverZ = ((float)(point->vz)-ZNear)/(float)(point->vz);
			  //	vertexPtr->color = RGBALIGHT_MAKE(66,70,0,127+(FastRandom()&63));
				vertexPtr->color = MeshVertexColour[i];
				vertexPtr->sz = oneOverZ;
			}
			NumVertices++;
			vertexPtr++;
			point++;
			#if TEXTURE_WATER
			pointWS++;
			#endif
		}
	}
//	textprint("numvertices %d\n",NumVertices);
	/* CONSTRUCT POLYS */
	{
		int x;
		for (x=0; x<15; x++)
		{
			int y;
			for(y=0; y<15; y++)
			{
				#if 1
				int p1 = 0+x+(16*y);
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);
				int p4 = 17+x+(16*y);

				if (MeshVertexOutcode[p1]||MeshVertexOutcode[p2]||MeshVertexOutcode[p3])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
				}
				if (MeshVertexOutcode[p2]||MeshVertexOutcode[p3]||MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	
				#else
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);

				if (MeshVertexOutcode[p2]&&MeshVertexOutcode[p3])
				{
					int p1 = 0+x+(16*y);
					int p4 = 17+x+(16*y);
					if (MeshVertexOutcode[p1])
					{
						OP_TRIANGLE_LIST(1, ExecBufInstPtr);
						OUTPUT_TRIANGLE(p1,p2,p3, 256);
					}
					if (MeshVertexOutcode[p4])
					{
						OP_TRIANGLE_LIST(1, ExecBufInstPtr);
						OUTPUT_TRIANGLE(p2,p4,p3, 256);
					}
				}	
				#endif				
			}
		}
	}
	#if 1
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	#endif
}

signed int ForceFieldPointDisplacement[15*3+1][16];
signed int ForceFieldPointDisplacement2[15*3+1][16];
signed int ForceFieldPointVelocity[15*3+1][16];
unsigned char ForceFieldPointColour1[15*3+1][16];
unsigned char ForceFieldPointColour2[15*3+1][16];

int Phase=0;
int ForceFieldPhase=0;
void InitForceField(void)
{
	for (int x=0; x<15*3+1; x++)
		for (int y=0; y<16; y++)
		{
			ForceFieldPointDisplacement[x][y]=0;
			ForceFieldPointDisplacement2[x][y]=0;
			ForceFieldPointVelocity[x][y]=0;
		}
	ForceFieldPhase=0;
}
#if 1

void UpdateForceField(void)
{
	#if 1
	Phase+=NormalFrameTime>>6;
	ForceFieldPhase+=NormalFrameTime>>5;
	int x;
	for (x=1; x<15*3; x++)
	{
		int y;
		for (y=1; y<15; y++)
		{
			
			int acceleration =32*(-8*ForceFieldPointDisplacement[x][y]
								+ForceFieldPointDisplacement[x-1][y-1]
								+ForceFieldPointDisplacement[x-1][y]
								+ForceFieldPointDisplacement[x-1][y+1]
								+ForceFieldPointDisplacement[x][y-1]
								+ForceFieldPointDisplacement[x][y+1]
#if 0
								)
#else								

								+ForceFieldPointDisplacement[x+1][y-1]
								+ForceFieldPointDisplacement[x+1][y]
								+ForceFieldPointDisplacement[x+1][y+1])			
#endif
								-(ForceFieldPointVelocity[x][y]*5);

			ForceFieldPointVelocity[x][y] += MUL_FIXED(acceleration,NormalFrameTime);
			ForceFieldPointDisplacement2[x][y] += MUL_FIXED(ForceFieldPointVelocity[x][y],NormalFrameTime);
#if 1
			if(ForceFieldPointDisplacement2[x][y]>200) ForceFieldPointDisplacement2[x][y]=200;
			if(ForceFieldPointDisplacement2[x][y]<-200) ForceFieldPointDisplacement2[x][y]=-200;
#else
			if(ForceFieldPointDisplacement2[x][y]>512) ForceFieldPointDisplacement2[x][y]=512;
			if(ForceFieldPointDisplacement2[x][y]<-512) ForceFieldPointDisplacement2[x][y]=-512;

#endif
			{
				int offset = ForceFieldPointDisplacement2[x][y];
				int colour = ForceFieldPointVelocity[x][y]/4;

				if (offset<0) offset =-offset;
				if (colour<0) colour =-colour;
				colour=(colour+offset)/2;

				if(colour>255) colour=255;
				colour++;
				
				ForceFieldPointColour1[x][y]=FastRandom()%colour;
				ForceFieldPointColour2[x][y]=FastRandom()%colour;
			}
		}

	}
	for (x=1; x<15*3; x++)
	{
		int y;
		for (y=1; y<15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement2[x][y];
		}
	}
	{
		#if 1
	  	if(ForceFieldPhase>1000)
		{
			ForceFieldPhase=0;
			int x = 1+(FastRandom()%(15*3-2));
			int y = 1+(FastRandom()%13);
			ForceFieldPointVelocity[x][y] = 10000;
			ForceFieldPointVelocity[x][y+1] = 10000;
			ForceFieldPointVelocity[x+1][y] = 10000;
			ForceFieldPointVelocity[x+1][y+1] = 10000;
		}	
		#else
	   //	if(ForceFieldPhase>1000)
		{
			ForceFieldPhase=0;
			int x = 1+(FastRandom()%(15*3-2));
			int y = 1+(FastRandom()%13);
			ForceFieldPointVelocity[x][y] = (FastRandom()&16383)+8192;
		}
		#endif				   
	}
	#else
	int x;
	int y;
	for (y=0; y<=15; y++)
	{
		ForceFieldPointDisplacement[0][y] += (FastRandom()&127)-64;
		if(ForceFieldPointDisplacement[0][y]>512) ForceFieldPointDisplacement[0][y]=512;
		if(ForceFieldPointDisplacement[0][y]<-512) ForceFieldPointDisplacement[0][y]=-512;
		ForceFieldPointVelocity[0][y] = (FastRandom()&16383)-8192;
	}
	for (x=15*3-1; x>0; x--)
	{
		for (y=0; y<=15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
			ForceFieldPointVelocity[x][y] = ForceFieldPointVelocity[x-1][y];
		}

	}
	for (x=15*3-1; x>1; x--)
	{
		y = FastRandom()&15;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
		y = (FastRandom()&15)-1;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
	}
	#endif
}
void UpdateWaterFall(void)
{
	int x;
	int y;
	for (y=0; y<=15; y++)
	{
		ForceFieldPointDisplacement[0][y] += (FastRandom()&127)-64;
		if(ForceFieldPointDisplacement[0][y]>512) ForceFieldPointDisplacement[0][y]=512;
		if(ForceFieldPointDisplacement[0][y]<-512) ForceFieldPointDisplacement[0][y]=-512;
		ForceFieldPointVelocity[0][y] = (FastRandom()&16383)-8192;
	}
	for (x=15*3-1; x>0; x--)
	{
		for (y=0; y<=15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
			ForceFieldPointVelocity[x][y] = ForceFieldPointVelocity[x-1][y];
		}

	}
	for (x=15*3-1; x>1; x--)
	{
		y = FastRandom()&15;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
		y = (FastRandom()&15)-1;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
	}
}

#endif
void D3D_DrawForceField(int xOrigin, int yOrigin, int zOrigin, int fieldType)
{
	MeshXScale = 4096/16;
	MeshZScale = 4096/16;
	
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];
			
			switch(fieldType)
			{
				case 0:
				{
				 	point->vx = xOrigin+(x*MeshXScale);
				 	point->vy = yOrigin+(z*MeshZScale);
				 	point->vz = zOrigin+offset;
					break;
				}
				case 1:
				{	

					int theta = (z*4095)/15;
					int u = (x*65536)/45;

					int b = MUL_FIXED(2*u,(65536-u));
					int c = MUL_FIXED(u,u);
					int phi = (Phase&4095);
					int x3 = (GetSin(phi))/64;
					int y3 = 5000-(GetCos((phi*3+1000)&4095)/128);
					int z3 = (GetSin((3*phi+1324)&4095))/32;
					int x2 = -x3/2;
					int y2 = 3000;
					int z2 = -z3/4;
					int innerRadius = 100;//GetSin(u/32)/16+offset;

					point->vx = xOrigin+(b*x2+c*x3)/65536+MUL_FIXED(innerRadius,GetSin(theta));
					point->vy = yOrigin-5000+(b*y2+c*y3)/65536;
					point->vz = zOrigin+(b*z2+c*z3)/65536+MUL_FIXED(innerRadius,GetCos(theta));
					break;
				}
				case 2:
				{
					int theta = (z*4095)/15;
					int phi = (x*4095)/45;
					int innerRadius = 1000+offset;
					int outerRadius = 4000;
					

					point->vx = xOrigin+MUL_FIXED(outerRadius-MUL_FIXED(innerRadius,GetSin(theta)),GetCos(phi));
					point->vy = yOrigin+MUL_FIXED(innerRadius,GetCos(theta));
					point->vz = zOrigin+MUL_FIXED(outerRadius-MUL_FIXED(innerRadius,GetSin(theta)),GetSin(phi));
					break;
				}
				case 3:
				{	

					int theta = (x*4095)/45;
					int radius = offset+2000;
					point->vx = xOrigin+MUL_FIXED(radius,GetCos(theta));
					point->vy = yOrigin+(z*MeshZScale);
					point->vz = zOrigin+MUL_FIXED(radius,GetSin(theta));
					break;
				}
			}			

			if (offset<0) offset =-offset;
			offset+=16;

//			offset-=32;
//			if (offset<0) offset = 0;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(ForceFieldPointColour1[x][z],ForceFieldPointColour2[x][z],255,offset);
			#if TEXTURE_WATER
			MeshWorldVertex[i].vx = point->vx;			
			MeshWorldVertex[i].vz = point->vz;			
			#endif
			
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
}


void D3D_DrawPowerFence(int xOrigin, int yOrigin, int zOrigin, int xScale, int yScale, int zScale)
{
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];
			
		 	point->vx = xOrigin+(x*xScale);
		 	point->vy = yOrigin+(z*yScale);
		 	point->vz = zOrigin+(x*zScale);

			if (offset<0) offset =-offset;
			offset+=16;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(ForceFieldPointColour1[x][z],ForceFieldPointColour2[x][z],255,offset);
			
			/* translate particle into view space */
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
}

void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin)
{
	{
		int noRequired = MUL_FIXED(250,NormalFrameTime);
		for (int i=0; i<noRequired; i++)
		{
			VECTORCH velocity;
			VECTORCH position;
			position.vx = xOrigin;
			position.vy = yOrigin-(FastRandom()&511);//+45*MeshXScale;
			position.vz = zOrigin+(FastRandom()%(15*MeshZScale));

			velocity.vy = (FastRandom()&511)+512;//-((FastRandom()&1023)+2048)*8;
			velocity.vx = ((FastRandom()&511)+256)*2;
			velocity.vz = 0;//-((FastRandom()&511))*8;
			MakeParticle(&(position), &velocity, PARTICLE_WATERFALLSPRAY);
		}
		#if 0
		noRequired = MUL_FIXED(200,NormalFrameTime);
		for (i=0; i<noRequired; i++)
		{
			VECTORCH velocity;
			VECTORCH position;
			position.vx = xOrigin+(FastRandom()%(15*MeshZScale));
			position.vy = yOrigin+45*MeshXScale;
			position.vz = zOrigin;

			velocity.vy = -((FastRandom()&16383)+4096);
			velocity.vx = ((FastRandom()&4095)-2048);
			velocity.vz = -((FastRandom()&2047)+1048);
			MakeParticle(&(position), &velocity, PARTICLE_WATERFALLSPRAY);
		}
		#endif
	}
	{
		extern void RenderWaterFall(int xOrigin, int yOrigin, int zOrigin);
		//RenderWaterFall(xOrigin, yOrigin-500, zOrigin+50);
	}
   	return;
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			int offset = ForceFieldPointDisplacement[x][z];

		#if 1
			int u = (x*65536)/45;

			int b = MUL_FIXED(2*u,(65536-u));
			int c = MUL_FIXED(u,u);
			int y3 = 45*MeshXScale;
			int x3 = 5000;
			int y2 = 1*MeshXScale;
			int x2 = GetSin(CloakingPhase&4095)+GetCos((CloakingPhase*3+399)&4095);
			x2 = MUL_FIXED(x2,x2)/128;

			if (offset<0) offset =-offset;
			point->vx = xOrigin+MUL_FIXED(b,x2)+MUL_FIXED(c,x3)+offset;
			point->vy = yOrigin+MUL_FIXED(b,y2)+MUL_FIXED(c,y3);
			point->vz = zOrigin+(z*MeshZScale);
			
			if (point->vy>4742)
			{
				if (z<=4)
				{
					point->vy-=MeshXScale; 
					if (point->vy<4742) point->vy=4742;
					if (point->vx<179427) point->vx=179427;
				}
				else if (z<=8)
				{
					point->vx+=(8-z)*1000;
				}
			}

			#else
			if (offset<0) offset =-offset;
		 	point->vx = xOrigin-offset;
		 	point->vy = yOrigin+(x*MeshXScale);
		 	point->vz = zOrigin+(z*MeshZScale);
			#endif


			   	

			offset= (offset/4)+127;

//			offset-=32;
//			if (offset<0) offset = 0;

			if(offset>255) offset=255;
	  
			MeshVertexColour[i] = RGBALIGHT_MAKE(offset,offset,255,offset/2);
			#if TEXTURE_WATER
			MeshWorldVertex[i].vx = point->vx;			
			MeshWorldVertex[i].vz = point->vz;			
			#endif
			
			/* translate particle into view space */
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}	
	}
}

void D3D_DrawMoltenMetal(int xOrigin, int yOrigin, int zOrigin)
{
	int i=0;
	int x;
	for (x=0; x<16; x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			
			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;
		 #if 0
			
			int offset=0;

		 	offset = MUL_FIXED(32,GetSin(  (point->vx+point->vz+CloakingPhase)&4095 ) );
		 	offset += MUL_FIXED(16,GetSin(  (point->vx-point->vz*2+CloakingPhase/2)&4095 ) );
			{
				float dx=point->vx-22704;
				float dz=point->vz+20652;
				float a = dx*dx+dz*dz;
				a=sqrt(a);

				offset+= MUL_FIXED(200,GetSin( (((int)a-CloakingPhase)&4095)  ));
			}
		#endif
		 #if 1
			int offset=0;

			/* basic noise ripples */
		 	offset = MUL_FIXED(128,GetSin(  ((point->vx+point->vz)/16+CloakingPhase)&4095 ) );
		 	offset += MUL_FIXED(64,GetSin(  ((point->vx-point->vz*2)/4+CloakingPhase/2)&4095 ) );
		 	offset += MUL_FIXED(64,GetSin(  ((point->vx*5-point->vz)/32+CloakingPhase/5)&4095 ) );

		#endif
			if (offset>450) offset = 450;
			if (offset<-1000) offset = -1000;
			point->vy = yOrigin+offset;

			{
				int shade = 191+(offset+256)/8;
				MeshVertexColour[i] = RGBLIGHT_MAKE(shade,shade,shade);
			}
			
			#if 1
			TranslatePointIntoViewspace(point);
			#else
			point->vx -= Global_VDB_Ptr->VDB_World.vx;
			point->vy -= Global_VDB_Ptr->VDB_World.vy;
			point->vz -= Global_VDB_Ptr->VDB_World.vz;
			MeshWorldVertex[i] = *point;
			RotateVector(point,&(Global_VDB_Ptr->VDB_Mat));
			point->vy = MUL_FIXED(point->vy,87381);

			#endif
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			#if 0
			{
				// v
				MeshWorldVertex[i].vy = (offset+256)*4;
				// u 
				MeshWorldVertex[i].vx = ((MeshWorldVertex[i].vx)&4095);
				
			}
			#else
			{
				Normalise(&MeshWorldVertex[i]);
				// v
				int theta = (MeshWorldVertex[i].vy+offset);
				if (theta<0) theta=0;
				if (theta>ONE_FIXED) theta=ONE_FIXED;

				// u 
				int arctan = ((atan2((double)MeshWorldVertex[i].vx,(double)MeshWorldVertex[i].vz)/ 6.28318530718))*4095;
				MeshWorldVertex[i].vx = (arctan+offset)&4095;

				MeshWorldVertex[i].vy = ArcCos(theta);
				
			}
			#endif


			i++;
		}
	}

	D3DTEXTUREHANDLE TextureHandle = (D3DTEXTUREHANDLE)ImageHeaderArray[StaticImageNumber].D3DHandle;
	if (CurrTextureHandle != TextureHandle)
	{
		OP_STATE_RENDER(1, ExecBufInstPtr);
		STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, TextureHandle, ExecBufInstPtr);
		CurrTextureHandle = TextureHandle;
	}
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_OFF);
	if (NumVertices)
	{
	   WriteEndCodeToExecuteBuffer();
  	   UnlockExecuteBufferAndPrepareForUse();
	   ExecuteBuffer();
  	   LockExecuteBuffer();
	}
	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawMoltenMetalMesh_Clipped();
	}
		
	
}

#endif /* not yet */

void D3D_DrawMoltenMetalMesh_Unclipped(void)
{
	float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	int i, x, y, z;
	int tc;

	for (i=0; i<256; i++) {
		GLfloat xf, yf, zf;
		GLfloat sf, tf, rhw;
		int r, g, b, a;
		
		if (point->vz < 1) point->vz = 1;

		x = (point->vx*(Global_VDB_Ptr->VDB_ProjX+1))/point->vz+Global_VDB_Ptr->VDB_CentreX;
		y = (point->vy*(Global_VDB_Ptr->VDB_ProjY+1))/point->vz+Global_VDB_Ptr->VDB_CentreY;

		if (x<Global_VDB_Ptr->VDB_ClipLeft) {
			x=Global_VDB_Ptr->VDB_ClipLeft;
		} else if (x>Global_VDB_Ptr->VDB_ClipRight) {
			x=Global_VDB_Ptr->VDB_ClipRight;	
		}			

		if (y<Global_VDB_Ptr->VDB_ClipUp) {
			y=Global_VDB_Ptr->VDB_ClipUp;
		} else if (y>Global_VDB_Ptr->VDB_ClipDown) {
			y=Global_VDB_Ptr->VDB_ClipDown;	
		}
			
		sf = pointWS->vx*WaterUScale+(1.0f/256.0f);
		tf = pointWS->vy*WaterVScale+(1.0f/256.0f);
	
		z = point->vz + HeadUpDisplayZOffset;
		rhw = 1.0f / (float)point->vz;		  	
		
		b = (MeshVertexColour[i] >> 0)  & 0xFF;
		g = (MeshVertexColour[i] >> 8)  & 0xFF;
		r = (MeshVertexColour[i] >> 16) & 0xFF;
		a = (MeshVertexColour[i] >> 24) & 0xFF;
			
		xf =  ((float)x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
		yf = -((float)y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);
		zf = 1.0f - 2.0f*ZNear/(float)z;
			
		tarr[i].v[0] = xf/rhw;
		tarr[i].v[1] = yf/rhw;
		tarr[i].v[2] = zf/rhw;
		tarr[i].v[3] = 1.0f/rhw;
		
		tarr[i].t[0] = sf;
		tarr[i].t[1] = tf;
		
		tarr[i].c[0] = r;
		tarr[i].c[1] = g;
		tarr[i].c[2] = b;
		tarr[i].c[3] = a;
		
		point++;
		pointWS++;
	}
    
	/* CONSTRUCT POLYS */
	
	tc = 0;
	for (x = 0; x < 15; x++) {
		for(y = 0; y < 15; y++) {
//			OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
//			OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);
			if ((16+x+(16*y)) < 256) {
				tris[tc+0].a = 0+x+(16*y);
				tris[tc+0].b = 1+x+(16*y);
				tris[tc+0].c = 16+x+(16*y);
				tris[tc+1].a = 1+x+(16*y);
				tris[tc+1].b = 17+x+(16*y);
				tris[tc+1].c = 16+x+(16*y);
			
				tc += 2;
			}
		}
	}
	DrawTriangleArray_T2F_C4UB_V4F(tc);
}

void D3D_DrawMoltenMetalMesh_Clipped(void)
{
	int i, x, y, z;
	
	float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);	

	{
		VECTORCH *point = MeshVertex;
		VECTORCH *pointWS = MeshWorldVertex;

		for (i=0; i<256; i++)
		{
			GLfloat xf, yf, zf;
			GLfloat sf, tf, rhw;
			int r, g, b, a;
			
			if (point->vz < 1) point->vz = 1;
			
			x = (point->vx*(Global_VDB_Ptr->VDB_ProjX+1))/point->vz+Global_VDB_Ptr->VDB_CentreX;
			y = (point->vy*(Global_VDB_Ptr->VDB_ProjY+1))/point->vz+Global_VDB_Ptr->VDB_CentreY;

			if (x<Global_VDB_Ptr->VDB_ClipLeft) {
				x=Global_VDB_Ptr->VDB_ClipLeft;
			} else if (x>Global_VDB_Ptr->VDB_ClipRight) {
				x=Global_VDB_Ptr->VDB_ClipRight;	
			}
				
			if (y<Global_VDB_Ptr->VDB_ClipUp) {
				y=Global_VDB_Ptr->VDB_ClipUp;
			} else if (y>Global_VDB_Ptr->VDB_ClipDown) {
				y=Global_VDB_Ptr->VDB_ClipDown;	
			}
			
			sf = pointWS->vx*WaterUScale+(1.0f/256.0f);
			tf = pointWS->vy*WaterVScale+(1.0f/256.0f);
	
			z = point->vz + HeadUpDisplayZOffset;
		  	rhw = 1.0f / (float)point->vz;
		  	
			b = (MeshVertexColour[i] >> 0)  & 0xFF;
			g = (MeshVertexColour[i] >> 8)  & 0xFF;
			r = (MeshVertexColour[i] >> 16) & 0xFF;
			a = (MeshVertexColour[i] >> 24) & 0xFF;
			
			xf =  ((float)x - (float)ScreenDescriptorBlock.SDB_CentreX - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreX - 0.5f);
			yf = -((float)y - (float)ScreenDescriptorBlock.SDB_CentreY - 0.5f) / ((float)ScreenDescriptorBlock.SDB_CentreY - 0.5f);
			zf = 1.0f - 2.0f*ZNear/(float)z;
			
			tarr[i].v[0] = xf/rhw;
			tarr[i].v[1] = yf/rhw;
			tarr[i].v[2] = zf/rhw;
			tarr[i].v[3] = 1.0f/rhw;
			
			tarr[i].t[0] = sf;
			tarr[i].t[1] = tf;
			
			tarr[i].c[0] = r;
			tarr[i].c[1] = g;
			tarr[i].c[2] = b;
			tarr[i].c[3] = a;
			
			point++;
			pointWS++;
		}
	}

	/* CONSTRUCT POLYS */
	{
		int tc = 0;
		
		for (x=0; x<15; x++)
		{
			for(y=0; y<15; y++)
			{
				int p1 = 0+x+(16*y);
				int p2 = 1+x+(16*y);
				int p3 = 16+x+(16*y);
				int p4 = 17+x+(16*y);
				
				if (p3 > 255)
					continue;
#if 0
				#if 0
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
				}
				if (MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(1, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	
				#else
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4])
				{
					OP_TRIANGLE_LIST(2, ExecBufInstPtr);
					OUTPUT_TRIANGLE(p1,p2,p3, 256);
					OUTPUT_TRIANGLE(p2,p4,p3, 256);
				}	

				#endif
#endif
				if (MeshVertexOutcode[p1]&&MeshVertexOutcode[p2]&&MeshVertexOutcode[p3]&&MeshVertexOutcode[p4]) {
					tris[tc+0].a = p1;
					tris[tc+0].b = p2;
					tris[tc+0].c = p3;
					tris[tc+1].a = p2;
					tris[tc+1].b = p4;
					tris[tc+1].c = p3;
					tc += 2;
				}
			}
		}
		DrawTriangleArray_T2F_C4UB_V4F(tc);
	}
	{
		POLYHEADER fakeHeader;

		fakeHeader.PolyFlags = 0;
		fakeHeader.PolyColour = 0;
		RenderPolygon.TranslucencyMode = TRANSLUCENCY_NORMAL;
		
		for (x=0; x<15; x++)
		{
			for(y=0; y<15; y++)
			{
				int p[4];
				p[0] = 0+x+(16*y);
				p[1] = 1+x+(16*y);
				p[2] = 17+x+(16*y);
				p[3] = 16+x+(16*y);

				if (p[3] > 255)
					continue;
					
				if (!(MeshVertexOutcode[p[0]]&&MeshVertexOutcode[p[1]]&&MeshVertexOutcode[p[2]]&&MeshVertexOutcode[p[3]]))
				{
					for (i=0; i<4; i++) 
					{
						VerticesBuffer[i].X	= MeshVertex[p[i]].vx;
						VerticesBuffer[i].Y	= MeshVertex[p[i]].vy;
						VerticesBuffer[i].Z	= MeshVertex[p[i]].vz;
						VerticesBuffer[i].U = MeshWorldVertex[p[i]].vx*(WaterUScale*128.0f*65536.0f);
						VerticesBuffer[i].V = MeshWorldVertex[p[i]].vy*(WaterVScale*128.0f*65536.0f);
															   
						VerticesBuffer[i].A = (MeshVertexColour[p[i]]&0xff000000)>>24;
						VerticesBuffer[i].R = (MeshVertexColour[p[i]]&0x00ff0000)>>16;
						VerticesBuffer[i].G	= (MeshVertexColour[p[i]]&0x0000ff00)>>8;
						VerticesBuffer[i].B = MeshVertexColour[p[i]]&0x000000ff;
						VerticesBuffer[i].SpecularR = 0;
						VerticesBuffer[i].SpecularG = 0;
						VerticesBuffer[i].SpecularB = 0;
						RenderPolygon.NumberOfVertices=4;
						
					}
					if (QuadWithinFrustrum())
					{		 
						GouraudTexturedPolygon_ClipWithZ();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithNegativeX();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithPositiveY();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithNegativeY();
						if(RenderPolygon.NumberOfVertices<3) continue;
						GouraudTexturedPolygon_ClipWithPositiveX();
						if(RenderPolygon.NumberOfVertices<3) continue;
					   //	D3D_ZBufferedGouraudPolygon_Output(&fakeHeader,RenderPolygon.Vertices);
				   	   	D3D_ZBufferedGouraudTexturedPolygon_Output(&fakeHeader,RenderPolygon.Vertices);
					}
				}
			}
		}
	}
}

#if 0 /* not yet */

void D3D_DrawWaterOctagonPatch(int xOrigin, int yOrigin, int zOrigin, int xOffset, int zOffset)
{
	float grad = 2.414213562373;
	int i=0;
	int x;
	for (x=xOffset; x<16+xOffset; x++)
	{
		int z;
		for(z=zOffset; z<16+zOffset; z++)
		{
			VECTORCH *point = &MeshVertex[i];

		  	if (x>z)
			{
				float m,xs;
				if (x!=0)
				{
					m = (float)(z)/(float)(x);
					xs = grad/(grad+m);
				}
				else
				{
					xs = 0;
				}
				#if 1
				f2i(point->vx , xs*x*MeshXScale);
				f2i(point->vz , (grad-grad*xs)*x*MeshZScale);
				#else
				point->vx = xs*x*MeshXScale;
				point->vz = (grad-grad*xs)*x*MeshZScale;
				#endif
			}
			else
			{
				float m,xs;
				if (z!=0)
				{
					m = (float)(x)/(float)(z);
					xs = grad/(grad+m);
				}
				else
				{
					xs = 0;
				}
				#if 1
				f2i(point->vz ,	xs*z*MeshZScale);
				f2i(point->vx ,	(grad-grad*xs)*z*MeshXScale);
				#else
				point->vz =	xs*z*MeshZScale;
				point->vx =	(grad-grad*xs)*z*MeshXScale;
				#endif
			}

			point->vx += xOrigin;
			point->vz += zOrigin;

			int offset = EffectOfRipples(point);
			
			point->vy = yOrigin+offset;

			#if 0
			MeshVertexColour[i] = LightSourceWaterPoint(point,offset);
			#else
			{
				int alpha = 128-offset/4;
		//		if (alpha>255) alpha = 255;
		//		if (alpha<128) alpha = 128;
				switch (CurrentVisionMode)
				{
					default:
					case VISION_MODE_NORMAL:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(10,51,28,alpha);
						break;
					}
					case VISION_MODE_IMAGEINTENSIFIER:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,51,0,alpha);
						break;
					}
					case VISION_MODE_PRED_THERMAL:
					case VISION_MODE_PRED_SEEALIENS:
					case VISION_MODE_PRED_SEEPREDTECH:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,0,28,alpha);
					  	break;
					}
				}

			}
			#endif
			TranslatePointIntoViewspace(point);
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}

	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawWaterMesh_Clipped();
	}
		
	
}

void D3D_DrawCable(VECTORCH *centrePtr, MATRIXCH *orientationPtr)
{
	{
			// Turn OFF texturing if it is on...
			if (CurrTextureHandle != NULL)
			{
				OP_STATE_RENDER(1, ExecBufInstPtr);
				STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, NULL, ExecBufInstPtr);
				CurrTextureHandle = NULL;
			}
	
			CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	
			if (NumVertices)
			{
			   WriteEndCodeToExecuteBuffer();
		  	   UnlockExecuteBufferAndPrepareForUse();
			   ExecuteBuffer();
		  	   LockExecuteBuffer();
			}
	
			OP_STATE_RENDER(1, ExecBufInstPtr);
			STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, FALSE, ExecBufInstPtr);
	
	}
	MeshXScale = 4096/16;
	MeshZScale = 4096/16;
	
	for (int field=0; field<3; field++)
	{
	int i=0;			   
	int x;
	for (x=(0+field*15); x<(16+field*15); x++)
	{
		int z;
		for(z=0; z<16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			{	
				int innerRadius = 20;
				VECTORCH radius;
				int theta = ((4096*z)/15)&4095;
				int rOffset = GetSin((x*64+theta/32-CloakingPhase)&4095);
				rOffset = MUL_FIXED(rOffset,rOffset)/512;


				radius.vx = MUL_FIXED(innerRadius+rOffset/8,GetSin(theta));
				radius.vy = MUL_FIXED(innerRadius+rOffset/8,GetCos(theta));
				radius.vz = 0;
				
				RotateVector(&radius,orientationPtr);

				point->vx = centrePtr[x].vx+radius.vx;
				point->vy = centrePtr[x].vy+radius.vy;
				point->vz = centrePtr[x].vz+radius.vz;

				MeshVertexColour[i] = RGBALIGHT_MAKE(0,rOffset,255,128);

			}
			
			TranslatePointIntoViewspace(point);
			
			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}
	//textprint("\n");
   	if ((MeshVertexOutcode[0]&&MeshVertexOutcode[15]&&MeshVertexOutcode[240]&&MeshVertexOutcode[255]))
	{
		D3D_DrawMoltenMetalMesh_Unclipped();
	   //	D3D_DrawWaterMesh_Unclipped();
	}	
	else
//	else if (MeshVertexOutcode[0]||MeshVertexOutcode[15]||MeshVertexOutcode[240]||MeshVertexOutcode[255])
	{
		D3D_DrawMoltenMetalMesh_Clipped();
  	   //	D3D_DrawWaterMesh_Clipped();
	}	
	}
			OP_STATE_RENDER(1, ExecBufInstPtr);
			STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, TRUE, ExecBufInstPtr);
}

#endif

#if 0
/* ** menu-type stuff that should be moved later ** */
#include "avp_menugfx.hpp"

int Hardware_RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format) 
{
	switch(format)
	{
		default:
//		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while(*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while(*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length/2;
			break;
		}	
	}

//	LOCALASSERT(x>0);

	{
		unsigned int colour = alpha>>8;
		if (colour>255) colour = 255;
		colour = (colour<<24)+0xffffff;
		D3D_RenderHUDString(textPtr,x,y,colour);
	}
	return x;
}

void RenderBriefingText(int centreY, int brightness)
{
	int lengthOfLongestLine=-1;
	int x,y,i;

	for(i=0; i<5; i++)
	{
		int length = 0;
		{
			char *ptr = BriefingTextString[i];

			while(*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}
		}
		
		if (lengthOfLongestLine < length)
		{
			lengthOfLongestLine = length;
		}
	}

	x = (ScreenDescriptorBlock.SDB_Width-lengthOfLongestLine)/2;
	y = centreY - 3*HUD_FONT_HEIGHT;
	for(i=0; i<5; i++)
	{
//		if (AvPMenus.MenusState != MENUSSTATE_MAINMENUS)
		{
			Hardware_RenderSmallMenuText(BriefingTextString[i], x, y, brightness, AVPMENUFORMAT_LEFTJUSTIFIED/*,MENU_CENTREY-60-100,MENU_CENTREY-60+180*/);
//		}
//		else
//		{
//			RenderSmallMenuText(BriefingTextString[i], x, y, brightness, AVPMENUFORMAT_LEFTJUSTIFIED);
//		}
		if (i) y+=HUD_FONT_HEIGHT;
		else y+=HUD_FONT_HEIGHT*2;
	}
}
#endif
