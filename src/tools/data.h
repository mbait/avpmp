#ifndef __DATA_H__
#define __DATA_H__

#define PACKED __attribute__((packed))

typedef struct pcx_header_type
{
        char manufacturer;
        char version;
        char encoding;
        char bits_per_pixel;
        short int  x, y;
        short int  width, height;
        short int  horz_res;
        short int  virt_res;
        char ega_palette[48];
        char reserved;
        char num_color_planes;
        short int  byte_per_line;
        short int  palette_type;
        short int hscreen_size;
        short int vscreen_size;
        char padding[54];
} PACKED pcx_header, *pcx_header_ptr;

unsigned char *Pal_3_4(unsigned char *dat);

unsigned char *Pixel_BGR_RGB(unsigned char *data, int length);
unsigned char *Pixel_BGRA_RGBA(unsigned char *data, int length);
unsigned char *Pixel_256_RGB(unsigned char *data, unsigned char *pal, int length);
unsigned char *Pixel_256_RGBA(unsigned char *data, unsigned char *pal, int length, int trans);

unsigned char *LoadTGA(char *filename, int *width, int *height);

unsigned char *LoadPCX(char *filename, unsigned char **pal, int *width, int *height);
unsigned char *LoadPCX_RGB(char *filename, int *width, int *height);
unsigned char *LoadPCX_RGBA(char *filename, int *width, int *height);
void SavePCX(unsigned char *buf, int width, int height, unsigned char *pal, char *name);

void SavePCX256ToFile(unsigned char *buf, int width, int height, unsigned char *pal, char *name);
void SavePCXRGBToFile(unsigned char *buf, int width, int height, char *name);
/*

todo: sgi graphic format, microsoft bmp format, rest of tga/pcx, ilbm
gif? 
jpg?
png? 

function call asks for certain graphic format and data is converted if needed

of course asking for an indexed type from a truecolor type will generate an
error

EnumerateLoadImageTypes
EnumerateSaveImageTypes
EnumerateLoadImageTypeCaps
EnumerateSaveImageTypeCaps
DetermineImageType
*/

#endif
