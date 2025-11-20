#include "sound_core.h"
#include "os_core.h"

#define __HEADER
#include "../common.h"
#undef __HEADER



#ifdef NOBASS
namespace Sound_Core
{

	void				Startup() {}
	void				Shutdown() {}
	//
	// Set global volume
	//
	void				SetGlobalMusicVolume(float theVolume) {}
	void				SetGlobalSoundVolume(float theVolume) {}

	//
	// Pauses sound (put in for multitasking)
	//
	void				Pause(bool theState) {}
	void				Multitasking(bool isForeground) {}

	//
	// Load/Play Sounds
	//
	unsigned int		LoadSound(char *theFilename, int theDuplicates) {return 0;}
	unsigned int		GetSoundBuffer(unsigned int theSoundID) {return 0;}
	void				PlaySoundBuffer(unsigned int theBufferID) {}
	void				StopSoundBuffer(unsigned int theBufferID) {}
	void				SetSoundBufferVolume(unsigned int theBufferID, float theVolume) {}
	void				SetSoundBufferFrequency(unsigned int theBufferID, float theFrequency) {}
	void				SetSoundBufferLooping(unsigned int theBufferID, bool theState) {}
	float				GetSoundBufferFrequency(unsigned int theBufferID) {return 0;}
	bool				IsSoundBufferPlaying(unsigned int theBufferID) {return false;}
	void				UnloadSound(unsigned int theSoundID) {}

	//
	// Streams
	//
	unsigned int		LoadStreamSound(char *theFilename) {return 0;}
	void				UnloadStreamSound(unsigned int theSoundID) {}
	void				GetStreamLevel(unsigned int theSoundID, float* theLeft, float* theRight) {*theLeft=0;*theRight=0;}


	//
	// Load/Play Music
	//
	unsigned int		LoadMusic(char *theFilename) {return 0;}
	void				UnloadMusic(unsigned int theHandle) {}
	void				PlayMusic(unsigned int theHandle, unsigned int theOffset) {}
	void				StopMusic(unsigned int theHandle) {}
	void				SetMusicVolume(unsigned int theHandle, float theVolume) {}
	void				SetMusicTrackVolume(unsigned int theHandle, unsigned int theTrack, float theVolume) {}

	void				AllowSoundsInBackground(bool theState) {}

	void				SetSoundPosition(unsigned int theHandle, float thePos) {}
	float				GetSoundPosition(unsigned int theHandle) {return 0;}

	void				GetSampleRate(unsigned int& theMin, unsigned int& theMax) {theMin=theMax=0;}
	unsigned int		CreateDynamicSound(void* theDataPacket, int theBufferLength) {return 0;}
	unsigned int		GetFrequency() {return 0;}
	
}


#else

#include "BASS/bass.h"

namespace Sound_Core
{

bool gBASS_Started=false;
bool gAllowSoundsInBackground=false;

void Startup()
{
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) 
	{
		OS_Core::Printf("!An incorrect version of BASS was loaded");
		return;
	}
	//HWND aWindow=*(HWND*)OS_Core::Query("HWND");
//	if (!BASS_Init(-1,44100,0,aWindow,NULL)) if (!BASS_Init(0,44100,0,aWindow,NULL))
	if (!BASS_Init(-1,44100,0,NULL,NULL)) if (!BASS_Init(0,44100,0,NULL,NULL))
	{
		OS_Core::Printf("!Sound_Core:: BASS_Init failed");
		return;
	}
	BASS_Start();
	gBASS_Started=true;
}

void Shutdown()
{
	gBASS_Started=false;
	BASS_Free();
}

void SetGlobalMusicVolume(float theVolume)
{
	if (!gBASS_Started) return;
	BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC,(DWORD)(theVolume*10000));
}

void SetGlobalSoundVolume(float theVolume)
{
	if (!gBASS_Started) return;
	BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE,(DWORD)(theVolume*10000));
	BASS_SetConfig(BASS_CONFIG_GVOL_STREAM,(DWORD)(theVolume*10000));
}

