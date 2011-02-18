
#ifndef __p_analog__
#define __p_analog__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
//#include "public.sdk/source/vst2.x/audioeffectx.h"
#include "audioeffectx.h"
//#include "vstgui.sf/vstgui/vstgui.h"
#include "vstgui.h"

//#define MB(x)       MessageBox(NULL,x,":)",MB_OK)

// whot, - no pi ?
const float Pi = 3.141592653f;

// resource id's
enum {
    // bitmaps
    kBackgroundId = 128,
    kparBGId,
    kFaderHandleId,
    kKnobHandleId,
    kKnobBgId,
    kWaveId,
    kFilId,
    kMatId,
    kLfoId,
};

// constants :
enum {
    kNumPrograms     = 19,
    kNumVoices       = 4,
    kNumWaves        = 5,
    kNumSwitches     = 9,
    kNumOutputs      = 2,
    kNumFrequencies  = 128,  // 128 midi notes
    kWaveSize        = 4096, // samples (must be power of 2 here)
    kfMul            = 16,    // octaves#pitch
    kFilCenter       = 8000,
    kFilWidth        = 3000,
    kLfoTicks        = 1500
};

// params:
enum {
    kWave0 = 0,
    kVol0,
    kFreq0,
    kWspot0,

    kAttack0,
    kDecay0,
    kSustain0,
    kRelease0,

    kWave1,
    kVol1,
    kFreq1,
    kWspot1,

    kAttack1,
    kDecay1,
    kSustain1,
    kRelease1,

    kFilFreq0,
    kFilFac0,
//    kFilTyp0,

    kFilFreq1,
    kFilFac1,
//    kFilTyp1,

    kVolume,    
    kPan,
    kMod,

    kLfoFreq0,
    kLfoFac0,
    kLfoFreq1,
    kLfoFac1,
    kLfoFreq2,
    kLfoFac2,
    // switch-matrix :
    kSKey,
    kSVel,
    kSPan,
    kSExp,
    kSLfo0,
    kSLfo1,
    kSLfo2,
    kSEnv0,
    kSEnv1,

    kPotiName,
    kStrip,
    kNControls,
    kNumParams = kNControls-2,
    kNRealPar  = kNumParams-kNumSwitches
};

//
// 2nd order latfir bandpass, orig. from 'sox':
// y(t) = A * x(t) - B * y(t-1) - C * y(t-2);
//
class Filter {
    static enum  { 
        kCMax  = 8000,     // max. centerfreq.  
        kSRate = 44100     // est. samplefreq.
    }; 
protected:
	float A, B, C, v0, v1, v2, om;
public:
    Filter() { om=2*Pi*kCMax/kSRate; stop(); }
    void stop() {       
        v0=v1=v2=0.0f;
        set(0.0f,0.0f);
    }
    void set( float c, float w ) {
        float center = om*c*c;
        float width  = om*(1-w*w); 
        C = expf(-width);
         float D = 1+C; // helper
        B = -4*C/D*cosf(center);
        A = sqrtf((D*D-B*B)*(1-C)/D); // 'noizy' version
    }
    inline float value( float in ) {
    	v0 = A * in - B * v1 - C * v2; 
        v2 = v1; v1 = v0;
        return v0;
    }
};

//
// speedup:
// set filter-state from table:
//
class Filter2 : public Filter {
    static enum  {
        kMax = 64   // size of table (8k+64) -> 6bit rez.
    }; 
    float a[kMax][kMax], b[kMax][kMax], c[kMax]; // will replace A & B & C
public:
    Filter2() : Filter() { 
        float mx = 1.0f/kMax;
        for ( short w=0; w<kMax; w++ ) {
            float cw = w * mx;
            for ( short f=0; f<kMax; f++ ) {
                Filter::set( f*mx, cw );
                a[w][f] = A; // copy coeffs:
                b[w][f] = B;
            }
            c[w] = C; // C is fixed for any freq.
        }
    }
    inline void set( float f, float w ) {
         short freq  = short( (kMax-1) * f );
         short width = short( (kMax-1) * w );
         A = a[width][freq]; // copy back:
         B = b[width][freq];
         C = c[width];
    }
};


class p_analogVoice {
public:
    bool  active, hold;
    float phase[2], a_phase[2], a_level[2];
    long  note, velocity;
    float noteScaled, velocityScaled;
    p_analogVoice() { stop(); }
    float stop() {
        active=hold=false; 
        note=velocity=0; 
        phase[0]=a_phase[0]=a_level[0]=phase[1]=a_phase[1]=a_level[1]=noteScaled=velocityScaled=0.0f;
        return 0.0f;
    }
    void start() { stop();active=hold=true;  }
};


