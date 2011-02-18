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
struct _BYE_BYE_ {	~_BYE_BYE_() { UdpLog::print("Feed:bye."); } } _bye_bye_;

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
static float delay[kWaveSize];
static float noise[kWaveSize];
static float sincos[kWaveSize];
static float freqtab[kNumFrequencies];

#ifndef RAND_MAX
#define RAND_MAX 0x7fffffff
#endif // RAND_MAX


inline float saturate(float input, float fMax=1.0f)
{
	static const float fGrdDiv = 0.5f;
	float x1 = fabsf(input + fMax);
	float x2 = fabsf(input - fMax);
	return fGrdDiv * (x1 - x2);
}

template < class T > T  _random( T lo, T hi )
{
    static time_t t_init=0;
    if ( ! t_init ) srand( (unsigned int)time(&t_init) );
    double fval = (double)(rand() & RAND_MAX) / (double)RAND_MAX;
    return (T)(lo + (fval * (hi - lo)));
}


class LfoTri 
{
	float T,Ts,tic,d0,d1;
	enum { tmul = 12 };
public:

	float level; 


	LfoTri()
	{
		level = tic = T = Ts = d0 = d1 = 0.0f; 
	}

	inline float tick(float frames) 
	{
		tic += frames/(44100.0f*tmul);
        if ( tic >= T )
			tic  -= T;
        level = (tic<Ts) ? tic*d0 : 1-(tic-Ts)*d1 ;
		return level;
    }

    void set( float t, float spot ) 
	{ 
		t = (t>0.06f) ? t : 0.06f;
        T = t;
        Ts = T * spot;
        d0 = 1.0f / Ts;
        d1 = 1.0f / (T-Ts);
	}
} lfo1;


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
}

//-----------------------------------------------------------------------------------------
void VstXSynth::initProcess ()
{
	fPhaseWav = fPhaseCarr = fPhaseFeed = fPhaseNoise = 0.0f;
	fScaler = (float)((double)kWaveSize / 44100.);	// we don't know the sample rate yet

	currentDelta = currentNote = curProgram = 0;
	VstInt32 i;

	UdpLog::setup( 7777, "127.0.0.1" );
	UdpLog::print( "Fred!" );

	// make waveforms
	VstInt32 wh = kWaveSize / 4;	// 1:3 pulse
	for (i = 0; i < kWaveSize; i++)
	{
		sawtooth[i] = (float)(-1. + (2. * ((double)i / (double)kWaveSize)));
		pulse[i] = (i < wh) ? -1.f : 1.f;
		delay[i] = 0;
		noise[i] = _random(-1.0f,1.0f);
		sincos[i] = (float)cos( 2.0 * (double)Pi * (double)i / (double)kWaveSize );
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

	adsr.setSampleRate(44100.0f);
}

//-----------------------------------------------------------------------------------------
void VstXSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	VstXSynthProgram &prog = programs[curProgram];

	lfo1.set( prog.param[kLfoTime], prog.param[kLfoSpot] );
	float lfoVal = prog.param[kLfoAmount] * lfo1.tick((float)sampleFrames);

	adsr.setAllTimes( prog.param[kEnvA]*0.001f, prog.param[kEnvD]*0.001f, prog.param[kEnvS], prog.param[kEnvR]*0.001f );

	if (adsr.getState() != ADSR::DONE)
	{
		float* wave1 = (prog.param[kWaveform1] < .5) ? sawtooth : pulse;

		float baseFreq = freqtab[currentNote & 0x7f] * fScaler;
		float carrFreq = baseFreq * 2 * prog.param[kPhaseFreq];
		float ev       = adsr.tick();
		float vol      = (float)( (double)currentVelocity * midiScaler ) * ev;
		float vol_s    = prog.param[kPhase];
		float vol_p    = (1.0f - prog.param[kPhase]);
		float tap1     = (0.5f+0.5f*prog.param[kFreq1]) * kWaveSize;
		float tap2     = (0.5f+0.5f*prog.param[kFreq2]) * kWaveSize;
		float gain1    = 5 * (2.0f * prog.param[kVolume1] - 1.0f);
		float gain2    = 5 * (2.0f * prog.param[kVolume2] - 1.0f);
		FilterLP::set( prog.param[kLpRes] * 10 , (5000) *( prog.param[kLpCut]+ (vol*prog.param[kLpAmount]) + lfoVal ), 44100 );

		// handle latencies
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
			float s = wave1[(VstInt32)fPhaseWav];
			
			int phase4 = (VstInt32)( fPhaseCarr * ((0.5+0.5*s)*prog.param[kPhaseFreq]*15)) % kWaveSize;
			float c =  sincos[phase4];

			// collect feedback
			float feed = 0.0f;
			int phase1 = (VstInt32)(fPhaseFeed + tap1);
			feed += delay[phase1%kWaveSize] * gain1;

			int phase2 = (VstInt32)(fPhaseFeed + tap2);
			feed += delay[phase2%kWaveSize] * gain2;

			// mix
			float sum = 0.0f;
			sum += vol_s * s + vol_p * c;
			sum += noise[(VstInt32)(fPhaseNoise)] * prog.param[kNoise];
			sum += feed * prog.param[kFeed];
			sum = FilterLP::process( sum );
			sum = saturate(vol * sum);

			// save to delay
			delay[(VstInt32)fPhaseFeed] = sum;

			// main vol should NOT contribute to feedback
			sum *= prog.param[kVolume]; 
			(*out1++) = sum;
			(*out2++) = sum;

			fPhaseWav += baseFreq;
			if ( fPhaseWav >= kWaveSize ) fPhaseWav -= kWaveSize;

			fPhaseCarr += carrFreq;
			if ( fPhaseCarr >= kWaveSize ) fPhaseCarr -= kWaveSize;

			fPhaseFeed += 1.0f;
			if ( fPhaseFeed >= kWaveSize ) fPhaseFeed = 0;

			fPhaseNoise+=0.3f;
			if ( fPhaseNoise >= kWaveSize ) fPhaseNoise = 0;
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
	//noteIsOn = true;
	fPhaseCarr = fPhaseNoise = fPhaseWav = fPhaseFeed = 0;
	adsr.keyOn();
	char b[120];
	sprintf( b, "note %2x %2x",note,velocity );
	UdpLog::print( b );
}

//-----------------------------------------------------------------------------------------
void VstXSynth::noteOff ()
{
	//noteIsOn = false;
	adsr.keyOff();
}

//-----------------------------------------------------------------------------------------
void VstXSynth::recalcNoise ( VstInt32 delta )
{
}
