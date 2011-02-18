//-------------------------------------------------------------------------------------------------------
//
// © 2009, Ein Zwerg Technologies, All Rights Reserved
//
//-------------------------------------------------------------------------------------------------------
#include "audioeffectx.h"
#include "pluginterfaces/vst2.x/aeffectx.h"
#include "vsthost.h"
#include "Profile.h"

#if _WIN32
#include <windows.h>
#elif TARGET_API_MAC_CARBON
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <stdio.h>
#include <math.h> // @#**! floor
#include "sound.h"

//-------------------------------------------------------------------------------------------------------
static const VstInt32 kBlockSize = SizeAudioBuffer;
//static const float kSampleRate = SampleRate;
//static const float kSampleRate = 48000.f;
//static const VstInt32 kNumProcessCycles = 5;

VstTimeInfo vstTimeInfo;

//-------------------------------------------------------------------------------------------------------
typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);
static VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);

static bool checkPlatform ();
static void checkEffectProperties (AEffect* effect);
extern bool checkEffectEditor (AEffect* effect); // minieditor.cpp

//-------------------------------------------------------------------------------------------------------
// PluginLoader
//-------------------------------------------------------------------------------------------------------
struct PluginLoader
{
//-------------------------------------------------------------------------------------------------------
	void* module;

	PluginLoader ()
		: module (0)
	{}

	~PluginLoader ()
	{
		if(module)
		{
			#if _WIN32
			FreeLibrary ((HMODULE)module);
			#elif TARGET_API_MAC_CARBON
			CFBundleUnloadExecutable ((CFBundleRef)module);
			CFRelease ((CFBundleRef)module);
			#endif
		}
	}

	bool loadLibrary (const char* fileName)
	{
		#if _WIN32
		module = LoadLibrary (fileName);
		#elif TARGET_API_MAC_CARBON
		CFStringRef fileNameString = CFStringCreateWithCString (NULL, fileName, kCFStringEncodingUTF8);
		if (fileNameString == 0)
			return false;
		CFURLRef url = CFURLCreateWithFileSystemPath (NULL, fileNameString, kCFURLPOSIXPathStyle, false);
		CFRelease (fileNameString);
		if (url == 0)
			return false;
		module = CFBundleCreate (NULL, url);
		CFRelease (url);
		if (module && CFBundleLoadExecutable ((CFBundleRef)module) == false)
			return false;
		#endif
		return module != 0;
	}

	PluginEntryProc getMainEntry ()
	{
		PluginEntryProc mainProc = 0;
		#if _WIN32
		mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "VSTPluginMain");
		if(!mainProc)
			mainProc = (PluginEntryProc)GetProcAddress ((HMODULE)module, "main");
		#elif TARGET_API_MAC_CARBON
		mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName((CFBundleRef)module, CFSTR("VSTPluginMain"));
		if (!mainProc)
			mainProc = (PluginEntryProc)CFBundleGetFunctionPointerForName((CFBundleRef)module, CFSTR("main_macho"));
		#endif
		return mainProc;
	}
//-------------------------------------------------------------------------------------------------------
};


//-------------------------------------------------------------------------------------------------------

struct VstHost : Item
{
	PluginLoader loader;
	AEffect * effect;
	char name[128];
	int map_a;
	int map_p[512];
	VstMidiEvent mev;
	VstEvents ev;

	//VstTimeInfo vstTimeInfo;

