#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <SDL/SDL_audio.h> // For a few defines (AUDIO_*)

#include "fixer.h"

#include "3dc.h"
#include "platform.h"
#include "psndplat.h"
#include "gamedef.h"
#include "avpview.h"
#include "ffstdio.h"
#include "dynamics.h"
#include "dynblock.h"
#include "stratdef.h"

 /* psndplat.cpp */
ACTIVESOUNDSAMPLE ActiveSounds[SOUND_MAXACTIVE];
ACTIVESOUNDSAMPLE BlankActiveSound = {SID_NOSOUND,ASP_Minimum,0,0,NULL,0,0,0,0,0, { {0,0,0},{0,0,0},0,0 }, 0, 0, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, NULL, NULL, NULL};
SOUNDSAMPLEDATA BlankGameSound = {0,0,0,0,0,NULL,0,0,NULL,0};
SOUNDSAMPLEDATA GameSounds[SID_MAXIMUM];

ALCdevice *AvpSoundDevice;
ALvoid *AvpSoundContext;

int PlatStartSoundSys()
{
	int i;
	ALfloat pos[] = { 0.0, 0.0, 0.0 },
	       vel[] = { 0.0, 0.0, 0.0 },
	       or[] = { 0.0, 0.0, 1.0, 0.0, -1.0, 0.0 };
	int attrlist[] = {
		ALC_FREQUENCY, 22050,
		ALC_SYNC, AL_FALSE,
		0
	};
	       
	fprintf(stderr, "PlatStartSoundSys()\n");
	
	AvpSoundDevice = alcOpenDevice ("'( (sampling-rate 22050 ))");
	AvpSoundContext = alcCreateContext (AvpSoundDevice, attrlist);
	alcMakeContextCurrent (AvpSoundContext);
	
	alListenerfv (AL_POSITION, pos);
	alListenerfv (AL_VELOCITY, vel);
	alListenerfv (AL_ORIENTATION, or);
	
	if (alGetError () != AL_NO_ERROR) {
		perror ("alListenerfv()");
		exit (1);
	}
	
	for (i = 0; i < SOUND_MAXACTIVE; i++) {
		ALuint p;
		
		alGenSources (1, &(ActiveSounds[i].ds3DBufferP));
		p = ActiveSounds[i].ds3DBufferP;

		ActiveSounds[i].PropSetP_pos[0] = 0.0;
		ActiveSounds[i].PropSetP_pos[1] = 0.0;
		ActiveSounds[i].PropSetP_pos[2] = 0.0;
		ActiveSounds[i].PropSetP_vel[0] = 0.0;
		ActiveSounds[i].PropSetP_vel[1] = 0.0;
		ActiveSounds[i].PropSetP_vel[2] = 0.0;

		if (alGetError () != AL_NO_ERROR) {
			perror ("alGenSources ()");
			return -1;
		}
		
		alSourcef (p, AL_PITCH, 1.0f);
		alSourcef (p, AL_GAIN, 1.0f);
		alSourcefv (p, AL_POSITION, ActiveSounds[i].PropSetP_pos);
		alSourcefv (p, AL_VELOCITY, ActiveSounds[i].PropSetP_vel);
	}
	
	return 1;
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
	int si;
	
	fprintf(stderr, "PlatPlaySound(%d)\n", activeIndex);

	if ((activeIndex < 0) || (activeIndex >= SOUND_MAXACTIVE))
		return 0;
	si = ActiveSounds[activeIndex].soundIndex;
	if ((si < 0) || (si >= SID_MAXIMUM))
		return 0;
	if (!GameSounds[si].loaded)
		return 0;

	if (!PlatSoundHasStopped(activeIndex))		
		PlatStopSound (activeIndex);
		
//	if (ActiveSounds[activeIndex].loop)
//		alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_LOOPING, AL_TRUE);
//	else
		alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_LOOPING, AL_FALSE);

	alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_BUFFER,
		   GameSounds[si].dsBufferP);


	if (ActiveSounds[activeIndex].pitch != GameSounds[si].pitch) {
		int ok = PlatChangeSoundPitch (activeIndex, ActiveSounds[activeIndex].pitch);
		if (ok == SOUND_PLATFORMERROR) {
			PlatStopSound (activeIndex);
			return ok;
		}
	}
	
	if (ActiveSounds[activeIndex].threedee) {
		int ok;
		
		/* set distance at which attenuation starts */
	
	
		ok = PlatDo3dSound (activeIndex);
		
		if (ok == SOUND_PLATFORMERROR) {
			PlatStopSound (activeIndex);
			return ok;
		}
	} else {
		int newVolume, ok;
		
		newVolume = ActiveSounds[activeIndex].volume;
		newVolume = (newVolume * VOLUME_PLAT2DSCALE) >> 7;
		ActiveSounds[activeIndex].volume = newVolume;
		
		ok = PlatChangeSoundVolume (activeIndex, ActiveSounds[activeIndex].volume);
		if (ok == SOUND_PLATFORMERROR) {
			PlatStopSound (activeIndex);
			return ok;
		}
	}
	
	
	
	if (!ActiveSounds[activeIndex].paused) {
		alSourcePlay (ActiveSounds[activeIndex].ds3DBufferP);
		
		if (ActiveSounds[activeIndex].loop) {
			fprintf (stderr, "Playing sound %i %s looping in slot %i\n",
				si, GameSounds[si].wavName, activeIndex);
		} else {
			fprintf (stderr, "Playing sound %i %s once in slot %i\n",
				si, GameSounds[si].wavName, activeIndex);
		}
	}
	
	return 1;
}

