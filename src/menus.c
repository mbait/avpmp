#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "strtab.hpp"

#include "awtexld.h"
#include "chnktexi.h"
#include "hud_layout.h"

#include "avp_menus.h"

#define UseLocalAssert Yes
#include "ourasert.h"
#include "ffstdio.h"

#include <GL/gl.h>

/* char AAFontWidths[256]; */

extern int AAFontImageNumber;

int PlayMenuBackgroundBink()
{
	fprintf(stderr, "PlayMenuBackgroundBink()\n");

	glClear(GL_COLOR_BUFFER_BIT);
	
	return 0;
}
                
AVPMENUGFX AvPMenuGfxStorage[MAX_NO_OF_AVPMENUGFXS] =
{
	{"Menus\\fractal.rim"},
	{"Common\\aa_font.rim"},// Warning! Texture from common used

	{"Menus\\copyright.rim"},

	{"Menus\\FIandRD.rim"},
	{"Menus\\presents.rim"},
	{"Menus\\AliensVPredator.rim"},
	
	{"Menus\\sliderbar.rim"},//AVPMENUGFX_SLIDERBAR,
	{"Menus\\slider.rim"},//AVPMENUGFX_SLIDER,

	{"Menus\\starfield.rim"},
	{"Menus\\aliens.rim"},
	{"Menus\\Alien.rim"},
	{"Menus\\Marine.rim"},
	{"Menus\\Predator.rim"},

	{"Menus\\glowy_left.rim"},
	{"Menus\\glowy_middle.rim"},
	{"Menus\\glowy_right.rim"},
	
	// Marine level
	{"Menus\\MarineEpisode1.rim"},
	{"Menus\\MarineEpisode2.rim"},
	{"Menus\\MarineEpisode3.rim"},
	{"Menus\\MarineEpisode4.rim"},
	{"Menus\\MarineEpisode5.rim"},
	{"Menus\\MarineEpisode6.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	
	// Predator level
	{"Menus\\PredatorEpisode1.rim"},
	{"Menus\\PredatorEpisode2.rim"},
	{"Menus\\PredatorEpisode3.rim"},
	{"Menus\\PredatorEpisode4.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Alien level
	{"Menus\\AlienEpisode2.rim"},
	{"Menus\\AlienEpisode4.rim"},
	{"Menus\\AlienEpisode1.rim"},
	{"Menus\\AlienEpisode3.rim"},
	{"Menus\\AlienEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Splash screens
	#if MARINE_DEMO
	{"MarineSplash\\splash00.rim"},
	{"MarineSplash\\splash01.rim"},
	{"MarineSplash\\splash02.rim"},
	{"MarineSplash\\splash03.rim"},
	{"MarineSplash\\splash04.rim"},
	{"MarineSplash\\splash05.rim"},
	#elif ALIEN_DEMO
	{"AlienSplash\\splash00.rim"},
	{"AlienSplash\\splash01.rim"},
	{"AlienSplash\\splash02.rim"},
	{"AlienSplash\\splash03.rim"},
	{"AlienSplash\\splash04.rim"},
	{"AlienSplash\\splash05.rim"},
	#else
	{"PredatorSplash\\splash00.rim"},
	{"PredatorSplash\\splash01.rim"},
	{"PredatorSplash\\splash02.rim"},
	{"PredatorSplash\\splash03.rim"},
	{"PredatorSplash\\splash04.rim"},
	{"PredatorSplash\\splash05.rim"},
	#endif
};

int LengthOfMenuText(char *textPtr)
{
	fprintf(stderr, "LengthOfMenuText(%s)\n", textPtr);
	
	return 0;
}

int RenderMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{

	return Hardware_RenderSmallMenuText(textPtr, x, y, alpha, format);
	
	fprintf(stderr, "RenderMenuText(%s, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format);

	return 0;
}

int RenderMenuText_Clipped(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{	
	fprintf(stderr, "RenderMenuText_Clipped(%s, %d, %d, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format, topY, bottomY);

	return 0;
}

int RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format)
{

	return Hardware_RenderSmallMenuText(textPtr, x, y, alpha, format);

	fprintf(stderr, "RenderSmallMenuText(%s, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format);

	return 0;
}

int RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{

	return Hardware_RenderSmallMenuText_Coloured(textPtr, x, y, alpha, format, red, green, blue);

	fprintf(stderr, "RenderSmallMenuText_Coloured(%s, %d, %d, %d, %d, %d, %d, %d)\n", textPtr, x, y, alpha, format, red, green, blue);
	
	return 0;	
}

void RenderKeyConfigRectangle(int alpha)
{

	void Hardware_RenderKeyConfigRectangle(int alpha);
	Hardware_RenderKeyConfigRectangle(alpha);
	
	fprintf(stderr, "RenderKeyConfigRectangle(%d)\n", alpha);	
}

void RenderHighlightRectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{

	void Hardware_RenderHighlightRectangle(int x1,int y1,int x2,int y2,int r, int g, int b);
	Hardware_RenderHighlightRectangle(x1, y1, x2, y2, r, g, b);

	fprintf(stderr, "RenderHighlightRectangle(%d, %d, %d, %d, %d, %d, %d)\n", x1, y1, x2, y2, r, g, b);
}

void RenderSmallFontString_Wrapped(char *textPtr,RECT* area,int alpha,int* output_x,int* output_y)
{
	fprintf(stderr, "RenderSmallFontString_Wrapped(%s, %p, %d, %p, %p)\n", textPtr, area, alpha, output_x, output_y);
}

void LoadAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	AVPMENUGFX *gfxPtr;
	char buffer[100];
	unsigned int fastFileLength;
	void const *pFastFileData;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
		
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	
	/* TODO: make sure this doesn't cause a leak */
	InitialiseTextures();
	CL_GetImageFileName(buffer, 100, gfxPtr->FilenamePtr, LIO_RELATIVEPATH);

	pFastFileData = ffreadbuf(buffer, &fastFileLength);
	
	if (pFastFileData) {
		gfxPtr->ImagePtr = AwCreateSurface(
			"pxfXY",
			pFastFileData,
			fastFileLength,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	} else {
		gfxPtr->ImagePtr = AwCreateSurface(
			"sfXY",
			buffer,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	}
	
	GLOBALASSERT(gfxPtr->ImagePtr);
	GLOBALASSERT(gfxPtr->Width>0);
	GLOBALASSERT(gfxPtr->Height>0);
	
	gfxPtr->hBackup = 0;
}

static void ReleaseAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	AVPMENUGFX *gfxPtr;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
	
	
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	
	GLOBALASSERT(gfxPtr);
	GLOBALASSERT(gfxPtr->ImagePtr);
	
	ReleaseDDSurface(gfxPtr->ImagePtr);
	
	gfxPtr->ImagePtr = NULL;
}

void LoadAllAvPMenuGfx()
{
	int i;
	
	for (i = 0; i < AVPMENUGFX_WINNER_SCREEN; i++) {
		LoadAvPMenuGfx(i);
	}
	
	AAFontImageNumber = CL_LoadImageOnce("Common\\aa_font.RIM",LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	
	/*
	TODO: load Fonts
	TODO: Create cloud texture
	TODO: Calculate AA font table?
	*/
	
	fprintf(stderr, "LoadAllAvPMenuGfx()\n");
}
	
void LoadAllSplashScreenGfx()
{
	int i;
	
	for (i = AVPMENUGFX_SPLASH_SCREEN1; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		LoadAvPMenuGfx(i);
	}
}

void InitialiseMenuGfx()
{
	int i;
	
	for (i = 0; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		AvPMenuGfxStorage[i].ImagePtr = NULL;
	}
	
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	
	glClear(GL_COLOR_BUFFER_BIT);
}

void ReleaseAllAvPMenuGfx()
{
	int i;
	
	for (i = 0; i < MAX_NO_OF_AVPMENUGFXS; i++) {
		if (AvPMenuGfxStorage[i].ImagePtr) {
			ReleaseAvPMenuGfx(i);
		}
	}
	
	/*
	TODO: Unload fonts
	*/
	fprintf(stderr, "ReleaseAllAvPMenuGfx()\n");
}

void DrawAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "DrawAvPMenuGfx(%d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format);
}

void DrawAvPMenuGfx_CrossFade(enum AVPMENUGFX_ID menuGfxID,enum AVPMENUGFX_ID menuGfxID2,int alpha)
{
	fprintf(stderr, "DrawAvPMenuGfx_CrossFade(%d, %d, %d)\n", menuGfxID, menuGfxID2, alpha);
}

void DrawAvPMenuGfx_Faded(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	fprintf(stderr, "DrawAvPMenuGfx_Faded(%d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format);
}

void DrawAvPMenuGfx_Clipped(enum AVPMENUGFX_ID menuGfxID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{
	fprintf(stderr, "DrawAvPMenuGfx_Clipped(%d, %d, %d, %d, %d, %d, %d)\n", menuGfxID, topleftX, topleftY, alpha, format, topY, bottomY);
}

int HeightOfMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
	return AvPMenuGfxStorage[menuGfxID].Height;
}

void FadedScreen(int alpha)
{
	fprintf(stderr, "FadedScreen(%d)\n", alpha);
}

void ClearScreenToBlack()
{
	fprintf(stderr, "ClearScreenToBlack()\n");
}

#include "aafont.h"
