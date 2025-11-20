#include "sound_core.h"
#include <emscripten.h>

#include <unistd.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "os_core.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisfile.h>
#include <assert.h>

//#include <xmp.h>
#include "OpenMPT/libopenmpt/libopenmpt.h"
#include "OpenMPT/libopenmpt/libopenmpt_stream_callbacks_file.h"

#define __HEADER
#include "common.h"
#undef __HEADER


//
// WASM Core... "music" is basically "streams" with the stream reading from a callback function.
// This could be constructed better by simply giving us streams with a pointer to a callback and
// make music a child of that...
//


namespace Sound_Core
{
	int gDebugSoundInstance=-1;
	
	
	bool gSound_Started=false;
	bool gPaused=false;
	ALCdevice* gDevice=NULL;
	ALCcontext* gContext=NULL;
	
	int gMusicDevice=NULL;
	bool gAdPause=false;
	

	//
	// Global list of all soundinstances
	// create some kind of webworker instance every time you algen a buffer.  So we must pre-allocate
	// a sort of holdy-box for our sound instance, but never initialize it until we actually try to play it.
	//
	struct SoundID;

	struct SoundInstance
	{
		bool mIsLive=false; // Can't rely on mBufferInstance... BITCH!
		SoundID* mSoundIDPtr=NULL;
		ALuint mBufferInstance=0;
		
		//
		// If mMusicHandle is nonzero, it's a dynamic sound...
		//
		int mMusicHandle=0;
	};
	CleanArray<SoundInstance*> gGlobalInstanceList;

	struct SoundID
	{
//		SoundID() {mHandle[0]=0;mBufferList=NULL;mDuplicates=0;mPicker=0;mFlag=0;}
		SoundID() {mHandle[0]=0;mDuplicates=0;mPicker=0;mFlag=0;}
		ALuint mHandle[1];
		//ALuint* mBufferList;
	
		Array<int> mInstanceList;
		
		unsigned char mDuplicates;
		unsigned char mPicker;
		unsigned char mFlag;
		
		char* mFN="none";
	};
	
	CleanArray<SoundID*> gSoundIDList;
	inline int GetFreeSoundSlot() {int aPos=1;for (aPos=1;aPos<gSoundIDList.Size();aPos++) {if (gSoundIDList[aPos]==NULL) return aPos;}return aPos;}
	inline SoundID* GetSoundID(int theSlot) {if (theSlot<1 || theSlot>=gSoundIDList.Size()) return NULL;return gSoundIDList[theSlot];}
	inline bool IsSoundInstanceLive(SoundInstance* theSoundInstance) {return theSoundInstance->mIsLive;}
	inline int GetOpenSoundInstance(SoundID* theSoundIDPtr)
	{
		int aFindSI;
		for (aFindSI=1;aFindSI<gGlobalInstanceList.Size();aFindSI++) if (gGlobalInstanceList[aFindSI]==NULL) break;
		gGlobalInstanceList[aFindSI]=new SoundInstance;
		gGlobalInstanceList[aFindSI]->mSoundIDPtr=theSoundIDPtr;
		
		//OS_Core::Printf("CreatedSoundInstance[%d] -> %s",aFindSI,theSoundIDPtr->mFN);
		return aFindSI;
	}

	inline void MakeSoundInstanceLive(SoundInstance* theSoundInstance)
	{
		if (!theSoundInstance) return;
		if (theSoundInstance->mIsLive) return;
		
		//OS_Core::Printf("Making Sound Live: %s",theSoundInstance->mSoundIDPtr->mFN);
		
		alGenSources(1,&theSoundInstance->mBufferInstance);
		if (alGetError()!=AL_NO_ERROR) {OS_Core::Printf("Could not make sound instance live! (alGenSources Failed)");return;}
		alSourcei(theSoundInstance->mBufferInstance,AL_BUFFER,theSoundInstance->mSoundIDPtr->mHandle[0]);
		if (alGetError()!=AL_NO_ERROR) {OS_Core::Printf("Could not make sound instance live! (alSourcei Failed)");return;}
		theSoundInstance->mIsLive=true;
	}	
	