void PlatStopSound(int activeIndex)
{
	fprintf(stderr, "PlatStopSound(%d)\n", activeIndex);
	
//	if (ActiveSounds[activeIndex].paused)
//		alSourcePause (ActiveSounds[activeIndex].ds3DBufferP);
//	else
//		alSourceStop (ActiveSounds[activeIndex].ds3DBufferP);
	if (!PlatSoundHasStopped (activeIndex))
		alSourceStop (ActiveSounds[activeIndex].ds3DBufferP);
}

int PlatChangeSoundVolume(int activeIndex, int volume)
{
//	float nv = 127.0f / (float) volume;
	
	fprintf(stderr, "PlatChangeSoundVolume(%d, %d)\n", activeIndex, volume);
	
//	if (nv > 1.0)
//		nv = 1.0;
		
//	alSourcef (ActiveSounds[activeIndex].ds3DBufferP, 
//		   AL_MAX_GAIN, nv);
		   
	return 1;
}

int PlatChangeSoundPitch(int activeIndex, int pitch)
{
	float frequency;
	
	fprintf(stderr, "PlatChangeSoundPitch(%d, %d)\n", activeIndex, pitch);

	if ((pitch < PITCH_MIN) || (pitch >= PITCH_MAX))
		return 0;
	
	if (pitch == PITCH_DEFAULTPLAT)
		frequency = 0;
	else {
//		SOUNDINDEX gsi = ActiveSounds[activeIndex].soundIndex;
//		frequency = ToneToFrequency (GameSounds[gsi].dsFrequency,
//			GameSounds[gameSoundIndex].pitch, pitch);
		frequency = (128.0f / ((float)pitch + 127.0));
	}
	
	ActiveSounds[activeIndex].pitch = pitch;
	
//	alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_PITCH, frequency);	

	return 1;
}

int PlatSoundHasStopped(int activeIndex)
{
	ALint val;
	
	fprintf(stderr, "PlatSoundHasStopped(%d)\n", activeIndex);

	alGetSourceiv (ActiveSounds[activeIndex].ds3DBufferP, 
			AL_SOURCE_STATE, &val);

	if (alGetError () != AL_NO_ERROR) 
		return SOUND_PLATFORMERROR;
		
	if ((val != AL_PLAYING) && (val != AL_PAUSED))
		return 1;

	return 0;
}

