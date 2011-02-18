//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4		$Date: 2006/01/09 16:02:36 $
//-------------------------------------------------------------------------------------------------------

#include "Filter.h"
#include "vstxsynth.h"
#include <math.h>

#include <time.h> // for random
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "UdpLog.h"

enum
{
	kNumFrequencies = 128,	// 128 midi notes
	kWaveSize = 4096		// samples (must be power of 2 here)
};

const double midiScaler = (1. / 127.);
const float Pi = 3.141592653f;

//double midiTime = 0;
static float sawtooth[kWaveSize];
static float pulse[kWaveSize];
static float noise[kWaveSize];
static float sincos[kWaveSize];
static float freqtab[kNumFrequencies];

#ifndef RAND_MAX
#define RAND_MAX 0x7fffffff
#endif // RAND_MAX

template < class T > T  _random( T lo, T hi )
{
    static time_t t_init=0;
    if ( ! t_init ) srand( (unsigned int)time(&t_init) );
    double fval = (double)(rand() & RAND_MAX) / (double)RAND_MAX;
    return (T)(lo + (fval * (hi - lo)));
}

struct _BYE_BYE_ {	~_BYE_BYE_() { UdpLog::print("bye."); } } _bye_bye_;


//-----------------------------------------------------------------------------------------
// VstXSynth
//-----------------------------------------------------------------------------------------
void VstXSynth::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);
	fScaler = (float)((double)kWaveSize / (double)sampleRate);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::setBlockSize (VstInt32 blockSize)
{
	AudioEffectX::setBlockSize (blockSize);
	// you may need to have to do something here...
}

//-----------------------------------------------------------------------------------------
void VstXSynth::initProcess ()
{
	fPhase1 = fPhase2 = fPhase3 = 0.f;
	fScaler = (float)((double)kWaveSize / 44100.);	// we don't know the sample rate yet
	noteIsOn = false;
	currentDelta = currentNote = currentDelta = 0;
	VstInt32 i;

	UdpLog::setup( 7777, "127.0.0.1" );
	UdpLog::print( "hello, world!" );

	// make waveforms
	VstInt32 wh = kWaveSize / 4;	// 1:3 pulse
	for (i = 0; i < kWaveSize; i++)
	{
		sawtooth[i] = (float)(-1. + (2. * ((double)i / (double)kWaveSize)));
		pulse[i] = (i < wh) ? -1.f : 1.f;
		noise[i] = _random(-1.0f,1.0f);
		sincos[i] = cos( 2*Pi*(double)i / (double)kWaveSize );
	}
	//recalcNoise(77);
	// make frequency (Hz) table
	double k = 1.059463094359;	// 12th root of 2
	double a = 6.875;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (i = 0; i < kNumFrequencies; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}

	//Filter4Pole::set( fVolume2, fFreq2, 44100 );
	//FilterLP::set( fVolume2*10.0f, fFreq2 * 5000.0f, 44100 );
	//FilterBP::set( fVolume1, fFreq1, 44100 );

	env.setLen( 44100 );
	env.setAmount( 0.0f );
	env.setRatio( 0.1f );
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];
	bool isPlaying = noteIsOn;
	VstXSynthProgram &prog = programs[curProgram];

	float ev = env.value();
	isPlaying |= (ev != 0.0f);
	env.step( float(sampleFrames) );