	//
	// Music...
	//
	SDL_AudioSpec gSoundSpec;
	float gGlobalMusicVolume=1.0f;
	void MusicMixer(void *userdata, Uint8 *stream, int len);
	int GetFreeMusicHandle();
	bool CreateMusicDevice();

	//
	// For dynamic filling of music buffers...
	//
	struct DynamicSoundData
	{
		void (*mCallback)(short *theBuffer, unsigned int theLength, void* theExtraData);
		void *mExtraData;

	};
	
	struct MusicHandle
	{
	public:
		openmpt_module *mModule=NULL;
		FILE* mFile=NULL;
		float mVolume=1.0f;
		bool mPlaying=false;
		
		//
		// For custom sound buffer callback (Beepbox)
		//
		DynamicSoundData* mCustom=NULL;
		
		bool IsUsed() {return (mFile!=NULL || mCustom!=NULL);}
	};
	Array<MusicHandle> gMusicHandle;
	
	
	
}

void Sound_Core::Startup()
{
	//
	// Anything that needs to be done to start up a sound system
	//
	// #ifdef _PORT_STARTUPSND
	// !ERROR
	// #endif
	//

	if (gSound_Started) return;
	gDevice=alcOpenDevice(NULL);
	if (!gDevice) {::OS_Core::Printf("Could not create OpenAL Device!");return;}
	gContext=alcCreateContext(gDevice,NULL);
	if (!gContext) {::OS_Core::Printf("Could not create OpenAL Context!");return;}
	if (!alcMakeContextCurrent(gContext)) {::OS_Core::Printf("Could not focus the OpenAL Context!");return;}
	gSound_Started=true;
	
	ALfloat listenerPos[] = {0.0, 0.0, 1.0};
	ALfloat listenerVel[] = {0.0, 0.0, 0.0};
	ALfloat listenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};

	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_VELOCITY, listenerVel);
	alListenerfv(AL_ORIENTATION, listenerOri);	
	
	gSoundIDList[0]=NULL;
	
	SDL_Init(SDL_INIT_AUDIO);
	
	
	//
	// XMPlay test stuff...
	//
	/*
	SDL_Init(SDL_INIT_AUDIO);
	
	OS_Core::Printf("Context1: %d",gXMPContext);
	gXMPContext=xmp_create_context();
	OS_Core::Printf("Context2: %d",gXMPContext);
	
	char* aFN="/music/test.it";
	OS_Core::Printf("Attempting Load...[%d]",OS_Core::DoesFileExist(aFN));
	int aResult=xmp_load_module(gXMPContext,aFN);
	if (aResult!=0) 
	{
		OS_Core::Printf("can't load module\n");
	}
	else OS_Core::Printf("LOAD OKAY!!!!");

	struct xmp_frame_info aInfo;

	xmp_get_frame_info(gXMPContext, &aInfo);
	OS_Core::Printf("Estimated Time: %d",aInfo.total_time);
	
	SDL_AudioSpec a;	
	
    a.freq = 44100;
    a.format = AUDIO_S16SYS;
    a.channels = 2;
    a.samples = 2048;
    a.callback = fill_audio;
    a.userdata = gXMPContext;

    if (SDL_OpenAudio(&a, NULL) < 0) 
	{
		OS_Core::Printf("OpenAL Error: %s\n", SDL_GetError());
    }
	else OS_Core::Printf("Here we go!!!!");
	
	aResult=xmp_start_player(gXMPContext,44100,0);
	xmp_play_frame(gXMPContext);
	OS_Core::Printf("XMP_Start_Player = %d",aResult);
	SDL_PauseAudio(0);
	*/
}

void Sound_Core::Shutdown()
{
	if (!gSound_Started) return;
	//
	// Anything that needs to be done to shut down a sound system
	//
	// #ifdef _PORT_STARTUPSND
	// !ERROR
	// #endif
	//
	
	if (gSound_Started) 
	{
		//#ifdef _PORT_CLEANUPSND
		//!ERROR
		//#endif
		
		for (int aCount=0;aCount<gSoundIDList.Size();aCount++) if (gSoundIDList[aCount]) UnloadSound(aCount);
		gSoundIDList.Reset();
		
		alcMakeContextCurrent(NULL);
		alcDestroyContext(gContext);
		alcCloseDevice(gDevice);
		
		gContext=NULL;
		gDevice=NULL;
		gSound_Started=false;
	}
}