	VstHost::VstHost( const char * n, int type, int nInputs, int ma, int & result ) 
		: Item(SizeAudioBuffer,type,nInputs)
		, map_a(ma)
	{
		strcpy( name, n );
		for ( int i=0; i<512;i++ )
		{
			map_p[i] = i;
		}

		// midi events:
		memset( &mev, 0, sizeof(VstMidiEvent) );
		mev.byteSize = sizeof(VstMidiEvent);
		mev.type  = kVstMidiType;
		mev.flags = kVstMidiEventIsRealtime;

		memset( &ev, 0, sizeof(VstEvents) );
		ev.numEvents = 1;
		ev.events[0] = (VstEvent*)&mev;

		// timer:
		vstTimeInfo.samplePos = 0.0;
		vstTimeInfo.sampleRate = SampleRate;
		vstTimeInfo.nanoSeconds = 0.0;
		vstTimeInfo.ppqPos = 0.0;
		vstTimeInfo.tempo = 120.0;
		vstTimeInfo.barStartPos = 0.0;
		vstTimeInfo.cycleStartPos = 0.0;
		vstTimeInfo.cycleEndPos = 100.0;
		vstTimeInfo.timeSigNumerator = 4;
		vstTimeInfo.timeSigDenominator = 4;
		vstTimeInfo.smpteOffset = 0;
		vstTimeInfo.smpteFrameRate = 1;
		vstTimeInfo.samplesToNextClock = 0;
		vstTimeInfo.flags = kVstTransportPlaying
							| kVstNanosValid
							| kVstPpqPosValid	///< VstTimeInfo::ppqPos valid
							| kVstTempoValid	///< VstTimeInfo::tempo valid
							| kVstTimeSigValid 	///< VstTimeInfo::timeSigNumerator and VstTimeInfo::timeSigDenominator valid
							| kVstSmpteValid;	///< VstTimeInfo::smpteOffset and VstTimeInfo::smpteFrameRate valid

		// xml:
		result = load( n );
	}

	virtual const char * typeName()
	{
		return name;
	}

	virtual VstHost::~VstHost() 
	{ 
		if ( effect ) 
		{
			//effect->dispatcher (effect, effEditClose, 0, 0, 0, 0);
			effect->dispatcher (effect, effClose, 0, 0, 0, 0);
			//delete  effect; ///???
			effect = 0;
		}
	}

	void setParam( int i, float p )
	{
		if ( effect ) 
		{
			//if ( i>=ncon ) {printf(__FUNCTION__" : index err %d\n",i); return;}
			effect->setParameter (effect, i, p );
		}
	}
	void suspend()
	{
		effect->dispatcher (effect, effMainsChanged, 0, 0, 0, 0);
	}
	void resume()
	{
		effect->dispatcher (effect, effMainsChanged, 0, 1, 0, 0);
	}

	bool canDo( const char * s )
	{
		if ( effect ) 
		{
			// map 'don't know' to NOmeansNO.
			return ( 1 == effect->dispatcher (effect, effCanDo, 0, 0, (void*)s, 0) );
		}
		return 0;
	}

	bool sendMidi( int a, int b, int c )
	{
		if ( effect ) 
		{
			mev.midiData[0]  = a & 0xff;
			mev.midiData[1]  = b & 0x7f;
			mev.midiData[2]  = c & 0x7f;
			// don't bother asking, just fire.
			effect->dispatcher (effect, effProcessEvents, 0, 0, (void*)&ev, 0);
			return 1;
		}
		return 0;
	}

	double getNanos()
	{
		static double timeScale = 0;
		if ( ! timeScale )
		{
			LARGE_INTEGER freq;
 			QueryPerformanceFrequency( &freq );
			timeScale = 1000.0 * 1000.0  / freq.QuadPart;
		}
		LARGE_INTEGER now;
		QueryPerformanceCounter( &now );
		return ( now.QuadPart * timeScale );
	}



	bool processAudio( float ** buffer, int nBuffer, int nElems )
	{
		PROFILE;
		if ( effect )
		{
			if ( con[0] ) 
			{
				con[0]->processAudio( buffer, nBuffer, nElems );
			}
			effect->processReplacing( effect, buffer, buffer, nElems );
			return 1;
		}
		return 0;
	}

	virtual bool updateFrame( float t, int f )
	{
		PROFILE;
		if ( ! effect ) return false;
		//w32::Lock lock(&critSec);

		// time (needed for sequencers, drum loops, etc):
		calcTime();

		// checkInputs:
		for ( int c=0; c<MaxConnections; c++)
		{
			if ( con[c] )
			{
				if ( con[c]->bSize > 1 )
				{
					if ( con[c]->bSize == 3) 
					// &&	((con[c]->type==Item::IT_MIDI_NOTE) || (con[c]->type==Item::IT_MIDI_PROG))
					{
						if ( ! con[c]->A )
							continue;
						char b0 = char(con[c]->v[0] * 255.0);
						char b1 = char(con[c]->v[1] * 127.0);
						char b2 = char(con[c]->v[2] * 127.0);

						//static int t = 10;
						//static int lb1 = 0;
						//if ( b1 == lb1 )
						//{
						//	if ( t > 0 )
						//	{
						//		t --;
						//		continue;
						//	}
						//}
						//lb1 = b1;
						//t = 20;

						sendMidi( b0,b1,b2 );
					}
					continue; // data input
				}
				// controller mapped to vst param:
				setParam( map_p[c], con[c]->conMult[c] * con[c]->value() );
			}
		}

		// map angle to param:
		if ( map_a != -1 )
		{
			setParam( map_a, a ); ///SCALE??
		}

		return 1;
	}