class p_adsr {
    float a,d,s,r;
    float m,M,Ta,Td,Tr;
    float dA,dD,dR;
    void calc() {
        M  = 80000.0f; m=200.0f;
        Ta = a*a * M + m;
        Td = Ta + d*d * M + m;
        Tr = Td + r*r * M + m;
        dA = 1.0f/( Ta );
        dD = (1.0f-s) / ( Td - Ta );
        dR = s / ( Tr - Td );
    }
public:
    p_adsr() {a=d=s=r=0.0f; calc(); }
    void set_a(float f) { a=f; calc(); }
    void set_d(float f) { d=f; calc(); }
    void set_r(float f) { r=f; calc(); }
    void set_s(float f) { s=f; calc(); }
    inline float val( p_analogVoice * av, int o ) {
        if ( av->hold ) {
            if  (av->a_phase[o]<Ta)  {
                av->a_phase[o]++; return ( av->a_level[o] += dA );
            } else if (av->a_phase[o]<Td) {
                av->a_phase[o]++; return ( av->a_level[o] -= dD );
            } else {
                return( av->a_level[o] = s );
            }
        } else if ( av->a_phase[o]<Tr ) {
            av->a_phase[o]++; return ( av->a_level[o] -= dR );
        } 
        if ((av->a_phase[0]>=Tr)&&(av->a_phase[1]>=Tr)){
            return av->stop();
        }
        return 0.0f;
    }
};

class p_lfo {
    float T,Ts,tic,d0,d1,gspot;
public:
    float level; 
    p_lfo()     { level=tic=T=Ts=d0=d1=gspot=0.0f; }
    inline void tick() {
        if (tic>T)  tic  = level = 0.0f;
        else        tic += kLfoTicks;
//        level += (tic<Ts) ? d0 : -d1;
        level = (tic<Ts) ? tic*d0 : 1-(tic-Ts)*d1 ;
    }
    void setTime( float t ) { T=t*t*1500000.0f;start(); }
    void setSpot( float t ) { gspot=t>0?t:0.06f;start(); }
    void start() {
        level=tic=0.0f;
        T=T>200?T:200.0f;
        Ts=T*gspot;
        d0=1.0f/Ts;
        d1=1.0f/(T-Ts);
    }
};


class p_analogWave {
public:
    float  data[4096];
    char   name[24];
};

class p_analogProgram {
friend class p_analog;
public:
    p_analogProgram();
    ~p_analogProgram() {}
private:
    float volume;
    float modulation;
    float pan;

    float wave[2];
    float wspot[2];
    float freq[2];
    float vol[2];

    float attack[2];
    float decay[2];
    float sustain[2];
    float release[2];

    float lfoFreq[3];
    float lfoFac[3];

    float filFreq[2];
    float filFac[2];
//    float filTyp[2];
    float matrix[kNRealPar][kNumSwitches];
    char  name[24];
};

//------------------------------------------------------------------------------------------
class p_analog : public AudioEffectX, p_analogProgram {
public:
    p_analog(audioMasterCallback audioMaster);
    ~p_analog();

    virtual void process(float** inputs, float** outputs, VstInt32 sampleframes);
    virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleframes);
    virtual VstInt32 processEvents(VstEvents* events);

    virtual void setProgram(VstInt32 program);
    virtual void setProgramName(char* name);
    virtual void getProgramName(char* name);
    virtual void setParameter(VstInt32 index, float value);
    virtual float getParameter(VstInt32 index);
    virtual void getParameterLabel(VstInt32 index, char* label);
    virtual void getParameterDisplay(VstInt32 index, char* text);
    virtual void getParameterName(VstInt32 index, char* text);
    virtual void setSampleRate(float sampleRate);
    virtual void setBlockSize(long blockSize);
    virtual void resume();

    virtual bool getOutputProperties (long index, VstPinProperties* properties);
    virtual bool getProgramNameIndexed (long category, long index, char* text);
    virtual bool copyProgram (long destination);
    virtual bool getEffectName (char* name);
    virtual bool getVendorString (char* text);
    virtual bool getProductString (char* text);
    virtual VstInt32 getVendorVersion () {return 1;}
    virtual VstInt32 canDo (char* text);

    virtual void sub_process(bool add, float** outputs, long sampleframes);
    virtual void calcWave(long n, long w);

    float getSwitch( long index, float defVal=1.0f );
    float midiExp;
    long currentPoti;
    p_adsr adsr[2];
    p_lfo  lfo[3];
    Filter2 filter[2];
private:
    void initProcess();
    void noteOn (long note, long velocity, long delta);
    void noteOff(long note, long velocity, long delta);

    p_analogProgram* programs;
    p_analogWave*    pWaves;
    p_analogVoice*   voices;

    long currentVoice;
    long currentDelta;
    bool noteIsOn;
};


//-----------------------------------------------------------------------------
class p_Editor : public AEffGUIEditor, public CControlListener {
public:
    p_Editor (AudioEffect* effect);
    virtual ~p_Editor ();

    virtual void setParameter (VstInt32 index, float value);
    virtual void valueChanged (CDrawContext* context, CControl* control);

protected:
    virtual bool open (void* ptr);
    virtual void close ();


private:
    // Controls
    CControl *control[kNControls];   

    // Bitmaps
    CBitmap* hBackground;
    CBitmap* hparBG;
    CBitmap* hFaderHandle;
    CBitmap* hKnobHandle;
    CBitmap* hKnobBg;
    CBitmap* hWave;
    CBitmap* hMat;
//    CBitmap* hFil;
    CBitmap* hLfo;
};



class p_analogEdit : public p_analog {
public:

    p_analogEdit (audioMasterCallback audioMaster);
    ~p_analogEdit ();

    virtual void setParameter (VstInt32 index, float value);
    virtual void setProgram(long index);
};


#endif // _p_analog_
