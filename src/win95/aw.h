#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct DIRECTDRAWSURFACE
{
	int id;
	
	int w;
	int h;
	
	int filter;
} DIRECTDRAWSURFACE;

typedef DIRECTDRAWSURFACE * LPDIRECTDRAWSURFACE;
typedef DIRECTDRAWSURFACE DDSurface;

typedef struct DIRECT3DTEXTURE
{
	int id;
	
	int w;
	int h;
	
	int filter;
} DIRECT3DTEXTURE;

typedef DIRECT3DTEXTURE * LPDIRECT3DTEXTURE;
typedef DIRECT3DTEXTURE D3DTexture;

typedef int D3DTEXTUREHANDLE;

#endif /* _INCLUDED_AW_H_ */
