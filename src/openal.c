#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "psndplat.h"

 /* psndplat.cpp */
ACTIVESOUNDSAMPLE ActiveSounds[SOUND_MAXACTIVE];
ACTIVESOUNDSAMPLE BlankActiveSound = {SID_NOSOUND,ASP_Minimum,0,0,NULL,0,0,0,0,0, { {0,0,0},{0,0,0},0,0 }, NULL, NULL, NULL};
SOUNDSAMPLEDATA BlankGameSound = {0,0,0,0,NULL,0,0,NULL,0};
SOUNDSAMPLEDATA GameSounds[SID_MAXIMUM];

int PlatStartSoundSys()
{
	fprintf(stderr, "PlatStartSoundSys()\n");
	
	return 0;
}

void PlatEndSoundSys()
{
	fprintf(stderr, "PlatEndSoundSys()\n");
}

int PlatChangeGlobalVolume(int volume)
{
	fprintf(stderr, "PlatChangeGlobalVolume(%d)\n", volume);
	
	return 1;
}

int PlatPlaySound(int activeIndex)
{
	fprintf(stderr, "PlatPlaySound(%d)\n", activeIndex);
		
	return 1;
}

void PlatStopSound(int activeIndex)
{
	fprintf(stderr, "PlatStopSound(%d)\n", activeIndex);
}

int PlatChangeSoundVolume(int activeIndex, int volume)
{
	fprintf(stderr, "PlatChangeSoundVolume(%d, %d)\n", activeIndex, volume);
	
	return 1;
}

int PlatChangeSoundPitch(int activeIndex, int pitch)
{
	fprintf(stderr, "PlatChangeSoundPitch(%d, %d)\n", activeIndex, pitch);
	
	return 1;
}

int PlatSoundHasStopped(int activeIndex)
{
	fprintf(stderr, "PlatSoundHasStopped(%d)\n", activeIndex);
	
	return 1;
}

int PlatDo3dSound(int activeIndex)
{
	fprintf(stderr, "PlatDo3dSound(%d)\n", activeIndex);
	
	return 1;
}

void PlatEndGameSound(SOUNDINDEX index)
{
	fprintf(stderr, "PlatEndGameSound(%d)\n", index);
}

unsigned int PlatMaxHWSounds()
{
	fprintf(stderr, "PlatMaxHWSounds()\n");
	
	return 256;
}

void InitialiseBaseFrequency(SOUNDINDEX soundNum)
{
	fprintf(stderr, "InitialiseBaseFrequency(%d)\n", soundNum);
}

int LoadWavFile(int soundNum, char * wavFileName)
{
	fprintf(stderr, "LoadWavFile(%d, %s)\n", soundNum, wavFileName);
	
	return 0;
}

unsigned char *ExtractWavFile(int soundIndex, unsigned char *bufferPtr)
{
	fprintf(stderr, "ExtractWavFile(%d, %p)\n", soundIndex, bufferPtr);
	
	return 0;
}

int LoadWavFromFastFile(int soundNum, char * wavFileName)
{
	fprintf(stderr, "LoadWavFromFastFile(%d, %s)\n", soundNum, wavFileName);
	
	return 0;
}

void PlatUpdatePlayer()
{
	fprintf(stderr, "PlatUpdatePlayer()\n");
}

void PlatSetEnviroment(unsigned int env_index, float reverb_mix)
{
	fprintf(stderr, "PlatSetEnvironment(%d, %f)\n", env_index, reverb_mix);
}

void UpdateSoundFrequencies()
{
	fprintf(stderr, "UpdateSoundFreqncies()\n");
}
