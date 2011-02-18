//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2005/11/15 15:14:03 $
//
// Category     : VST 2.x SDK Samples
// Filename     : again.cpp
// Created by   : Steinberg Media Technologies
// Description  : Stereo plugin which applies Gain [-oo, 0dB]
//
// © 2005, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include "again.h"
#include "..\..\P4P1Synth\source\UdpLog.h"
#include <stdio.h>

namespace Band
{
	const double base_coeffs[11] = 
	{
			8.11044e-07,
			8.943665402,    -36.83889529,    92.01697887,    -154.337906,    181.6233289,
			-151.8651235,   89.09614114,    -35.10298511,    8.388101016,    -0.923313471  ///A
	};

	const double base_factors[11] = 
	{
			1.0e-5,
			1.0e-6,   1.0e-6,    1.0e-6,    1.0e-6,    1.0e-6,
			1.0e-6,   1.0e-6,    1.0e-6,    1.0e-6,    1.0e-6
	};

	struct TheFilter
	{
		double memory[10];
		double coeff[11];

		TheFilter()
		{
			reset();
		}

		void reset()
		{
			memset(memory,0,10*sizeof(double));
			memcpy(coeff,base_coeffs,11*sizeof(double));;
		}

		float value(float in)
		{
			double res	=  
			( 
				 coeff[0]  * in +
				 coeff[1]  * memory[0] +  
				 coeff[2]  * memory[1] +
				 coeff[3]  * memory[2] +
				 coeff[4]  * memory[3] +
				 coeff[5]  * memory[4] +
				 coeff[6]  * memory[5] +
				 coeff[7]  * memory[6] +
				 coeff[8]  * memory[7] +
				 coeff[9]  * memory[8] +
				 coeff[10] * memory[9] 
			);

			//if ( res > 1.0 ) res = 1.0;
			//if ( res < -1.0 ) res = -1.0;

			memory[9]= memory[8];
			memory[8]= memory[7];
			memory[7]= memory[6];
			memory[6]= memory[5];
			memory[5]= memory[4];
			memory[4]= memory[3];
			memory[3]= memory[2];
			memory[2]= memory[1];                    
			memory[1]= memory[0];
			memory[0]=(double) res;
			return (float)res;
		}
	} filtor;

	void set( int vnum, float val )
	{
		if ( vnum==0&&val==0) filtor.reset();
		val = 2.0 * val - 1.0;
		val *= base_factors[vnum];
		filtor.coeff[vnum] = base_coeffs[vnum] + val;
		UdpLog::printf("coeff[%d %f]=%f",vnum,val,filtor.coeff[vnum] );
	}

	float get( int vnum )
	{
		float val = filtor.coeff[vnum] - base_coeffs[vnum];
		val /= base_factors[vnum];
		val = 0.5 + 0.5 * val;
		if ( val > 1 ) val = 1;
		if ( val < -1 ) val = -1;
		return val;
//		UdpLog::printf("coeff[%d]=%f",vnum,val );
	}

	float process( float sound )
	{
		return filtor.value(sound);
	}
}


//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new AGain (audioMaster);
}

//-------------------------------------------------------------------------------------------------------
AGain::AGain (audioMasterCallback audioMaster)
: AudioEffectX (audioMaster, 1, 11)	// 1 program, 1 parameter only
{
	setNumInputs (2);		// stereo in
	setNumOutputs (2);		// stereo out
	setUniqueID ('Band');	// identify
	canProcessReplacing ();	// supports replacing output
	canDoubleReplacing ();	// supports double precision processing

	fGain = 1.f;			// default to 0 dB
	vst_strncpy (programName, "Default", kVstMaxProgNameLen);	// default program name
	UdpLog::setup( 7777, "localhost" );
}

//-------------------------------------------------------------------------------------------------------
AGain::~AGain ()
{
	// nothing to do here
}

//-------------------------------------------------------------------------------------------------------
void AGain::setProgramName (char* name)
{
	vst_strncpy (programName, name, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void AGain::getProgramName (char* name)
{
	vst_strncpy (name, programName, kVstMaxProgNameLen);
}

//-----------------------------------------------------------------------------------------
void AGain::setParameter (VstInt32 index, float value)
{
	Band::set(index, value);
}

//-----------------------------------------------------------------------------------------
float AGain::getParameter (VstInt32 index)
{
	return Band::get(index);
}

//-----------------------------------------------------------------------------------------
void AGain::getParameterName (VstInt32 index, char* label)
{
	sprintf(label,"band %d",index);
}

//-----------------------------------------------------------------------------------------
void AGain::getParameterDisplay (VstInt32 index, char* text)
{
	float2string (Band::filtor.coeff[index], text, kVstMaxParamStrLen);
}

//-----------------------------------------------------------------------------------------
void AGain::getParameterLabel (VstInt32 index, char* label)
{
	//vst_strncpy (label, "dB", kVstMaxParamStrLen);
}

//------------------------------------------------------------------------
bool AGain::getEffectName (char* name)
{
	vst_strncpy (name, "BandFilter", kVstMaxEffectNameLen);
	return true;
}

//------------------------------------------------------------------------
bool AGain::getProductString (char* text)
{
	vst_strncpy (text, "BandFilter", kVstMaxProductStrLen);
	return true;
}

//------------------------------------------------------------------------
bool AGain::getVendorString (char* text)
{
	vst_strncpy (text, "BandFilter", kVstMaxVendorStrLen);
	return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 AGain::getVendorVersion ()
{ 
	return 1000; 
}

//-----------------------------------------------------------------------------------------
void AGain::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
    float* in1  =  inputs[0];
    float* in2  =  inputs[1];
    float* out1 = outputs[0];
    float* out2 = outputs[1];

	float s;
    while (--sampleFrames >= 0)
    {
		s = Band::process (*in1++);
        (*out1++) = s * fGain;
		//s = Band::process (*in2++);
        (*out2++) = s * fGain;
    }
}

//-----------------------------------------------------------------------------------------
void AGain::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
    double* in1  =  inputs[0];
    double* in2  =  inputs[1];
    double* out1 = outputs[0];
    double* out2 = outputs[1];
	double dGain = fGain;

    while (--sampleFrames >= 0)
    {
        (*out1++) = (*in1++) * dGain;
        (*out2++) = (*in2++) * dGain;
    }
}