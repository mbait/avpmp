#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "r2base.h"
#include "indexfnt.hpp"

/* d3d_render.cpp */
void r2rect::AlphaFill(unsigned char R, unsigned char G, unsigned char B, unsigned char translucency) const
{
	fprintf(stderr, "r2rect::AlphaFill(%d, %d, %d, %d)\n", R, G, B, translucency);
}

void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour)
{
	fprintf(stderr, "D3D_RenderHUDNumber_Centred(%d, %d, %d, %d)\n", number, x, y, colour);
}

void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour)
{
	fprintf(stderr, "D3D_RenderHUDString_Centred(%s, %d, %d, %d)\n", stringPtr, centreX, y, colour);
}


/* indexfnt.cpp */
#if 1
IndexedFont* IndexedFont::GetFont(FontIndex I_Font_ToGet)
{
	fprintf(stderr, "IndexedFont::GetFont(%d)\n", I_Font_ToGet);
//	return pIndexedFont[ I_Font_ToGet ];

	return NULL;
}
#endif

void IndexedFont::UnloadFont(FontIndex I_Font_ToGet)
{
	fprintf(stderr, "IndexedFont::UnloadFont(%d)\n", I_Font_ToGet);
}

OurBool IndexedFont::bCanRenderFully(ProjChar* pProjCh)
{
	fprintf(stderr, "IndexedFont::bCanRenderFully(%p)\n", pProjCh);
	
	return 0;
}

void IndexedFont_Proportional_PF::PFUnLoadHook(FontIndex I_Font_ToGet)
{
	fprintf(stderr, "IndexedFont_Proportional_PF::PFUnLoadHook(%d)\n", I_Font_ToGet);
}

IndexedFont::IndexedFont(FontIndex I_Font_New)
{
	fprintf(stderr, "IndexedFont::IndexedFont(%d)\n", I_Font_New);
}

IndexedFont::~IndexedFont()
{
	fprintf(stderr, "IndexedFont::~IndexedFont()\n");
}

IndexedFont_HUD::IndexedFont_HUD(FontIndex I_Font_New) : IndexedFont(I_Font_New)
{
	fprintf(stderr, "IndexedFont_HUD::IndexedFont_HUD(%d)\n", I_Font_New);
}

void IndexedFont_HUD :: RenderString_Clipped
(
	struct r2pos& R2Pos_Cursor,
	const struct r2rect& R2Rect_Clip,
	int FixP_Alpha,// FixP_Alpha,
        const SCString& SCStr
) const
{
	fprintf(stderr, "IndexedFont_HUD :: RenderString_Clipped(stuff, stuff, %d, stuff)\n", FixP_Alpha);
}

void IndexedFont_HUD :: RenderString_Unclipped
(
        struct r2pos& R2Pos_Cursor,
        int FixP_Alpha, //  FixP_Alpha,
        const SCString& SCStr
) const
{
	fprintf(stderr, "IndexedFont_HUD :: RenderString_Unclipped: shouldn't be called!\n");
	exit(EXIT_FAILURE);
}

r2size IndexedFont_HUD :: CalcSize
(
        ProjChar* pProjCh
) const
{
        r2size R2Size_Return
        (
                0,
                GetHeight()
        );

	fprintf(stderr, "IndexedFont_HUD :: CalcSize(%p)\n", pProjCh);

	return R2Size_Return;
}

void IndexedFont_HUD :: RenderChar_Clipped
(
        struct r2pos& R2Pos_Cursor,
        const struct r2rect& R2Rect_Clip,
        int, // FixP_Alpha,
        ProjChar ProjCh
) const
{
	fprintf(stderr, "IndexedFont_HUD :: RenderChar_Clipped(stuff, stuff, stuff, stuff)\n");
}

void IndexedFont_HUD :: RenderChar_Unclipped
(
        struct r2pos& R2Pos_Cursor,
        int, // FixP_Alpha,
        ProjChar ProjCh
) const
{
	fprintf(stderr, "IndexedFont_HUD :: RenderChar_Unclipped(stuff, stuff, stuff)\n");
}
