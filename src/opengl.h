#ifndef AVP_OPENGL_H
#define AVP_OPENGL_H

#include "kshape.h"

void InitOpenGL();
void ThisFramesRenderingHasBegun();
void ThisFramesRenderingHasFinished();
void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr);
void D3D_DrawBackdrop();

#endif