void Multitasking(bool isForeground)
{
	if (!gBASS_Started) return;
	if (gAllowSoundsInBackground) return;

	if (isForeground) BASS_Start();
	else BASS_Pause();
}

void AllowSoundsInBackground(bool theState)
{
	gAllowSoundsInBackground=theState;
}



void Pause(bool theState)
{
	if (!gBASS_Started) return;
	if (!theState) BASS_Start();
	else BASS_Pause();
}

unsigned int LoadSound(char *theFilename, int theDuplicates)
{
	if (!gBASS_Started) return 0;
	/*
	unsigned int aHandle=BASS_SampleLoad(false,theFilename,0,0,theDuplicates,NULL);
	OS_Core::Printf("Bass error: %d [%s]",BASS_ErrorGetCode(),theFilename);
	return aHandle;
	*/
	_FixPath(theFilename);
	return BASS_SampleLoad(false,theFilename,0,0,theDuplicates,NULL);
}

unsigned int LoadStreamSound(char *theFilename)
{
	if (!gBASS_Started) return 0;
	_FixPath(theFilename);
	return BASS_StreamCreateFile(false,theFilename,0,0,0);
}

unsigned int GetSoundBuffer(unsigned int theSoundID)
{
	if (!gBASS_Started) return 0;
	return BASS_SampleGetChannel(theSoundID,true);
}

void PlaySoundBuffer(unsigned int theBufferID)
{
	if (!gBASS_Started) return;
	if (!BASS_ChannelPlay(theBufferID,true)) 
	{
#ifdef _DEBUG
		int aCode=BASS_ErrorGetCode();
		switch (aCode)
		{
		case 9:break; // App isn't focused (BASS_ERROR_START)
		default:OS_Core::Printf("! Play Bass error: %d (Playing %d)",aCode,theBufferID);break;
		}
#endif
	}
}

void StopSoundBuffer(unsigned int theBufferID)
{
	if (!gBASS_Started) return;
	BASS_ChannelPause(theBufferID);
}

void SetSoundBufferVolume(unsigned int theBufferID, float theVolume)
{
	if (!gBASS_Started) return;
	BASS_ChannelSetAttribute(theBufferID,BASS_ATTRIB_VOL,theVolume);
}

void SetSoundBufferFrequency(unsigned int theBufferID, float theFrequency)
{
	if (!gBASS_Started) return;
	BASS_ChannelSetAttribute(theBufferID,BASS_ATTRIB_FREQ,theFrequency);
}

float GetSoundBufferFrequency(unsigned int theBufferID)
{
	if (!gBASS_Started) return 0;
	BASS_CHANNELINFO aInfo;
	BASS_ChannelGetInfo(theBufferID,&aInfo);
	return (float)aInfo.freq;
}

bool IsSoundBufferPlaying(unsigned int theBufferID)
{
	if (!gBASS_Started) return false;
	return (BASS_ChannelIsActive(theBufferID)==BASS_ACTIVE_PLAYING);
}

void UnloadSound(unsigned int theSoundID)
{
	if (!gBASS_Started) return;
	BASS_SampleFree(theSoundID);
}

void UnloadStreamSound(unsigned int theSoundID)
{
	if (!gBASS_Started) return;
	BASS_StreamFree(theSoundID);
}

void GetStreamLevel(unsigned int theSoundID, float* theLeft, float* theRight)
{
	if (!gBASS_Started) {*theLeft=0;*theRight=0;return;}

	DWORD aLevel;
	aLevel=BASS_ChannelGetLevel(theSoundID);
	*theLeft=(float)LOWORD(aLevel)/32768.0f;
	*theRight=(float)HIWORD(aLevel)/32768.0f;
}




void SetSoundBufferLooping(unsigned int theBufferID, bool theState)
{
	if (!gBASS_Started) return;
	DWORD aFlag=0;
	if (theState) aFlag=BASS_SAMPLE_LOOP;
	BASS_ChannelFlags(theBufferID,aFlag,BASS_SAMPLE_LOOP);
}