	int VstHost::load ( const char* fileName )
	{
		PROFILE;

		if(!checkPlatform ())
		{
			printf ("Platform verification failed! Please check your Compiler Settings!\n");
			return 0;
		}

		if(!loader.loadLibrary (fileName))
		{
			printf ("Failed to load VST Plugin library!\n");
			return 0;
		}

		PluginEntryProc mainEntry = loader.getMainEntry ();
		if(!mainEntry)
		{
			printf ("VST Plugin main entry not found!\n");
			return 0;
		}

		// ... here we'll spend a lot of time...
		effect = mainEntry (HostCallback);
		if(!effect)
		{
			printf ("Failed to create effect instance!\n");
			return 0;
		}

		VstIntPtr res = 0;
		res = effect->dispatcher (effect, effOpen, 0, 0, 0, 0);

		res = effect->dispatcher (effect, effSetSampleRate, 0, 0, 0, (VstInt32)(SampleRate));

		//// this is a safety measure against some plugins that only set their buffers
		//// ONCE - this should ensure that they allocate a buffer that's large enough
		//res = effect->dispatcher (effect, effSetBlockSize, 0, 11025, 0, 0);
		//// force a cycle:
		//resume();
		//suspend();

		// now set real size:
		res = effect->dispatcher (effect, effSetBlockSize, 0, kBlockSize, 0, 0);

		checkEffectProperties (effect);
//		checkEffectProcessing (effect);
//		checkEffectEditor (effect);
		
		resume();
		bool r = 1;
		//if ( canDo("receiveVstMidiEvent") )
		//{
		//	r = sendMidi( 0x90, 0x30, 0x37f );
		//}
		printf ("HOST> Loaded '%s' %d.\n", fileName,r);
		return r;
	}

	void calcTime()
	{
		// we don't care for the mask in here
		static double fSmpteDiv[] =	{	24.f,	25.f,	24.f,	30.f,	29.97f,	30.f	};

		vstTimeInfo.nanoSeconds = getNanos();
		vstTimeInfo.samplePos += kBlockSize;

		double dPos = vstTimeInfo.samplePos / vstTimeInfo.sampleRate;
		vstTimeInfo.ppqPos = dPos * vstTimeInfo.tempo / 60.L;
											/* offset in fractions of a second   */
		double dOffsetInSecond = dPos - floor(dPos);
		vstTimeInfo.smpteOffset = (long)(dOffsetInSecond *
									 fSmpteDiv[vstTimeInfo.smpteFrameRate] *
									 80.L);
	}


}; // VstHost


Item * createVstHost(const char * n, int t, int nInputs,int ma)
{
	PROFILE;
	int result = 0;
	Item * it = new VstHost(n,t,nInputs,ma,result);
	if ( ! result )
	{
		delete it;
		it = 0;
	}
	return it;
}


bool setVstHostMapping( Item * it, int param, int con )
{
	VstHost * vst = (VstHost*) it;
	if ( con<0 || con>=vst->ncon )
		return 0;
	
	vst->map_p[ con ]  = param;
	return 1;
}