int PlatDo3dSound(int activeIndex)
{
	int distance;
	VECTORCH relativePosn;
	int newPan, newVolume;

return;	
	fprintf(stderr, "PlatDo3dSound(%d)\n", activeIndex);

	relativePosn.vx = ActiveSounds[activeIndex].threedeedata.position.vx - 
			Global_VDB_Ptr->VDB_World.vx;
	relativePosn.vy = ActiveSounds[activeIndex].threedeedata.position.vy - 
			Global_VDB_Ptr->VDB_World.vy;
	relativePosn.vz = ActiveSounds[activeIndex].threedeedata.position.vz - 
			Global_VDB_Ptr->VDB_World.vz;

	distance = Magnitude (&relativePosn);
	
	if (ActiveSounds[activeIndex].paused) {
		if (distance < (ActiveSounds[activeIndex].threedeedata.outer_range + SOUND_DEACTIVATERANGE)) {
			PlatStopSound (activeIndex);
			
			if (ActiveSounds[activeIndex].loop)
				alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_LOOPING, AL_TRUE);
			else								
				alSourcei (ActiveSounds[activeIndex].ds3DBufferP, AL_LOOPING, AL_FALSE);
			
			alSourcePlay (ActiveSounds[activeIndex].ds3DBufferP);
			newVolume = 0;
			ActiveSounds[activeIndex].paused = 0;
		} else {
			return 1;
		}
	}
	
	if (distance < ActiveSounds[activeIndex].threedeedata.inner_range) {
		newVolume = ActiveSounds[activeIndex].volume;
	} else {
		if (distance < ActiveSounds[activeIndex].threedeedata.outer_range) {
			float in_to_dis_to_out = ActiveSounds[activeIndex].threedeedata.outer_range - distance;
			float in_to_out = ActiveSounds[activeIndex].threedeedata.outer_range - ActiveSounds[activeIndex].threedeedata.inner_range;
			
			if (in_to_out > 0.0) {
				newVolume = (int)
					((float)ActiveSounds[activeIndex].volume * (in_to_dis_to_out / in_to_out));
			} else {
				newVolume = 0;
			}
		} else {
			newVolume = 0;
			
			if ((distance < (ActiveSounds[activeIndex].threedeedata.outer_range + SOUND_DEACTIVATERANGE)) &&
			   ActiveSounds[activeIndex].loop) {
				PlatStopSound (activeIndex);
				ActiveSounds[activeIndex].paused = 1;
			}
		}
	}
	
	if (newVolume > VOLUME_MAX)
		newVolume = VOLUME_MAX;
	if (newVolume < VOLUME_MIN)
		newVolume = VOLUME_MIN;
	
	if (PlatChangeSoundVolume (activeIndex, newVolume) == SOUND_PLATFORMERROR) {
		return SOUND_PLATFORMERROR;
	}
	
	if (distance < ActiveSounds[activeIndex].threedeedata.outer_range) {
		ActiveSounds[activeIndex].PropSetP_pos[0] =
			relativePosn.vx;
		ActiveSounds[activeIndex].PropSetP_pos[1] =
			relativePosn.vy;
		ActiveSounds[activeIndex].PropSetP_pos[2] =
			relativePosn.vz;
		alSourcefv (ActiveSounds[activeIndex].ds3DBufferP,
			    AL_POSITION, ActiveSounds[activeIndex].PropSetP_pos);

/* The c++ code had this stuff marked out b/c there was no "Doppler" shifting */	
#if 0
		ActiveSounds[activeIndex].PropSetP_vel[0] =
			ActiveSounds[activeIndex].threedeedata.velocity.vx;
		ActiveSounds[activeIndex].PropSetP_vel[1] =
			ActiveSounds[activeIndex].threedeedata.velocity.vy;
		ActiveSounds[activeIndex].PropSetP_vel[2] =
			ActiveSounds[activeIndex].threedeedata.velocity.vz;
		alSourcefv (ActiveSounds[activeIndex].ds3DBufferP,
			    AL_VELOCITY, ActiveSounds[activeIndex].PropSetP_vel);
#endif
	} else {
		int angle;
		Normalise (&relativePosn);
		angle = ArcTan (relativePosn.vx, relativePosn.vz);
		if (angle >= Player->ObEuler.EulerY)
			angle -= Player->ObEuler.EulerY;
		else
			angle += (4096 - Player->ObEuler.EulerY);
		
		// LOCALASSERT ((angle>=0)&&(angle<=4095))
		if (angle > 1024) {
			if (angle < 3072)
				angle = (2048-angle);
			else 
				angle = (angle-4096);
		}
		// LOCALASSERT((angle>=-1024)&&(angle<=1024));
		newPan = (PAN_MAXPLAT * angle) >> 10;
		
		if ((distance < ActiveSounds[activeIndex].threedeedata.inner_range) && (newPan != 0)) {
			newPan = (newPan * distance) / ActiveSounds[activeIndex].threedeedata.inner_range;
		}
		
		if (PlatChangeSoundPan (activeIndex, newPan) == SOUND_PLATFORMERROR) {
			return SOUND_PLATFORMERROR;
		}
	}

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
	ALsizei size, freq, bits;
	ALenum format;
	ALvoid *data;
	
	fprintf(stderr, "LoadWavFile(%d, %s) - sound\n", soundNum, wavFileName);

	alutLoadWAV (wavFileName, &data, &format, &size, &bits, &freq);
	alGenBuffers (1, &(GameSounds[soundNum].dsBufferP));
	alBufferData (GameSounds[soundNum].dsBufferP, format, data, size, freq);

	GameSounds[soundNum].loaded = 1;
	
	free (data);
	return 0;
}