void Sound_Core::SetGlobalSoundVolume(float theVolume)
{
	if (!gSound_Started) return;
	//
	// Set the global volume for all sounds
	//
	//#ifdef _PORT_SNDVOLUME
	//!ERROR
	//#endif
	//
	alListenerf(AL_GAIN,theVolume);
}

void Sound_Core::Multitasking(bool isForeground)
{
	if (!gSound_Started) return;
	
	//OS_Core::Printf("Sound_Core::Multitasking(%d) : %d",isForeground,gAdPause);

	//
	// Do whatever needs to be done when the app gains or loses focus
	//
	//#ifdef _PORT_MULTITASK
	//!ERROR
	//#endif
	//
	if (gAdPause) return; // Don't allow "multitasking" while adpaused...
	Pause(!isForeground);
}

void Sound_Core::AdPause(bool theState)
{
	if (!gAdPause) gAdPause=theState; // If we're recovering, set it first so we can unpause...
	Pause(theState);
	gAdPause=theState;
}

void Sound_Core::Pause(bool theState)
{
	if (!gSound_Started) return;
	if (gAdPause) return;
	
	//OS_Core::Printf("Pause Sound Core (2) = %d",theState);
	
	//
	// Pauses all sound and music playing
	//
	//#ifdef _PORT_SNDFINAL
	//!ERROR
	//#endif
	//
	gPaused=theState;
	
	for (int aCount=1;aCount<gGlobalInstanceList.Size();aCount++)
	{
		SoundInstance* aSI=gGlobalInstanceList[aCount];
		if (aSI->mIsLive && aSI->mBufferInstance)
		{
			ALint aResult;
			alGetSourcei(aSI->mBufferInstance,AL_SOURCE_STATE,&aResult);
			if (theState && aResult==AL_PLAYING) alSourcePause(aSI->mBufferInstance);
			if (!theState && aResult==AL_PAUSED) alSourcePlay(aSI->mBufferInstance);
		}
	}
	
	if (gMusicDevice)
	{
		if (gPaused) SDL_LockAudioDevice(gMusicDevice);
		else SDL_UnlockAudioDevice(gMusicDevice);
	}
}

