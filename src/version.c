#include "version.h"

extern void NewOnScreenMessage(unsigned char *messagePtr);

void GiveVersionDetails(void)
{
	NewOnScreenMessage("Aliens vs Predator - Linux\n   Build 000 \n  Based on Rebellion Developments AvP Gold source  \n");
}
