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
#include "bh_types.h"
#include "showcmds.h"
#include "d3d_hud.h"
#include "hud_layout.h"
#include "avp_userprofile.h"
#include "aw.h"


extern IMAGEHEADER ImageHeaderArray[];
extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern unsigned char GammaValues[256];
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern int SpecialFXImageNumber;
extern int StaticImageNumber;
extern int PredatorNumbersImageNumber;
extern int BurningImageNumber;
extern int HUDFontsImageNumber;

extern int FMVParticleColour;
extern int HUDScaleFactor;
extern int CloakingPhase;

static D3DTexture *CurrTextureHandle;


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
		default:
			fprintf(stderr, "RenderPolygon.TranslucencyMode: invalid %d\n", RenderPolygon.TranslucencyMode);
			return;
	}
	
	if (mode != TRANSLUCENCY_OFF && CurrentTranslucencyMode == TRANSLUCENCY_OFF)
		glEnable(GL_BLEND);
		
	CurrentTranslucencyMode = mode;
}

#define TA_MAXVERTICES		8
typedef struct TriangleArray
{
	int elements;
	
	GLfloat v[TA_MAXVERTICES*4];
	
	GLfloat t[TA_MAXVERTICES*4];
	
	GLubyte c[TA_MAXVERTICES*4];
} TriangleArray;

TriangleArray tarr;

static void DrawTriangles_T2F_C4UB_V4F()
{
	
#define OUTPUT_VERTEX(d) \
{ \
	glColor4ubv	(&tarr.c[(d) * 4]);	\
	glTexCoord2fv	(&tarr.t[(d) * 4]);	\
	glVertex4fv	(&tarr.v[(d) * 4]);	\
}
#define OUTPUT_TRIANGLE(a, b, c) \
{ \
	OUTPUT_VERTEX((a));	\
	OUTPUT_VERTEX((b));	\
	OUTPUT_VERTEX((c));	\
}

	glBegin(GL_TRIANGLES);
	switch(tarr.elements) {
		case 3:
			OUTPUT_TRIANGLE(0, 2, 1);
			break;
#if 0			
		case 4:
			OUTPUT_TRIANGLE(0, 1, 2);
			OUTPUT_TRIANGLE(0, 2, 3);
			break;
#endif			
		case 5:
			OUTPUT_TRIANGLE(0, 1, 4);
			OUTPUT_TRIANGLE(1, 3, 4);
			OUTPUT_TRIANGLE(1, 2, 3);
			break;
#if 0			
		case 6:
			OUTPUT_TRIANGLE(0, 4, 5);
			OUTPUT_TRIANGLE(0, 3, 4);
			OUTPUT_TRIANGLE(0, 2, 3);
			OUTPUT_TRIANGLE(0, 1, 2);
			break;
		case 7:
			OUTPUT_TRIANGLE(0, 5, 6);
			OUTPUT_TRIANGLE(0, 4, 5);
			OUTPUT_TRIANGLE(0, 3, 4);
			OUTPUT_TRIANGLE(0, 2, 3);
			OUTPUT_TRIANGLE(0, 1, 2);
			break;
#endif			
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
			fprintf(stderr, "DrawTriangles_T2F_C4UB_V4F: tarr.elements = %d\n", tarr.elements);
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
	CheckBoundTextureIsCorrect(0);
	
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
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);

	RecipW = (1.0f/65536.0f)/128.0f;
	RecipH = (1.0f/65536.0f)/128.0f;
	
	CheckBoundTextureIsCorrect(TextureHandle->id);
	
//	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);
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

#if 0
		glColor4ub(GammaValues[vertices->R], GammaValues[vertices->G], GammaValues[vertices->B], vertices->A);

	/* they both work. */
#if 0		
		glTexCoord4f(s*rhw, t*rhw, 0, rhw);
		glVertex3f(x, y, z);
#else
		glTexCoord2f(s, t);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
#endif

#endif
		
		tarr.v[4*i+0] = x/rhw;
		tarr.v[4*i+1] = y/rhw;
		tarr.v[4*i+2] = z/rhw;
		tarr.v[4*i+3] = 1/rhw;
		
		tarr.t[4*i+0] = s;
		tarr.t[4*i+1] = t;
		
		tarr.c[4*i+0] = GammaValues[vertices->R];
		tarr.c[4*i+1] = GammaValues[vertices->G];
		tarr.c[4*i+2] = GammaValues[vertices->B];
		tarr.c[4*i+3] = vertices->A;
	}
//	glEnd();

	tarr.elements = RenderPolygon.NumberOfVertices;
	DrawTriangles_T2F_C4UB_V4F();
		
	CurrTextureHandle = TextureHandle;
}

void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
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
	} else {
		TextureHandle = CurrTextureHandle;
	}
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);

	RecipW = (1.0f/65536.0f)/128.0f;
	RecipH = (1.0f/65536.0f)/128.0f;
	
	CheckBoundTextureIsCorrect(TextureHandle->id);
	
//	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);
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

//		zvalue = vertices->Z+HeadUpDisplayZOffset;
//		z = 1.0 - 2*ZNear/zvalue;
		z = 1.0f;

#if 0		
		glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);

	/* they both work. */