unsigned char *Force8to16 (unsigned char *buf, int *len)
{
	unsigned char *nbuf;
	unsigned int i;
	
	nbuf = (unsigned char *) AllocateMem (*len * 2);
	
	for (i = 0; i < *len; i++) {
		short int x = ((buf[i] << 8) | buf[i]) ^ 0x8000;
		nbuf[i*2+0] = (x & 0x00ff);
		nbuf[i*2+1] = (x >> 8) & 0xff;
	}
	
	*len *= 2;
	return nbuf;
}

// In libopenal
extern void *acLoadWAV (void *data, ALuint *size, void **udata, 
			ALushort *fmt, ALushort *chan, ALushort *freq);
			
unsigned char *ExtractWavFile(int soundIndex, unsigned char *bufferPtr)
{
	ALint len, seclen = 0;
	unsigned char *nb;
	void *udata;
	ALushort rfmt, rchan, rfreq, rsize;
		
	fprintf(stderr, "ExtractWavFile(%d, %p)\n", soundIndex, bufferPtr);

	{
		int slen = strlen (bufferPtr) + 1;
		GameSounds[soundIndex].wavName = (char *)AllocateMem (slen);
		strcpy (GameSounds[soundIndex].wavName, bufferPtr);
		bufferPtr += slen;
fprintf (stderr, "Loaded %s\n", GameSounds[soundIndex].wavName);
	}
	
	if (acLoadWAV (bufferPtr, (ALuint *) &rsize, &udata, &rfmt,
			&rchan, &rfreq) == NULL) {
		fprintf (stderr, "Unable to convert data\n");
		return (unsigned char *)0;
	}
	
	len = rsize;
	
	if ((rfmt == AUDIO_U8)) {
		nb = Force8to16 (udata, &len);
		rfmt = AUDIO_S16LSB;
		
		free (udata);
		udata = nb;
	} 
	
	if ((rfmt == AUDIO_S16LSB) || (rfmt == AUDIO_S16MSB)) {
		if (rchan == 2) {
			rfmt = AL_FORMAT_STEREO16;
			seclen = len / (rfreq * 2 * 2);
		} else if (rchan == 1) {
			rfmt = AL_FORMAT_MONO16;
			seclen = len / (rfreq * 2);
		}
	} else
		return (unsigned char *)0;
	
	alGenBuffers (1, &(GameSounds[soundIndex].dsBufferP));
	alBufferData (GameSounds[soundIndex].dsBufferP,
		      rfmt, udata, len, rfreq);
	
	GameSounds[soundIndex].loaded = 1;
	GameSounds[soundIndex].flags = SAMPLE_IN_HW;
	GameSounds[soundIndex].length = (seclen != 0) ? seclen : 1;
	GameSounds[soundIndex].dsFrequency = rfreq;
	GameSounds[soundIndex].pitch = PITCH_DEFAULTPLAT;
	
	free (udata);

	/* read RIFF chunk length and jump past it */
	return bufferPtr + 8 + 
		((bufferPtr[4] <<  0) | (bufferPtr[5] << 8) |
		 (bufferPtr[6] << 16) | (bufferPtr[7] << 24));
}