int gFakeSound=-1;
unsigned int Sound_Core::LoadSound(char *theFilename, int theDuplicates)
{
	if (!gSound_Started) return 0;
	
	_FixPath(theFilename);

/*	
	if (gFakeSound==-1)
	{
		//
		// There is some kind of problem with loading the first sound... so we load a fake sound
		// first...
		//
		gFakeSound=0; // So we don't recurse inside the load...
		gFakeSound=LoadSound(theFilename,1);
		OS_Core::Printf("FAKE SOUND = %d",gFakeSound);
	}
*/	
	
	//
	// Loads the data of a sound in... this should return a HANDLE to the sound.
	// A handle is not what we play, it is just the data.  We actually play sound
	// INSTANCES (below).  
	//
	//#ifdef _PORT_LOADSND
	//!ERROR
	//#endif
	//
	OggVorbis_File vf;
	int current_section=0;
	FILE *aFile = fopen(theFilename,"rb");
	if (aFile==NULL)
	{
		OS_Core::Printf("Could not load OGG (FILENOTFOUND): %s",theFilename);
		return 0;
	}
	
	if(ov_open(aFile, &vf, NULL, 0) < 0) 
	{
		fclose(aFile);
		OS_Core::Printf("Could not load OGG (OV_OPEN FAILED): %s",theFilename);
		return 0;
	}
	
	vorbis_info *anInfo = ov_info(&vf,-1);
	
	ALenum aWaveFormat;
	short aChannels=anInfo->channels;
	long aSampleRate=anInfo->rate;
	short aAverageBytesPerSec;
	short aBitsPerSample=16;
	short aBlockAlign=aChannels*(aBitsPerSample/8);
	long aDataSize;
	
	if (aChannels==1) aWaveFormat=AL_FORMAT_MONO16;
	else aWaveFormat=AL_FORMAT_STEREO16;

	int aLenBytes=aDataSize=(int)(ov_pcm_total(&vf,-1)*aBlockAlign);	
	char *aTempBuffer=new char[aLenBytes];
	
	char *aPtr=aTempBuffer;
	int aNumBytes=aLenBytes;
	while(aNumBytes>0)
	{		
		long aRet=ov_read(&vf,aPtr,aNumBytes,0,2,1,&current_section);		
		if (aRet==0) break;
		else if (aRet<0) break;
		else {aPtr+=aRet;aNumBytes-=aRet;}
	}
	
	int aSlot=GetFreeSoundSlot();
	SoundID* aSound=new SoundID;
	gSoundIDList[aSlot]=aSound;
	
	alGenBuffers(1,aSound->mHandle);
	if (alGetError()!=AL_NO_ERROR) {OS_Core::Printf("Could not load OGG sound (alGenBuffers Failed): %s",theFilename);UnloadSound(aSlot);return 0;}

	alBufferData(aSound->mHandle[0],aWaveFormat,aTempBuffer,aDataSize,aSampleRate);	
	if (alGetError()!=AL_NO_ERROR) {OS_Core::Printf("Could not load OGG sound (alBufferData Failed): %s",theFilename);UnloadSound(aSlot);return 0;}
	
	aSound->mFN=theFilename;
	aSound->mDuplicates=theDuplicates;
	aSound->mInstanceList.GuaranteeSize(aSound->mDuplicates);
	gGlobalInstanceList.GuaranteeSize(gGlobalInstanceList.Size()+aSound->mDuplicates); // For speed
	for (int aCount=0;aCount<aSound->mDuplicates;aCount++) 
	{
		aSound->mInstanceList[aCount]=GetOpenSoundInstance(aSound);
		//SoundInstance* aSI=gGlobalInstanceList[aSound->mInstanceList[aCount]];
	}
	
	
	
	/*
	aSound->mBufferList=new ALuint[theDuplicates];
	alGenSources(theDuplicates,aSound->mBufferList);
	if (alGetError()!=AL_NO_ERROR) {OS_Core::Printf("Could not load OGG sound (alGenSources Failed): %s",theFilename);UnloadSound(aSlot);return 0;}
	for (int aCount=0;aCount<aSound->mDuplicates;aCount++) alSourcei(aSound->mBufferList[aCount],AL_BUFFER,aSound->mHandle[0]);
	*/
	return aSlot;
}

unsigned int Sound_Core::GetSoundBuffer(unsigned int theSoundID)
{
	if (!gSound_Started) return 0;
	//
	// Gets an INSTANCE of the sound that was loaded (this is how we play multiples)
	// theSoundID is the HANDLE that was returned by LoadSound.
	//
	//#ifdef _PORT_LOADSND
	//!ERROR
	//#endif
	//
	SoundID* aSound=GetSoundID(theSoundID);
	if (aSound) if (aSound->mPicker<aSound->mDuplicates) 
	{
		//OS_Core::Printf("Give Instance: %d",aSound->mInstanceList[aSound->mPicker]);
		return aSound->mInstanceList[aSound->mPicker++];
	}
	return 0;
}

void Sound_Core::PlaySoundBuffer(unsigned int theBufferID)
{
	if (!gSound_Started) return;
	
	

/*
	if (gFakeSound!=-1)
	{
		//
		// Part of the sound screwup kludge...
		// Unload our fake sound so it doesn't occupy memory...
		//
		UnloadSound(gFakeSound);
		gFakeSound=-1;
	}
*/	
	
	//
	// Play a sound instance.
	//
	//#ifdef _PORT_PLAYSND
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	
	if (theBufferID==gDebugSoundInstance)
	{
		OS_Core::Printf("Play Debugger One...%d -> %d",theBufferID,aSI);
	}
	
	if (aSI)
	{
		if (aSI->mMusicHandle>0) {PlayMusic(aSI->mMusicHandle,0);return;}
		if (!IsSoundInstanceLive(aSI)) MakeSoundInstanceLive(aSI);
		alSourcePlay(aSI->mBufferInstance);
	}
	//alSourcePlay(theBufferID);
}

