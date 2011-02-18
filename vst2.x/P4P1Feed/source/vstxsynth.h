
#ifndef __vstxsynth__
#define __vstxsynth__

#include "public.sdk/source/vst2.x/audioeffectx.h"
#include "ADSR.h"




//------------------------------------------------------------------------------------------
enum FeedProps
{
	// Global
	kNumOutputs = 2,
	kNumPrograms = 128
};

enum FeedParams
{
	// Parameters Tags
	kVolume = 0,
	kWaveform1,

	kPhase,
	kPhaseFreq,

	kNoise,

	kFeed,

	kFreq1,
	kVolume1,
	kFreq2,
	kVolume2,

	kLfoTime,
	kLfoSpot,
	
	kEnvA,
	kEnvD,
	kEnvS,
	kEnvR,

	kLpCut,
	kLpRes,
	kLpAmount,
	kLfoAmount,

	kNumParams
};

//------------------------------------------------------------------------------------------
// VstXSynthProgram
//------------------------------------------------------------------------------------------
class VstXSynthProgram
{
friend class VstXSynth;
public:
	VstXSynthProgram ();
	~VstXSynthProgram () {}

private:
	float param[kNumParams];
	
	char name[kVstMaxProgNameLen+1];
};

//------------------------------------------------------------------------------------------
// VstXSynth
//------------------------------------------------------------------------------------------
class VstXSynth : public AudioEffectX
{
public:
	VstXSynth (audioMasterCallback audioMaster);
	~VstXSynth ();

	virtual void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual VstInt32 processEvents (VstEvents* events);

	virtual void setProgram (VstInt32 program);
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);
	virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);
	
	virtual void setSampleRate (float sampleRate);
	virtual void setBlockSize (VstInt32 blockSize);
	
	virtual bool getOutputProperties (VstInt32 index, VstPinProperties* properties);
		
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();
	virtual VstInt32 canDo (char* text);

	virtual VstInt32 getNumMidiInputChannels ();
	virtual VstInt32 getNumMidiOutputChannels ();


private:
	float fPhaseWav, fPhaseCarr, fPhaseFeed, fPhaseNoise;
	float fScaler;

	ADSR adsr;

	VstXSynthProgram* programs;
	VstInt32 channelPrograms[16];

	VstInt32 currentNote;
	VstInt32 currentVelocity;
	VstInt32 currentDelta;

	void initProcess ();
	void recalcNoise ( VstInt32 delta );
	void noteOn (VstInt32 note, VstInt32 velocity, VstInt32 delta);
	void noteOff ();
	void fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn);
};

#endif
