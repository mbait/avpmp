#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct DIRECTDRAWSURFACE
{
	int id;
	
	int w;
	int h;
	unsigned char *data;
} DIRECTDRAWSURFACE;

typedef DIRECTDRAWSURFACE * LPDIRECTDRAWSURFACE;
typedef DIRECTDRAWSURFACE DDSurface;

typedef struct DIRECT3DTEXTURE
{
	int id;
	
	int w;
	int h;
	unsigned char *data;
} DIRECT3DTEXTURE;

typedef DIRECT3DTEXTURE * LPDIRECT3DTEXTURE;
typedef DIRECT3DTEXTURE D3DTexture;

//typedef struct D3DTEXTUREHANDLE
//{
//	int x;
//} D3DTEXTUREHANDLE;

typedef int D3DTEXTUREHANDLE;

#endif /* _INCLUDED_AW_H_ */
