#ifndef AVPVIEW_H
#define AVPVIEW_H

/* KJL 10:49:41 04/21/97 - avpview.h */
void AvpShowViews(void);
void InitCameraValues(void);
void LightSourcesInRangeOfObject(DISPLAYBLOCK *dptr);


extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;

#endif