void Sound_Core::StopSoundBuffer(unsigned int theBufferID)
{
	if (!gSound_Started) return;
	//
	// Stop playing a sound instance
	//
	//#ifdef _PORT_PLAYSND
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI) 
	{
		if (aSI->mMusicHandle>0) {StopMusic(aSI->mMusicHandle);return;}
		if (IsSoundInstanceLive(aSI)) alSourceStop(aSI->mBufferInstance);
	}
	
	//alSourceStop(theBufferID);
}

void Sound_Core::SetSoundBufferVolume(unsigned int theBufferID, float theVolume)
{
	if (!gSound_Started) return;
	//
	// Sets the volume of a single instance (for playing sounds at varied volumes)
	//
	//#ifdef _PORT_SNDVOLUME
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI)
	{
		if (aSI->mMusicHandle>0) 
		{
			SetMusicVolume(aSI->mMusicHandle,theVolume);return;
		}
		else
		{
			if (!IsSoundInstanceLive(aSI)) MakeSoundInstanceLive(aSI);
			alSourcef(aSI->mBufferInstance,AL_GAIN,theVolume);
		}
	}
//	alSourcef(theBufferID,AL_GAIN,theVolume);
}

void Sound_Core::SetSoundBufferFrequency(unsigned int theBufferID, float theFrequency)
{
	if (!gSound_Started) return;
	//
	// Adjust the pitch of a sound instance
	//
	//#ifdef _PORT_SNDPITCH
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI)
	{
		if (aSI->mMusicHandle>0) return;
		if (!IsSoundInstanceLive(aSI)) MakeSoundInstanceLive(aSI);
		alSourcef(aSI->mBufferInstance,AL_PITCH,theFrequency);
	}
//	alSourcef(theBufferID,AL_PITCH,theFrequency);
}

float Sound_Core::GetSoundBufferFrequency(unsigned int theBufferID)
{
	if (!gSound_Started) return 0;
	//
	// Returns the pitch of a sound instance
	//
	//#ifdef _PORT_SNDPITCH
	//!ERROR
	//#endif
	//
	
	
	ALfloat aFreq;
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI)
	{
		if (aSI->mMusicHandle>0) return GetFrequency();
		
		if (!IsSoundInstanceLive(aSI)) aFreq=1.0f;
		else alGetSourcef(aSI->mBufferInstance, AL_PITCH, &aFreq);
	}
	
	//alGetSourcef(theBufferID, AL_PITCH, &aFreq);
	return aFreq;
}

bool Sound_Core::IsSoundBufferPlaying(unsigned int theBufferID)
{
	if (!gSound_Started) return false;
	//
	// Tells us if a sound is playing
	//
	//#ifdef _PORT_PLAYSND
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI)
	{
		if (aSI->mMusicHandle>0) return gMusicHandle[aSI->mMusicHandle].mPlaying;
		if (!IsSoundInstanceLive(aSI)) return false;
		
		ALint aResult;
		alGetSourcei(aSI->mBufferInstance,AL_SOURCE_STATE,&aResult);
		return (aResult==AL_PLAYING);
	}
	return false;
	
	//alGetSourcei(theBufferID,AL_SOURCE_STATE,&aResult) ;
	//return (aResult==AL_PLAYING);
}

void Sound_Core::UnloadSound(unsigned int theSoundID)
{
	if (!gSound_Started) return;
	//
	// Unloads a sound from memory
	//
	//#ifdef _PORT_LOADSND
	//!ERROR
	//#endif
	//
	SoundID* aSound=GetSoundID(theSoundID);
	if (aSound)
	{
		/*
		SoundID* mSoundIDPtr=NULL;
		ALuint mBufferInstance=-1;
		*/
		for (int aCount=0;aCount<aSound->mInstanceList.Size();aCount++)
		{
			
			SoundInstance* aSI=gGlobalInstanceList[aSound->mInstanceList[aCount]];
			if (aSI)
			{
				if (aSI->mMusicHandle>0)
				{
					UnloadMusic(aSI->mMusicHandle);
					aSI->mMusicHandle=0;
				}
				
				alDeleteSources(1,&aSI->mBufferInstance);
				delete gGlobalInstanceList[aSound->mInstanceList[aCount]];
				gGlobalInstanceList[aSound->mInstanceList[aCount]]=NULL;
			}
		}
		
		alDeleteBuffers(1,aSound->mHandle);
		//delete [] aSound->mBufferList;
		delete aSound;
		gSoundIDList[theSoundID]=NULL;
	}
}