int LoadWavFromFastFile(int soundNum, char * wavFileName)
{
	FFILE *fp;
	unsigned char *buf;
	unsigned int len = 0;
	
	fprintf(stderr, "LoadWavFromFastFile(%d, %s)\n", soundNum, wavFileName);

	if ((fp = ffopen (wavFileName, "rb")) != NULL) {
		ffseek (fp, 0, SEEK_END);
		len = fftell (fp);
		ffseek (fp, 0, SEEK_SET);
		buf = (unsigned char *) malloc (len + strlen (wavFileName) + 1);
		strcpy (buf, wavFileName);
		ffread (&buf[strlen(wavFileName)+1], len, 1, fp);
		ffclose (fp);
		len = (int)ExtractWavFile (soundNum, buf);		
		free (buf);
	}

	return len;
}

void PlatUpdatePlayer()
{
	ALfloat vel[3], or[6], pos[3];
	fprintf(stderr, "PlatUpdatePlayer()\n");

return;
	if (Global_VDB_Ptr) {	
		extern int NormalFrameTime;
		extern int DopplerShiftIsOn;
		
		if (AvP.PlayerType != I_Alien) {
			or[0] = (float) ((Global_VDB_Ptr->VDB_Mat.mat13) / 65536.0F);
			or[1] = 0.0;
			or[2] = (float) ((Global_VDB_Ptr->VDB_Mat.mat33) / 65536.0F);
			or[3] = 0.0;
			or[4] = 1.0;
			or[5] = 0.0;
		} else {
			or[0] = (float) ((Global_VDB_Ptr->VDB_Mat.mat13) / 65536.0F);
			or[1] = (float) ((Global_VDB_Ptr->VDB_Mat.mat23) / 65536.0F);
			or[2] = (float) ((Global_VDB_Ptr->VDB_Mat.mat33) / 65536.0F);
			or[3] = (float) ((Global_VDB_Ptr->VDB_Mat.mat12) / 65536.0F);
			or[4] = (float) ((Global_VDB_Ptr->VDB_Mat.mat22) / 65536.0F);
			or[5] = (float) ((Global_VDB_Ptr->VDB_Mat.mat32) / 65536.0F);
		}

		if (AvP.PlayerType == I_Alien && DopplerShiftIsOn && NormalFrameTime) {
			DYNAMICSBLOCK *dynPtr = Player->ObStrategyBlock->DynPtr;
			float invFrameTime = 100000.0f/(float)NormalFrameTime;
			
			vel[0] = (float)(dynPtr->Position.vx - dynPtr->PrevPosition.vx) * invFrameTime;
			vel[1] = (float)(dynPtr->Position.vy - dynPtr->PrevPosition.vy) * invFrameTime;
			vel[2] = (float)(dynPtr->Position.vz - dynPtr->PrevPosition.vz) * invFrameTime;
		} else {
			vel[0] = 0.0;
			vel[1] = 0.0;
			vel[2] = 0.0;
		}
		
/* again, no doppler crap. */
#if 0
		{
			pos[0] = Global_VDB_Ptr->VDB_World.vx;
			pos[1] = Global_VDB_Ptr->VDB_World.vy;
			pos[2] = Global_VDB_Ptr->VDB_World.vz;
		}
#else
		{
			pos[0] = pos[1] = pos[2] = 0.0;
		}
#endif
	}
	
	
	// fixme: add reverb check
	alListenerfv (AL_ORIENTATION, or);
	alListenerfv (AL_VELOCITY, vel);
	alListenerfv (AL_POSITION, pos);
}

void PlatSetEnviroment(unsigned int env_index, float reverb_mix)
{
	fprintf(stderr, "PlatSetEnvironment(%d, %f)\n", env_index, reverb_mix);
}

void UpdateSoundFrequencies()
{
	fprintf(stderr, "UpdateSoundFreqncies()\n");
}

int PlatChangeSoundPan (int activeIndex, int pan)
{
	return 1;
}
