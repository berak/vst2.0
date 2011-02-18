
#include "vstxsynth.h"


//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new VstXSynth (audioMaster);
}

//-----------------------------------------------------------------------------------------
// VstXSynthProgram
//-----------------------------------------------------------------------------------------
const char *FeedParamNames[] = 
{
	"Volume",
	"Base Wave",

	"Phase",
	"PhaseFreq",

	"Noise",

	"Feedback",
	"Tap 1",
	"-->1<--",
	"Tap 2",
	"-->2<--",

	"Lfo Time",
	"Lfo Spot",
	
	"Attack",
	"Decay",
	"Sustain",
	"Release",

	"Lp Cutoff",
	"Lp Reso",
	"Env Amount",
	"Lfo Amount",
	0
};

VstXSynthProgram::VstXSynthProgram ()
{
	// Default Program Values
	param[kVolume]    = 0.4f;
	param[kWaveform1] = 0.f;	// saw

	param[kFeed]      = .0f;
	param[kPhase]     = .0f;
	param[kPhaseFreq] = .5f;
	param[kFreq1]     = .5f;
	param[kVolume1]   = .5f;
	param[kFreq2]     = .5f;	
	param[kVolume2]   = .5f;

	param[kLfoTime]   = 0.3f;
	param[kLfoSpot]   = 0.3f;
	param[kLfoAmount] = 0.3f;

	param[kEnvA]      = 0.2f;
	param[kEnvD]      = 0.2f;
	param[kEnvS]      = 0.8f;
	param[kEnvR]      = 0.2f;
	
	param[kLpCut]     = 0.5f;
	param[kLpRes]     = 0.5f;
	param[kLpAmount]  = 0.5f;

	vst_strncpy (name, "Basic", kVstMaxProgNameLen);

}

//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterName (VstInt32 index, char* label)
{
	if ( index < kNumParams )
		vst_strncpy (label, FeedParamNames[index], kVstMaxProgNameLen);
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
		setUniqueID ('P2P1');			// <<<! *must* change this!!!!
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
	
	curProgram = program;
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
	label[0]=0;
}


//-----------------------------------------------------------------------------------------
void VstXSynth::getParameterDisplay (VstInt32 index, char* text)
{
	text[0] = 0;
	VstXSynthProgram &prog = programs[curProgram];
	switch (index)
	{
		case kWaveform1:
			if (prog.param[kWaveform1] < .5)
				vst_strncpy (text, "Sawtooth", kVstMaxParamStrLen);
			else
				vst_strncpy (text, "Pulse", kVstMaxParamStrLen);
			break;

		default:
			float2string (prog.param[index], text, kVstMaxParamStrLen);	break;
	}
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setParameter (VstInt32 index, float value)
{
	programs[curProgram].param[index] = value;	
}

//-----------------------------------------------------------------------------------------
float VstXSynth::getParameter (VstInt32 index)
{
	return programs[curProgram].param[index];
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
	vst_strncpy (name, "P4P1Feed", kVstMaxEffectNameLen);
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
	vst_strncpy (text, "p4p1 Feedback Synth", kVstMaxProductStrLen);
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
