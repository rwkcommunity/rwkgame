#pragma once
#include "os_headers.h"

namespace Sound_Core
{
	//
	// Startup/Shutdown sound system
	//
	void				Startup();
	void				Shutdown();

	//
	// Set global volume
	//
	void				SetGlobalMusicVolume(float theVolume);
	void				SetGlobalSoundVolume(float theVolume);

	//
	// Pauses sound (put in for multitasking)
	//
	void				Pause(bool theState);
	void				Multitasking(bool isForeground);

	//
	// Load/Play Sounds
	//
	unsigned int		LoadSound(char *theFilename, int theDuplicates);
	unsigned int		GetSoundBuffer(unsigned int theSoundID);
	void				PlaySoundBuffer(unsigned int theBufferID);
	void				StopSoundBuffer(unsigned int theBufferID);
	void				SetSoundBufferVolume(unsigned int theBufferID, float theVolume);
	void				SetSoundBufferFrequency(unsigned int theBufferID, float theFrequency);
	void				SetSoundBufferLooping(unsigned int theBufferID, bool theState);
	float				GetSoundBufferFrequency(unsigned int theBufferID);
	bool				IsSoundBufferPlaying(unsigned int theBufferID);
	void				UnloadSound(unsigned int theSoundID);
	//
	// Load/Unload Streams
	//
	unsigned int		LoadStreamSound(char *theFilename);
	void				UnloadStreamSound(unsigned int theSoundID);
	void				GetStreamLevel(unsigned int theSoundID, float* theLeft, float* theRight);

	//
	// Load/Play Music
	//
	unsigned int		LoadMusic(char *theFilename);
	void				UnloadMusic(unsigned int theHandle);
	void				PlayMusic(unsigned int theHandle, unsigned int theOffset);
	void				StopMusic(unsigned int theHandle);
	void				SetMusicVolume(unsigned int theHandle, float theVolume);
	void				SetMusicTrackVolume(unsigned int theHandle, unsigned int theTrack, float theVolume);
	
	//
	// Advanced stuff for custom sound playing.
	// This was put in to port BeepBox, but it's not necesary unless you 
	//
	void				GetSampleRate(unsigned int& theMin, unsigned int& theMax);
	unsigned int		GetFrequency();
	unsigned int		CreateDynamicSound(void* theDataPacket, int theBufferLength);
	
	//
	// Queue sounds to position
	//
	void				SetSoundPosition(unsigned int theHandle, float thePos);
	float				GetSoundPosition(unsigned int theHandle);
	
	void				AdPause(bool theState); // Special pause for ads in WASM
}