unsigned int Sound_Core::LoadStreamSound(char *theFilename)
{
	if (!gSound_Started) return 0;
	//
	// Returns a handle of some sort to a sound you're going to stream.
	//
	//#ifdef _PORT_STREAMSND
	//!ERROR
	//#endif
	//
	//return BASS_StreamCreateFile(false,OS_Core::FixPath(theFilename),0,0,0);
	
	//
	// In WebAssembly/OpenAL, streaming requires its own processor.
	// I'm not going to do that at this point.  But we COULD have a file open and dump it to a
	// music buffer if we wanted to...
	//
	
	int aSlot=LoadSound(theFilename,1);
	return GetSoundBuffer(aSlot);
}

void Sound_Core::UnloadStreamSound(unsigned int theSoundID)
{
	if (!gSound_Started) return;
	//
	// Destroys a handle to a streaming sound
	//
	//#ifdef _PORT_STREAMSND
	//!ERROR
	//#endif
	//
	
	//
	// In OpenAL, streams are just sounds... but we only know the buffer ID.
	// So we have to cycle through and find it...
	//
	
	for (int aCount=0;aCount<gSoundIDList.Size();aCount++)
	{
		if (gSoundIDList[aCount]->mDuplicates==1)
		{
			//if (gSoundIDList[aCount]->mBufferList[0]==theSoundID)
			if (gSoundIDList[aCount]->mInstanceList[0]==theSoundID)
			{
				UnloadSound(aCount);
				break;
			}
		}
	}
}

void Sound_Core::GetStreamLevel(unsigned int theSoundID, float* theLeft, float* theRight)
{
	if (!gSound_Started) {*theLeft=0;*theRight=0;return;}
	//
	// Returns how "loud" a streaming sound is playing (this is often used to queue up
	// mouth motion or other motion with voice)
	//
	//#ifdef _PORT_STREAMSND
	//!ERROR
	//#endif
	//

	// 
	// In OpenAL, this is not possible unless we keep the sound data around.
	// I MIGHT want to do that eventually, but for now, I don't.
	//
	if(IsSoundBufferPlaying(theSoundID))
	{
		*theLeft=1.0f;
		*theRight=1.0f;
	}
	else
	{
		*theLeft=0.0f;
		*theRight=0.0f;
	}
}

void Sound_Core::SetSoundBufferLooping(unsigned int theBufferID, bool theState)
{
	if (!gSound_Started) return;
	if (theBufferID==0) return;
	//
	// Set us a sound instance to loop or not when it is played.
	//
	//#ifdef _PORT_SNDLOOP
	//!ERROR
	//#endif
	//
	SoundInstance* aSI=gGlobalInstanceList[theBufferID];
	if (aSI)
	{
		if (aSI->mMusicHandle>0) return;
		if (!IsSoundInstanceLive(aSI)) MakeSoundInstanceLive(aSI);
		alSourcei(aSI->mBufferInstance,AL_LOOPING,theState);
		//OS_Core::Printf("SET LOOPING: %d",theBufferID);
	}
	
	//alSourcei(theBufferID,AL_LOOPING,theState);
}

void Sound_Core::GetSampleRate(unsigned int& theMin, unsigned int& theMax)
{
	if (!gSound_Started) return;
	//
	//#ifdef _PORT_CUSTOMSND
	//!ERROR
	//#endif
	//
	
	// NOTE: This is not always necessary to be ported.  This is only here for if you want to make a custom mixer.
	// I added this function for BeepBox.
	// This is how it's done in BASS...
	/*
	BASS_INFO aInfo;
	BASS_GetInfo(&aInfo);

	theMin=aInfo.minrate;
	theMax=aInfo.maxrate;
	*/
}

unsigned int Sound_Core::GetFrequency()
{
	// SDL audio mixer is hard set to 48000... you can ^F for it...
    return 48000/2;
}

void Sound_Core::SetSoundPosition(unsigned int theHandle, float thePos)
{
}