//	midiTime += 0.001;
//	recalcNoise(11);
	if (isPlaying)
	{
		float baseFreq = freqtab[currentNote & 0x7f] * fScaler;
		float carrFreq =  baseFreq * 2* prog.fFreq1;
		float* wave1 = (prog.fWaveform1 < .5) ? sawtooth : pulse;
		float* wave2 = noise;//(fWaveform2 < .5) ? sawtooth : pulse;
		float volNoise = (float)(prog.fWaveform2);
		float vol = (float)( (double)currentVelocity * midiScaler ) * ev;
		float modScale = prog.fVolume1 * 60.0;
		//FilterBP::set( fVolume1, fFreq1 + (1000.0f*fFreq1*ev*env.amount), 44100 );
		FilterLP::set( prog.fVolume2*10 , 5000 * (prog.fFreq2 + (vol*env.amount)), 44100 );
		//FilterBessel::set( fVolume2 , 5000 * (fFreq2 + (vol*env.amount)), 44100 );
		//Filter4Pole::set( fVolume2 , (fFreq2 + (vol*env.amount)), 44100 );

		vol *= prog.fVolume;

		int v = (prog.fVowel*6) - 1;
		FilterFormant::set(v);

		if (currentDelta > 0)
		{
			if (currentDelta >= sampleFrames)	// future
			{
				currentDelta -= sampleFrames;
				return;
			}
			memset (out1, 0, currentDelta * sizeof (float));
			memset (out2, 0, currentDelta * sizeof (float));
			out1 += currentDelta;
			out2 += currentDelta;
			sampleFrames -= currentDelta;
			currentDelta = 0;
		}

		// loop
		while (--sampleFrames >= 0)
		{
			float w = wave1[(VstInt32)fPhase1]  * (1.0f-volNoise);
			float n = wave2[(VstInt32)fPhase2]  * (volNoise);
			int phase3 = (VstInt32)( fPhase3 *  ((0.5+0.5*w)*prog.fVolume1*15));
			//int phase3 = (VstInt32)( fPhase3 +  ((0.5+5.5*w)*prog.fVolume1 * kWaveSize));
			float c = sincos[phase3%kWaveSize];
			//float c = sincos[(VstInt32)(fPhase3)];
			float s = n + c;
			//s = FilterBP::process(s);
			if ( v>=0 )
				s = FilterFormant::process(s);
			s = FilterLP::process(s);
			//s = Filter4Pole::process(s);
			s = s * vol;
			(*out1++) = s;
			(*out2++) = s;

			fPhase1 += baseFreq;
			if ( fPhase1 >= kWaveSize ) fPhase1 -= kWaveSize;

			fPhase2 += 0.3f; // fixed freq optimized against aliasing
			if ( fPhase2 >= kWaveSize ) fPhase2 -= kWaveSize;

			fPhase3 += carrFreq ;//+ w*modScale;
			if ( fPhase3 >= kWaveSize ) fPhase3 -= kWaveSize;
		}
	}						
	else
	{
		memset (out1, 0, sampleFrames * sizeof (float));
		memset (out2, 0, sampleFrames * sizeof (float));
	}
}

//-----------------------------------------------------------------------------------------
VstInt32 VstXSynth::processEvents (VstEvents* ev)
{
	for (VstInt32 i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;

		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		VstInt32 status = midiData[0] & 0xf0;	// ignoring channel
		if (status == 0x90 || status == 0x80)	// we only look at notes
		{
			VstInt32 note = midiData[1] & 0x7f;
			VstInt32 velocity = midiData[2] & 0x7f;
			if (status == 0x80)
				velocity = 0;	// note off by velocity 0
			if (!velocity && (note == currentNote))
				noteOff ();
			else
				noteOn (note, velocity, event->deltaFrames);
		}
		else if (status == 0xb0)
		{
			if (midiData[1] == 0x7e || midiData[1] == 0x7b)	// all notes off
				noteOff ();
		}
		event++;
	}
	return 1;
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOn (VstInt32 note, VstInt32 velocity, VstInt32 delta)
{
	currentNote = note;
	currentVelocity = velocity;
	currentDelta = delta;
	noteIsOn = true;
	fPhase1 = fPhase2 = 0;
	env.trigger(true);
	char b[120];
	sprintf( b, "note %2x %2x",note,velocity );
	UdpLog::print( b );
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOff ()
{
	noteIsOn = false;
	env.trigger(false);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::recalcNoise ( VstInt32 delta )
{
	//for (int i = 0; i < kWaveSize; i++ )
	//{
	//	double t0 = (double)i / (double)kWaveSize;
	//	//noise[i] = _random(-fFreq2,fFreq2) + float( sin( fVolume1 * t0  + fFreq1 * 7.0f * noise[i] ) + fVolume2*cos(t0 + midiTime*sin(noise[i])) );
	//	noise[i] = (float)(-1. + (2. * t0));
	//	//noise[i] = _random(-fFreq2,fFreq2) + float( sin( fVolume1 * t0  + fFreq1 * 7.0f * noise[i] ) + fVolume2*cos(t0 + midiTime*sin(noise[i])) );
	//}
}
