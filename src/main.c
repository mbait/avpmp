#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "cdtrackselection.h"

PROCESSORTYPES ReadProcessorType()
{
	return PType_PentiumMMX;
}

int InitialiseWindowsSystem()
{
	return 0;
}

int ExitWindowsSystem()
{
	return 0;
}

int main(int argc, char *argv[])
{
	LoadCDTrackList();
	
	return 0;
}