float Sound_Core::GetSoundPosition(unsigned int theHandle)
{
	return 0;
}



void Sound_Core::MusicMixer(void *userdata, Uint8 *stream, int len)
{
	//openmpt_module_read_mono(gMusicHandle[0].mModule,48000,len/2,(int16_t*)stream);
	//return;
	static bool aHadVolume=true;
	if (gGlobalMusicVolume==0 || (Sound_Core::gPaused|Sound_Core::gAdPause))
	{
		if (aHadVolume)
		{
			memset(stream,0,len);
			aHadVolume=false;
			//if (Sound_Core::gPaused|Sound_Core::gAdPause) SDL_PauseAudioDevice(Sound_Core::gMusicDevice,1);
		}
		return;
	}
	
	aHadVolume=true;
	
	static int16_t aData[10000];
	//static Uint8 aMix[10000];
	Uint8* aMix=stream;
	memset(aMix,0,len);

	bool aPlayed=false;
	for (int aCount=0;aCount<gMusicHandle.Size();aCount++)
	{
		if (gMusicHandle[aCount].mPlaying)
		{
			aPlayed=true;
			// Dynamic sound... here's where we'd call our dynamic sound mixer...
			
			if (gMusicHandle[aCount].mCustom)
			{
				//
				// Okay, custom music not working... is it the freq?
				//
				gMusicHandle[aCount].mCustom->mCallback(aData,len,gMusicHandle[aCount].mCustom->mExtraData);
				//memset(aData,OS_Core::Tick(),len);
			}
			else openmpt_module_read_mono(gMusicHandle[aCount].mModule,gSoundSpec.freq,len/2,aData);
			SDL_MixAudioFormat(aMix, (Uint8*)aData, gSoundSpec.format, len, (int)(gGlobalMusicVolume*(float)SDL_MIX_MAXVOLUME*gMusicHandle[aCount].mVolume));
		}
	}
	
	//if (aPlayed) memcpy(stream,aMix,len);
	//else memset(stream,0,len);
}

static void MusicLog( const char * message, void * userdata ) {if (message) {OS_Core::Printf("openmpt: %s", message );}}
static int MusicError( int error, void * userdata ) {return OPENMPT_ERROR_FUNC_RESULT_DEFAULT & ~OPENMPT_ERROR_FUNC_RESULT_LOG;}

int Sound_Core::GetFreeMusicHandle()
{
	int aFind=0;
	for (aFind=1;aFind<gMusicHandle.Size();aFind++) if (!gMusicHandle[aFind].IsUsed()) break;
	return aFind;
}

void Sound_Core::SetGlobalMusicVolume(float theVolume)
{
	if (!gSound_Started) return;
	gGlobalMusicVolume=theVolume;
}