//-------------------------------------------------------------------------------------------------------
static bool checkPlatform ()
{
//#if VST_64BIT_PLATFORM
//	printf ("*** This is a 64 Bit Build! ***\n");
//#else
//	printf ("*** This is a 32 Bit Build! ***\n");
//#endif
//
	int sizeOfVstIntPtr = sizeof(VstIntPtr);
	int sizeOfVstInt32 = sizeof(VstInt32);
	int sizeOfPointer = sizeof(void*);
	int sizeOfAEffect = sizeof(AEffect);
	
	//printf ("VstIntPtr = %d Bytes, VstInt32 = %d Bytes, Pointer = %d Bytes, AEffect = %d Bytes\n\n",
	//		sizeOfVstIntPtr, sizeOfVstInt32, sizeOfPointer, sizeOfAEffect);

	return sizeOfVstIntPtr == sizeOfPointer;
}
//-------------------------------------------------------------------------------------------------------
void checkEffectProperties (AEffect* effect)
{
	printf ("HOST> Gathering properties...\n");

	char effectName[256] = {0};
	char vendorString[256] = {0};
	char productString[256] = {0};

	effect->dispatcher (effect, effGetEffectName, 0, 0, effectName, 0);
	effect->dispatcher (effect, effGetVendorString, 0, 0, vendorString, 0);
	effect->dispatcher (effect, effGetProductString, 0, 0, productString, 0);

	printf ("Name = %s\nVendor = %s\nProduct = %s\n\n", effectName, vendorString, productString);

	printf ("numPrograms = %d\nnumParams = %d\nnumInputs = %d\nnumOutputs = %d\n\n", 
			effect->numPrograms, effect->numParams, effect->numInputs, effect->numOutputs);

	//// Iterate programs...
	//for(VstInt32 progIndex = 0; progIndex < effect->numPrograms; progIndex++)
	//{
	//	char progName[256] = {0};
	//	if(!effect->dispatcher (effect, effGetProgramNameIndexed, progIndex, 0, progName, 0))
	//	{
	//		effect->dispatcher (effect, effSetProgram, 0, progIndex, 0, 0); // Note: old program not restored here!
	//		effect->dispatcher (effect, effGetProgramName, 0, 0, progName, 0);
	//	}
	//	printf ("Program %03d: %s\n", progIndex, progName);
	//}

	//printf ("\n");

	// Iterate parameters...
	for(VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++)
	{
		char paramName[256] = {0};
		char paramLabel[256] = {0};
		char paramDisplay[256] = {0};

		effect->dispatcher (effect, effGetParamName, paramIndex, 0, paramName, 0);
		effect->dispatcher (effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
		effect->dispatcher (effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
		//effect->setParameter (effect, paramIndex, 0.3f);
		float value = effect->getParameter (effect, paramIndex);

		printf ("Param %03d: %-10s (%6.4f)\n", paramIndex, paramName, value);
	}

	printf ("\n");

	// Can-do nonsense...
	static const char* canDos[] =
	{
		"receiveVstEvents",
		"receiveVstMidiEvent",
		"midiProgramNames"
	};

	for(VstInt32 canDoIndex = 0; canDoIndex < sizeof(canDos)/sizeof(canDos[0]); canDoIndex++)
	{
		printf ("Can do %s... ", canDos[canDoIndex]);
		VstInt32 result = (VstInt32)effect->dispatcher (effect, effCanDo, 0, 0, (void*)canDos[canDoIndex], 0);
		switch(result)
		{
		case 0  : printf ("don't know"); break;
		case 1  : printf ("yes"); break;
		case -1 : printf ("definitely not!"); break;
		default : printf ("?????");
		}
		printf ("\n");
	}

	printf ("\n");
}


//-------------------------------------------------------------------------------------------------------
VstIntPtr VSTCALLBACK HostCallback (AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	PROFILE;
	VstIntPtr result = 0;

	static bool once[64] = {0};
	bool filtered = false;
	switch( opcode )
	{
		case audioMasterVersion : // 1
			result = kVstVersion;
			break;

		case audioMasterGetTime: // 7
			return VstIntPtr(&vstTimeInfo);

		//// Filter idle calls...
		case audioMasterUpdateDisplay: // 42
		case audioMasterIdle: // 3
		case 14: // audioMasterNeedIdle:
		case 6:  // audioMasterWantMidi:
		case 8:  // audioMasterProcessEvents
		case 10: // audioMasterTempoAt
		{
			if(once[opcode])
				filtered = true;
			break;
		}
	}
	once[opcode] = true;

	if(!filtered)
		printf ("PLUG> HostCallback (opcode %d)\n index = %d, value = %p, ptr = %p, opt = %f\n", opcode, index, FromVstPtr<void> (value), ptr, opt);

	return result;
}