#if 0		
		glTexCoord4f(s*rhw, t*rhw, 0, rhw);
		glVertex3f(x, y, z);
#else
		glTexCoord2f(s, t);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
#endif
#endif
		tarr.v[4*i+0] = x/rhw;
		tarr.v[4*i+1] = y/rhw;
		tarr.v[4*i+2] = z/rhw;
		tarr.v[4*i+3] = 1/rhw;
		
		tarr.t[4*i+0] = s;
		tarr.t[4*i+1] = t;
		
		tarr.c[4*i+0] = vertices->R;
		tarr.c[4*i+1] = vertices->G;
		tarr.c[4*i+2] = vertices->B;
		tarr.c[4*i+3] = vertices->A;		
	}
//	glEnd();

	tarr.elements = RenderPolygon.NumberOfVertices;
	DrawTriangles_T2F_C4UB_V4F();
	
	CurrTextureHandle = TextureHandle;
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
	
	CheckBoundTextureIsCorrect(TextureHandle->id);
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
	
//	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);                
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

#if 0
		glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);

	/* they both work. */
#if 0		
		glTexCoord4f(s*rhw, t*rhw, 0, rhw);
		glVertex3f(x, y, z);
#else
		glTexCoord2f(s, t);
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
#endif
#endif		
		tarr.v[4*i+0] = x/rhw;
		tarr.v[4*i+1] = y/rhw;
		tarr.v[4*i+2] = z/rhw;
		tarr.v[4*i+3] = 1/rhw;
		
		tarr.t[4*i+0] = s;
		tarr.t[4*i+1] = t;
		
		tarr.c[4*i+0] = vertices->R;
		tarr.c[4*i+1] = vertices->G;
		tarr.c[4*i+2] = vertices->B;
		tarr.c[4*i+3] = vertices->A;
	}
//	glEnd();

	tarr.elements = RenderPolygon.NumberOfVertices;
	DrawTriangles_T2F_C4UB_V4F();
	
	CurrTextureHandle = TextureHandle;
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
		CheckBoundTextureIsCorrect(0);
		
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
		
		CheckBoundTextureIsCorrect(TextureHandle->id);
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
	
	SelectPolygonBeginType(RenderPolygon.NumberOfVertices);
	for (i = 0; i < RenderPolygon.NumberOfVertices; i++) {
		RENDERVERTEX *vertices = &renderVerticesPtr[i];
		
		GLfloat x, y, z, zvalue;
		GLfloat s, t, rhw;
		int x1, y1;
		
		rhw = 1.0 / vertices->Z;
		
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
		
		s = ((float)(vertices->U>>16)+.5) * RecipW;
		t = ((float)(vertices->V>>16)+.5) * RecipH;

		zvalue = vertices->Z+HeadUpDisplayZOffset;
		z = 1.0 - 2*ZNear/zvalue;
		
//		zvalue = vertices->Z+HeadUpDisplayZOffset;
//		zvalue = ((zvalue-ZNear)/zvalue);

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

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	int flags, i;
	float ZNear;
	
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
	
	flags = inputPolyPtr->PolyFlags;
	
	CheckTranslucencyModeIsCorrect(RenderPolygon.TranslucencyMode);
	CheckBoundTextureIsCorrect(0);
	
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
		
		if (flags & iflag_transparent)
			glColor4ub(vertices->R, vertices->G, vertices->B, vertices->A);
		else
			glColor4ub(vertices->R, vertices->G, vertices->B, 255);
		
		glVertex4f(x/rhw, y/rhw, z/rhw, 1/rhw);
	}
	glEnd();		
}

void D3D_PlayerOnFireOverlay()
{
	int c = 128;
	int colour = (FMVParticleColour&0xffffff)+(c<<24);
	GLfloat x[4], y[4], s[4], t[4];
	float u, v;
	int r, g, b, a;
	D3DTexture *tex;

	b = (colour >> 0)  & 0xFF;
	g = (colour >> 8)  & 0xFF;
	r = (colour >> 16) & 0xFF;
	a = (colour >> 24) & 0xFF;
	
	tex = ImageHeaderArray[BurningImageNumber].D3DTexture;
	
	CheckTranslucencyModeIsCorrect(TRANSLUCENCY_GLOWING);
	//CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	CheckBoundTextureIsCorrect(tex->id);
	
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
	D3DTexture *tex;
	int theta[2];
	int colour, baseColour;
	int r, g, b, a;
	int i;
	
	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;
	
	tex = ImageHeaderArray[SpecialFXImageNumber].D3DTexture;
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
	
	
	// CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	CheckBoundTextureIsCorrect(tex->id);
	
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
	// CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	CheckBoundTextureIsCorrect(tex->id);
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
//	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	CheckBoundTextureIsCorrect(tex->id);

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
	CheckBoundTextureIsCorrect(0);
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
	//CheckFilteringModeIsCorrect(FILTERING_BILINEAR_ON);
	CheckBoundTextureIsCorrect(tex->id);
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
	CheckBoundTextureIsCorrect(0);
	
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
	CheckBoundTextureIsCorrect(tex->id);
	
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
	
//	CheckFilteringModeIsCorrect(FILTERING_BILINEAR_OFF);
	
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
