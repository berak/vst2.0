//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/01/09 16:02:36 $
//
// Category     : VST 2.x SDK Samples
// Filename     : vstxsynth.h
// Created by   : Steinberg Media Technologies
// Description  : Example VstXSynth
//
// A simple 2 oscillators test 'synth',
// Each oscillator has waveform, frequency, and volume
//
// *very* basic monophonic 'synth' example. you should not attempt to use this
// example 'algorithm' to start a serious virtual instrument; it is intended to demonstrate
// how VstEvents ('MIDI') are handled, but not how a virtual analog synth works.
// there are numerous much better examples on the web which show how to deal with
// bandlimited waveforms etc.
//
// © 2005, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#ifndef __vstxsynth__
#define __vstxsynth__

#include "public.sdk/source/vst2.x/audioeffectx.h"




//------------------------------------------------------------------------------------------
//
//  4 point catmul-rom spline:
//     A-_
//    /   -B_
//   /       -
//  *         -*
//  |----------|  T
//  since start & end are 0, it boils down to 3 params, the levels of the inner points and time
//
//------------------------------------------------------------------------------------------

struct Envelope
{
	float length; // in samples
	float pos; // in samples
	float ratio;
	float amount;

	Envelope() 
		: length(1000)
		, pos(1001)
		, ratio(0.5f)
		, amount(1.0f)
	{
	}
	void setAmount( float a )
	{
		amount = a;
	}
	void setRatio( float r )
	{
		ratio = r;
	}
	void setLen( float len ) // in samples
	{
		pos = length = len;
	}
	void step( float smp ) // in samples
	{
		pos += smp;
	}
	void trigger(bool on=true)
	{
		if ( on )
		{
			pos = 0;
		}
	}

	float value()
	{
		float u = pos/length;
		if ( u >= 1.0f ) 
			return 0;
		return value( u, 1.0f - ratio, ratio );
	}

	// u [0..1]
	static float value( float u, float A, float B )
	{
		float c1 = A*( 1.0f);
		float c2 =           +  B*( 0.5f);
		float c3 = A*(-2.5f) +  B*( 2.0f);
		float c4 = A*( 1.5f) +  B*(-1.5f);

		return (((c4*u + c3)*u +c2)*u + c1);
	}
};


//------------------------------------------------------------------------------------------
enum
{
	// Global
	kNumPrograms = 128,
	kNumOutputs = 2,

	// Parameters Tags
	kVolume = 0,
	kWaveform1,

	kWaveform2,

	kVowel,

	kFreq2,
	kVolume2,

	
	kEnvA,
	kEnvB,
	kEnvLen,

	kFreq1,
	kVolume1,


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
	float fWaveform1;
	float fFreq1;
	float fVolume1;

	float fWaveform2;
	float fFreq2;
	float fVolume2;

	float fEnvA;
	float fEnvB;
	float fEnvLen;
	float fVolume;
	float fVowel;
	
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

	virtual VstInt32 getMidiProgramName (VstInt32 channel, MidiProgramName* midiProgramName);
	virtual VstInt32 getCurrentMidiProgram (VstInt32 channel, MidiProgramName* currentProgram);
	virtual VstInt32 getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* category);
	virtual bool hasMidiProgramsChanged (VstInt32 channel);
	virtual bool getMidiKeyName (VstInt32 channel, MidiKeyName* keyName);

private:
	//VstXSynthProgram prog;
	//float fWaveform1;
	//float fFreq1;
	//float fVolume1;
	//float fWaveform2;
	//float fFreq2;
	//float fVolume2;
	//float fVolume;	
	//float fVowel;
	float fPhase1, fPhase2, fPhase3;
	float fScaler;

	Envelope env;

	VstXSynthProgram* programs;
	VstInt32 channelPrograms[16];

	VstInt32 currentNote;
	VstInt32 currentVelocity;
	VstInt32 currentDelta;
	bool noteIsOn;

	void initProcess ();
	void recalcNoise ( VstInt32 delta );
	void noteOn (VstInt32 note, VstInt32 velocity, VstInt32 delta);
	void noteOff ();
	void fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn);
};

#endif