unsigned int LoadMusic(char *theFilename)
{
	if (!gBASS_Started) return 0;
	_FixPath(theFilename);
	return BASS_MusicLoad(FALSE,theFilename,0,0,BASS_MUSIC_LOOP,0);
}

void UnloadMusic(unsigned int theHandle)
{
	if (!gBASS_Started) return;
	BASS_MusicFree(theHandle);
}


void PlayMusic(unsigned int theHandle, unsigned int theOffset)
{
	if (!gBASS_Started) return;
	BASS_CHANNELINFO aInfo;
	BASS_ChannelGetInfo(theHandle,&aInfo);
	BASS_ChannelFlags(theHandle,BASS_SAMPLE_LOOP,BASS_SAMPLE_LOOP);
	QWORD aPos=MAKELONG(theOffset,0);
	BASS_ChannelSetPosition(theHandle,aPos,BASS_POS_MUSIC_ORDER);
	BASS_ChannelPlay(theHandle,false);
}

void StopMusic(unsigned int theHandle)
{
	if (!gBASS_Started) return;
	BASS_ChannelStop(theHandle);
}

void SetMusicVolume(unsigned int theHandle, float theVolume)
{
	if (!gBASS_Started) return;
	BASS_ChannelSetAttribute(theHandle,BASS_ATTRIB_VOL,theVolume);
}

void SetMusicTrackVolume(unsigned int theHandle, unsigned int theTrack, float theVolume)
{
	if (!gBASS_Started) return;
	BASS_ChannelSetAttribute(theHandle,BASS_ATTRIB_MUSIC_VOL_CHAN+theTrack,theVolume);
}

void SetSoundPosition(unsigned int theHandle, float thePos)
{
	QWORD aPos=BASS_ChannelSeconds2Bytes(theHandle,thePos);
	BASS_ChannelSetPosition(theHandle,aPos,BASS_POS_BYTE);
}

float GetSoundPosition(unsigned int theHandle)
{
	QWORD aPos=BASS_ChannelGetPosition(theHandle,BASS_POS_BYTE);
	return (float)BASS_ChannelBytes2Seconds(theHandle,aPos);
}

void GetSampleRate(unsigned int& theMin, unsigned int& theMax)
{
	BASS_INFO aInfo;
	BASS_GetInfo(&aInfo);

	theMin=aInfo.minrate;
	theMax=aInfo.maxrate;
}

unsigned int GetFrequency()
{
	BASS_INFO aInfo;
	BASS_GetInfo(&aInfo);
	return aInfo.freq;
}


struct DynamicSoundData
{
	void (*mCallback)(short *theBuffer, unsigned int theLength, void* theExtraData);
	void *mExtraData;

};

DWORD CALLBACK WriteStream(unsigned int theHandle, short* theBuffer, DWORD theLength, void *theUserPtr)
{
	DynamicSoundData* aDD=(DynamicSoundData*)theUserPtr;
	aDD->mCallback((short*)theBuffer,theLength,aDD->mExtraData);
	return theLength;
}


unsigned int CreateDynamicSound(void* theDataPacket, int theBufferLength)
{
	if (!gBASS_Started) return 0;

	DynamicSoundData* aDD=(DynamicSoundData*)theDataPacket;

	BASS_INFO aInfo;
	BASS_GetInfo(&aInfo);
	if (!aInfo.freq) aInfo.freq=44100;

	BASS_SetConfig(BASS_CONFIG_BUFFER,theBufferLength+aInfo.minbuf+1);
//	unsigned int aStream=BASS_StreamCreate(aInfo.freq,2,BASS_SAMPLE_FLOAT,(STREAMPROC*)WriteStream,theDataPacket);
	unsigned int aStream=BASS_StreamCreate(aInfo.freq,2,0,(STREAMPROC*)WriteStream,theDataPacket);
	return aStream;
}


} // Namespace end
#endif