bool Sound_Core::CreateMusicDevice()
{
	if (!Sound_Core::gMusicDevice)
	{
		SDL_AudioSpec aSpec;	
		aSpec.freq=48000;
		aSpec.format=AUDIO_S16;
		aSpec.channels=1;
		aSpec.samples=1024; //2048;
		aSpec.callback=MusicMixer;
		//aSpec.userdata=&gMusicHandle[aFind];
		
		Sound_Core::gMusicDevice=SDL_OpenAudioDevice(NULL, 0, &aSpec, &gSoundSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
		if (Sound_Core::gMusicDevice==0) return false; 
		SDL_PauseAudioDevice(Sound_Core::gMusicDevice,0);
	}
	
	return true;
}

unsigned int Sound_Core::LoadMusic(char *theFilename)
{
	if (!gSound_Started) return 0;
	
	_FixPath(theFilename);
	
	int aFind=GetFreeMusicHandle();
	
	int aModuleError = OPENMPT_ERROR_OK;
	const char *aModuleErrorString=NULL;
	
	gMusicHandle[aFind].mFile=fopen(theFilename,"rb");
	if (gMusicHandle[aFind].mFile==NULL) 
	{
		OS_Core::Printf("LoadMusic(%s) : FILE NOT FOUND!",theFilename);
		return -1;
	}
	
	gMusicHandle[aFind].mModule=openmpt_module_create2( openmpt_stream_get_file_callbacks(),gMusicHandle[aFind].mFile, &MusicLog, NULL, &MusicError, NULL, &aModuleError, &aModuleErrorString, NULL);
	if (!gMusicHandle[aFind].mModule) {UnloadMusic(aFind);OS_Core::Printf("LoadMusic(%s) : MODULE OPEN ERROR!!!!",theFilename);return -1;}
	
	//fclose(gMusicHandle[aFind].mFile);
	
	//OS_Core::Printf("MODULE LOADED = %d",gMusicHandle[aFind].mModule);
	
	openmpt_module_set_repeat_count(gMusicHandle[aFind].mModule,-1);
	openmpt_module_set_position_order_row(gMusicHandle[aFind].mModule,0,0);
	
	if (!CreateMusicDevice()) {UnloadMusic(aFind);OS_Core::Printf("SDL Audio Error: %s\n", SDL_GetError());return 0;}
	
	return aFind;
}

void Sound_Core::UnloadMusic(unsigned int theHandle)
{
	if (!gSound_Started) return;
	if (theHandle<=0 || theHandle>=gMusicHandle.Size()) return;
	if (!gMusicHandle[theHandle].IsUsed()) return;
	
	if (gMusicHandle[theHandle].mModule) openmpt_module_destroy(gMusicHandle[theHandle].mModule);
	if (gMusicHandle[theHandle].mFile) fclose(gMusicHandle[theHandle].mFile);
	gMusicHandle[theHandle].mModule=NULL;		
	gMusicHandle[theHandle].mFile=NULL;
	gMusicHandle[theHandle].mCustom=NULL;
}


void Sound_Core::PlayMusic(unsigned int theHandle, unsigned int theOffset)
{
	if (!gSound_Started) return;
	if (theHandle<=0 || theHandle>=gMusicHandle.Size()) return;
	if (!gMusicHandle[theHandle].IsUsed()) return;
	if (gMusicHandle[theHandle].mModule) openmpt_module_set_position_order_row(gMusicHandle[theHandle].mModule,theOffset,0);
	gMusicHandle[theHandle].mPlaying=true;
	SDL_PauseAudioDevice(gMusicDevice,0);
}	

void Sound_Core::StopMusic(unsigned int theHandle)
{
	if (!gSound_Started) return;
	if (theHandle<=0 || theHandle>=gMusicHandle.Size()) return;
	if (!gMusicHandle[theHandle].IsUsed()) return;
	gMusicHandle[theHandle].mPlaying=false;
	
	bool aPlaying=false;
	for (int aCount=0;aCount<gMusicHandle.Size();aCount++) if (gMusicHandle[aCount].mPlaying) {aPlaying=true;break;}
	if (!aPlaying) SDL_PauseAudioDevice(gMusicDevice,1);
}

void Sound_Core::SetMusicVolume(unsigned int theHandle, float theVolume)
{
	if (!gSound_Started) return;
	if (theHandle<=0 || theHandle>=gMusicHandle.Size()) return;
	if (!gMusicHandle[theHandle].IsUsed()) return;
	gMusicHandle[theHandle].mVolume=theVolume;
}

void Sound_Core::SetMusicTrackVolume(unsigned int theHandle, unsigned int theTrack, float theVolume)
{
	if (!gSound_Started) return;
	//
	// This sets the volume of ONE SPECIFIC track of music.  I think this is BASS specific... so if no bass,
	// don't worry about it.
	//
	//#ifdef _PORT_MUSICVOLUME
	//!ERROR
	//#endif
	//
	//
	// No Music in Web
	//
}


unsigned int Sound_Core::CreateDynamicSound(void* theDataPacket, int theBufferLength)
{
	DynamicSoundData* aDD=(DynamicSoundData*)theDataPacket;
	
	int aSlot=GetOpenSoundInstance(NULL);
	SoundInstance* aSI=gGlobalInstanceList[aSlot];
	aSI->mMusicHandle=GetFreeMusicHandle();
	gMusicHandle[aSI->mMusicHandle].mCustom=aDD;
	
	if (!CreateMusicDevice()) {UnloadMusic(aSI->mMusicHandle);UnloadSound(aSlot);OS_Core::Printf("SDL Audio Error: %s\n", SDL_GetError());return 0;}
	return aSlot;
}

