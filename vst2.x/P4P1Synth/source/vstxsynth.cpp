//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/01/09 16:02:36 $
//
// Category     : VST 2.x SDK Samples
// Filename     : vstxsynth.cpp
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

#include "vstxsynth.h"
#include "gmnames.h"
//#include "Filter.h"

//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new VstXSynth (audioMaster);
}

//-----------------------------------------------------------------------------------------
// VstXSynthProgram
//-----------------------------------------------------------------------------------------
VstXSynthProgram::VstXSynthProgram ()
{
	// Default Program Values
	fWaveform1 = 0.f;	// saw
	fFreq1     = .03f;
	fVolume1   = .03f;

	fWaveform2 = .0f;	// silent
	fFreq2     = .5f;	
	fVolume2   = .13f;
	fVowel	   = 0.0f;
	fEnvA      = 0.2f;
	fEnvB      = 0.2f;
	fEnvLen    = 0.2f;

	fVolume    = .4f;
	vst_strncpy (name, "Basic", kVstMaxProgNameLen);

}

//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
VstXSynth::VstXSynth (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
{
	// initialize programs
	programs = new VstXSynthProgram[kNumPrograms];
	for (VstInt32 i = 0; i < 16; i++)
		channelPrograms[i] = i;

	if (programs)
		setProgram (0);
	
	if (audioMaster)
	{
		setNumInputs (0);				// no inputs
		setNumOutputs (kNumOutputs);	// 2 outputs, 1 for each oscillator
		canProcessReplacing ();
		isSynth ();
		setUniqueID ('P451');			// <<<! *must* change this!!!!
	}

	initProcess ();
	suspend ();
}

//-----------------------------------------------------------------------------------------
VstXSynth::~VstXSynth ()
{
	if (programs)
		delete[] programs;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgram (VstInt32 program)
{
	if (program < 0 || program >= kNumPrograms)
		return;
	
	VstXSynthProgram *ap = &programs[program];
	curProgram = program;
	//
	//fWaveform1 = ap->fWaveform1;
	//fFreq1     = ap->fFreq1;
	//fVolume1   = ap->fVolume1;

	//fWaveform2 = ap->fWaveform2;
	//fFreq2     = ap->fFreq2;
	//fVolume2   = ap->fVolume2;
	//
	//fVolume    = ap->fVolume;
	//fVowel     = ap->fVowel;
	//prog = *ap;
	env.setAmount( ap->fEnvA );
	env.setRatio( ap->fEnvB );
	env.setLen( ap->fEnvLen * 1000000 );
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setProgramName (char* name)
{
	vst_strncpy (programs[curProgram].name, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getProgramName (char* name)
{
	vst_strncpy (name, programs[curProgram].name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterLabel (VstInt32 index, char* label)
{
	switch (index)
	{
		//case kWaveform1:
		//case kWaveform2:
		//	vst_strncpy (label, "Shape", kVstMaxParamStrLen);
		//	break;

		//case kFreq1:
		//case kFreq2:
		//	vst_strncpy (label, "Hz", kVstMaxParamStrLen);
		//	break;

		//case kVolume1:
		//case kVolume2:
		//case kVolume:
		//	vst_strncpy (label, "dB", kVstMaxParamStrLen);
		//	break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterDisplay (VstInt32 index, char* text)
{
	text[0] = 0;
	VstXSynthProgram &prog = programs[curProgram];
	switch (index)
	{
		case kWaveform1:
			if (prog.fWaveform1 < .5)
				vst_strncpy (text, "Sawtooth", kVstMaxParamStrLen);
			else
				vst_strncpy (text, "Pulse", kVstMaxParamStrLen);
			break;

		case kFreq1:		float2string (prog.fFreq1, text, kVstMaxParamStrLen);	break;
		case kVolume1:		float2string (prog.fVolume1, text, kVstMaxParamStrLen);	break;	
		case kWaveform2:	float2string (prog.fWaveform2, text, kVstMaxParamStrLen);	break;
		case kFreq2:		float2string (prog.fFreq2, text, kVstMaxParamStrLen);	break;
		case kVolume2:		float2string (prog.fVolume2, text, kVstMaxParamStrLen);	break;
		case kVolume:		float2string (prog.fVolume, text, kVstMaxParamStrLen);	break;
		case kVowel:		float2string (prog.fVowel, text, kVstMaxParamStrLen);	break;
		case kEnvA:			float2string (prog.fEnvA, text, kVstMaxParamStrLen);	break;
		case kEnvB:			float2string (prog.fEnvB, text, kVstMaxParamStrLen);	break;
		case kEnvLen:		float2string (prog.fEnvLen, text, kVstMaxParamStrLen);	break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterName (VstInt32 index, char* label)
{
	switch (index)
	{
		case kWaveform1:	vst_strncpy (label, "Wave 1",  kVstMaxParamStrLen);	break;
		case kFreq1:		vst_strncpy (label, "Freq BP", kVstMaxParamStrLen);	break;
		case kVolume1:		vst_strncpy (label, "Levl BP", kVstMaxParamStrLen);	break;
		case kWaveform2:	vst_strncpy (label, "Noise",   kVstMaxParamStrLen);	break;
		case kFreq2:		vst_strncpy (label, "Freq LP", kVstMaxParamStrLen);	break;
		case kVolume2:		vst_strncpy (label, "Levl LP", kVstMaxParamStrLen);	break;
		case kVolume:		vst_strncpy (label, "Volume",  kVstMaxParamStrLen);	break;
		case kVowel:		vst_strncpy (label, "Vowel",  kVstMaxParamStrLen);	break;
		case kEnvA:			vst_strncpy (label, "Env Amount",   kVstMaxParamStrLen);	break;
		case kEnvB:			vst_strncpy (label, "Env Ratio",   kVstMaxParamStrLen);	break;
		case kEnvLen:		vst_strncpy (label, "Env Len", kVstMaxParamStrLen);	break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setParameter (VstInt32 index, float value)
{
	VstXSynthProgram &prog = programs[curProgram];
	switch (index)
	{
		case kWaveform1:	prog.fWaveform1	= value;	break;
		case kFreq1:		prog.fFreq1		= value;	break;
		case kVolume1:		prog.fVolume1		= value;	break;
		case kWaveform2:	prog.fWaveform2	= value;	break;
		case kFreq2:		prog.fFreq2		= value;	break;
		case kVolume2:		prog.fVolume2		= value;	break;
		case kVolume:		prog.fVolume		= value;	break;
		case kVowel:		prog.fVowel		= value;	break;
		case kEnvA:			prog.fEnvA	= value;	env.setAmount( prog.fEnvA ); break;
		case kEnvB:			prog.fEnvB	= value;	env.setRatio( prog.fEnvB ); break;
		case kEnvLen:		prog.fEnvLen = value;	env.setLen( prog.fEnvLen * 100000 ); break;
	}
}

//-----------------------------------------------------------------------------------------
float VstXSynth::getParameter (VstInt32 index)
{
	float value = 0;
	VstXSynthProgram &prog = programs[curProgram];
	switch (index)
	{
		case kWaveform1:	value = prog.fWaveform1;	break;
		case kFreq1:		value = prog.fFreq1;		break;
		case kVolume1:		value = prog.fVolume1;	break;
		case kWaveform2:	value = prog.fWaveform2;	break;
		case kFreq2:		value = prog.fFreq2;		break;
		case kVolume2:		value = prog.fVolume2;	break;
		case kVolume:		value = prog.fVolume;	break;
		case kVowel:		value = prog.fVowel;		break;
		case kEnvA:			value = prog.fEnvA;	break;
		case kEnvB:			value = prog.fEnvB;	break;
		case kEnvLen:		value = prog.fEnvLen;	break;
	}
	return value;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	if (index < kNumOutputs)
	{
		vst_strncpy (properties->label, "Vstx ", 63);
		char temp[11] = {0};
		int2string (index + 1, temp, 10);
		vst_strncat (properties->label, temp, 63);

		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		vst_strncpy (text, programs[index].name, kVstMaxProgNameLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getEffectName (char* name)
{
	vst_strncpy (name, "P4P1Synth", kVstMaxEffectNameLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getVendorString (char* text)
{
	vst_strncpy (text, "Ein Zwerg nologies", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
bool VstXSynth::getProductString (char* text)
{
	vst_strncpy (text, "p4p1 Synth", kVstMaxProductStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getVendorVersion ()
{ 
	return 1000; 
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp (text, "midiProgramNames"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiInputChannels ()
{
	return 1; // we are monophonic
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::getNumMidiOutputChannels ()
{
	return 0; // no MIDI output back to Host app
}

// midi program names:
// as an example, GM names are used here. in fact, VstXSynth doesn't even support
// multi-timbral operation so it's really just for demonstration.
// a 'real' instrument would have a number of voices which use the
// programs[channelProgram[channel]] parameters when it receives
// a note on message.

//------------------------------------------------------------------------
VstInt32 VstXSynth::getMidiProgramName (VstInt32 channel, MidiProgramName* mpn)
{
	VstInt32 prg = mpn->thisProgramIndex;
	if (prg < 0 || prg >= 128)
		return 0;
	fillProgram (channel, prg, mpn);
	if (channel == 9)
		return 1;
	return 128L;
}

//------------------------------------------------------------------------
VstInt32 VstXSynth::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || !mpn)
		return -1;
	VstInt32 prg = channelPrograms[channel];
	mpn->thisProgramIndex = prg;
	fillProgram (channel, prg, mpn);
	return prg;
}

//------------------------------------------------------------------------
void VstXSynth::fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn)
{
	mpn->midiBankMsb = -1;
	mpn->midiBankLsb = -1;
	mpn->reserved = 0;
	mpn->flags = 0;

	if (channel == 9)	// drums
	{
		vst_strncpy (mpn->name, "Standard", 63);
		mpn->midiProgram = 0;
		mpn->parentCategoryIndex = 0;
	}
	else
	{
		vst_strncpy (mpn->name, GmNames[prg], 63);
		mpn->midiProgram = (char)prg;
		mpn->parentCategoryIndex = -1;	// for now

		for (VstInt32 i = 0; i < kNumGmCategories; i++)
		{
			if (prg >= GmCategoriesFirstIndices[i] && prg < GmCategoriesFirstIndices[i + 1])
			{
				mpn->parentCategoryIndex = i;
				break;
			}
		}
	}
}

//------------------------------------------------------------------------
VstInt32 VstXSynth::getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* cat)
{
	cat->parentCategoryIndex = -1;	// -1:no parent category
	cat->flags = 0;					// reserved, none defined yet, zero.
	VstInt32 category = cat->thisCategoryIndex;
	if (channel == 9)
	{
		vst_strncpy (cat->name, "Drums", 63);
		return 1;
	}
	if (category >= 0 && category < kNumGmCategories)
		vst_strncpy (cat->name, GmCategories[category], 63);
	else
		cat->name[0] = 0;
	return kNumGmCategories;
}

//------------------------------------------------------------------------
bool VstXSynth::hasMidiProgramsChanged (VstInt32 channel)
{
	return false;	// updateDisplay ()
}

//------------------------------------------------------------------------
bool VstXSynth::getMidiKeyName (VstInt32 channel, MidiKeyName* key)
								// struct will be filled with information for 'thisProgramIndex' and 'thisKeyNumber'
								// if keyName is "" the standard name of the key will be displayed.
								// if false is returned, no MidiKeyNames defined for 'thisProgramIndex'.
{
	// key->thisProgramIndex;		// >= 0. fill struct for this program index.
	// key->thisKeyNumber;			// 0 - 127. fill struct for this key number.
	key->keyName[0] = 0;
	key->reserved = 0;				// zero
	key->flags = 0;					// reserved, none defined yet, zero.
	return false;
}


