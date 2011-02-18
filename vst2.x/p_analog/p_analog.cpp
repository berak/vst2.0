/*-----------------------------------------------------------------------------

© 2001, Ein Zwerg GmbH, All Rights Cracked

-----------------------------------------------------------------------------*/

#ifndef __p_analog__
#include "p_analog.h"
#endif

static AudioEffect *_effect = 0;

//bool oome   = false;


const double midiScaler = (1. / 127.);
static float freqtab[kNumFrequencies];
 


//-------------------------------------------------------------------------------------------------------
AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
    bool doEdit = true;
    _effect = doEdit ?
             new p_analogEdit (audioMaster) : 
             new p_analog (audioMaster);
	return _effect;
}


//AEffect *main (audioMasterCallback audioMaster);
//AEffect *main (audioMasterCallback audioMaster) {
//    // get vst version
//    if(!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
//        return 0;  // old version
//
//    effect = doEdit ?
//             new p_analogEdit (audioMaster) : 
//             new p_analog (audioMaster);
//    if (!effect)
//        return 0;
//    if (oome)
//    {
//        delete effect;
//        return 0;
//    }
//    return effect->getAeffect ();
//}

#if MAC
#pragma export off
#endif
//
//#if WIN32
//#include <windows.h>
//void* hInstance;
//BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
//{
//    hInstance = hInst;
//    return 1;
//}
//#endif

//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
// p_analogProgram
//-----------------------------------------------------------------------------------------
p_analogProgram::p_analogProgram ()
{
    for (long i=0; i<2; i++ ) {
        wave[i]     = .5f;
        wspot[i]    = .5f;
        freq[i]     = .0f;
        vol[i]      = .5f;
        attack[i]   = .0f;
        decay[i]    = .2f;
        sustain[i]  = .0f;
        release[i]  = .0f;
        lfoFreq[i]  = .5f;
        lfoFac[i]   = .5f;
        filFreq[i]  = .0f;
        filFac[i]   = .02f;
//        filTyp[i]   = .5f;
    }
    lfoFreq[2]=lfoFac[2]=.5f;
    volume  = .9f;
    modulation = .0f;
    pan     = .5f;
    strcpy (name, "init");
    for (long a=0; a<kNRealPar; a++ ) 
        for (long b=0; b<kNumSwitches; b++ ) 
            matrix[a][b] = false;
}

//-----------------------------------------------------------------------------------------
// p_analog
//-----------------------------------------------------------------------------------------
p_analog::p_analog (audioMasterCallback audioMaster) 
    : AudioEffectX (audioMaster, kNumPrograms, kNumParams) 
    , p_analogProgram()
{
    pWaves   = new p_analogWave[2];
    programs = new p_analogProgram[kNumPrograms];
    voices   = new p_analogVoice[kNumVoices];
    currentPoti = 0;
    if (audioMaster) {
        setNumInputs (0);               // no inputs
        setNumOutputs (kNumOutputs);    // 2 outputs, 1 for each osciallator
        canProcessReplacing ();
///PPP        hasClip (false);
        isSynth ();
    }
    setUniqueID ('aNNa');
    initProcess ();
    if (programs)
        setProgram(0);
   // suspend ();
    resume();
}

//-----------------------------------------------------------------------------------------
p_analog::~p_analog ()
{
    if (programs)
        delete[] programs;
    if (pWaves)
        delete[] pWaves;
    if (voices)
        delete[] voices;
}

//-----------------------------------------------------------------------------------------
void p_analog::setProgram (VstInt32 program)
{
    p_analogProgram *ap = &programs[program];
    curProgram = program;
    for ( long i=0; i<2; i++ ) {
        wave[i]     = ap->wave[i];
        wspot[i]    = ap->wspot[i];
        calcWave(i,long(wave[i]*kNumWaves));

        freq[i]     = ap->freq[i];
        vol[i]      = ap->vol[i];
        adsr[i].set_a( attack[i]   = ap->attack[i] );
        adsr[i].set_d( decay[i]    = ap->decay[i]  );
        adsr[i].set_s( sustain[i]  = ap->sustain[i]);
        adsr[i].set_r( release[i]  = ap->release[i]);

        lfoFreq[i]  = ap->lfoFreq[i];
        lfoFac[i]   = ap->lfoFac[i];
        
        float f = filFreq[i]  = ap->filFreq[i];
        float w = filFac[i]   = ap->filFac[i];
        filter[i].set(f,w);
    }
    lfoFreq[2]  = ap->lfoFreq[2];
    lfoFac[2]   = ap->lfoFac[2];
    modulation = ap->modulation;
    volume = ap->volume;
    pan    = ap->pan;
    for (long a=0; a<kNRealPar; a++ ) 
        for (long b=0; b<kNumSwitches; b++ ) 
            matrix[a][b] = ap->matrix[a][b];
}

//-----------------------------------------------------------------------------------------
void p_analog::setProgramName (char *name)
{
    strcpy (programs[curProgram].name, name);
}

//-----------------------------------------------------------------------------------------
void p_analog::getProgramName (char *name)
{
    strcpy (name, programs[curProgram].name);
}

//-----------------------------------------------------------------------------------------
void p_analog::getParameterLabel (VstInt32 index, char *label)
{
}

//-----------------------------------------------------------------------------------------
void p_analog::getParameterDisplay (VstInt32 index, char *text)
{
}

//-----------------------------------------------------------------------------------------
void p_analog::getParameterName (VstInt32 index, char *label)
{
    switch (index)
    {
        case kWave0:     strcpy (label, " Wave 1");     break;
        case kWspot0:    strcpy (label, " Wspot 1");     break;
        case kFreq0:     strcpy (label, " Freq  1");     break;
        case kVol0:      strcpy (label, " Vol  1");     break;
        case kAttack0:   strcpy (label, " Attack 1");     break;
        case kSustain0:  strcpy (label, " Sustain 1");     break;
        case kDecay0:    strcpy (label, " Decay  1");     break;
        case kRelease0:  strcpy (label, " Release 1");     break;
        case kWave1:     strcpy (label, " Wave 2");     break;
        case kWspot1:    strcpy (label, " Wspot 2");     break;
        case kFreq1:     strcpy (label, " Freq  2");     break;
        case kVol1:      strcpy (label, " Vol  2");     break;
        case kAttack1:   strcpy (label, " Attack 2");     break;
        case kSustain1:  strcpy (label, " Sustain 2");     break;
        case kDecay1:    strcpy (label, " Decay  2");     break;
        case kRelease1:  strcpy (label, " Release 2");     break;
        case kVolume:   strcpy (label, " Volume ");     break;
        case kMod:      strcpy (label, " Mod type");     break;
        case kPan:      strcpy (label, " Pan    ");     break;
        case kLfoFreq0:   strcpy (label, " Lfo freq 1");     break;
        case kLfoFac0:    strcpy (label, " Lfo fac  1");     break;
        case kLfoFreq1:   strcpy (label, " Lfo freq 2");     break;
        case kLfoFac1:    strcpy (label, " Lfo fac  2");     break;
        case kLfoFreq2:   strcpy (label, " Lfo freq 3");     break;
        case kLfoFac2:    strcpy (label, " Lfo fac  3");     break;
        case kFilFreq0:   strcpy (label, " Fil freq 1");     break;
        case kFilFac0:    strcpy (label, " Fil fac  1");     break;
//        case kFilTyp0:    strcpy (label, " Fil typ  1");     break;
        case kFilFreq1:   strcpy (label, " Fil freq 2");     break;
        case kFilFac1:    strcpy (label, " Fil fac  2");     break;
//        case kFilTyp1:    strcpy (label, " Fil typ  2");     break;
    }
}

//-----------------------------------------------------------------------------------------
void p_analog::setParameter (VstInt32 index, float value)
{
    p_analogProgram *ap = &programs[curProgram];
    switch (index) {
        case kPotiName: currentPoti = (long)value; break;
        case kWave0:     wave[0]    = ap->wave[0]      = value; calcWave(0,long(value*kNumWaves));break;
        case kWspot0:    wspot[0]   = ap->wspot[0]     = value; calcWave(0,long(wave[0]*kNumWaves)); break;
        case kFreq0:     freq[0]    = ap->freq[0]      = value;    break;
        case kVol0:      vol[0]     = ap->vol[0]       = value;    break;
        case kAttack0:   attack[0]  = ap->attack[0]    = value;   adsr[0].set_a(value);  break;
        case kDecay0:    decay[0]   = ap->decay[0]     = value;   adsr[0].set_d(value); break;
        case kSustain0:  sustain[0] = ap->sustain[0]   = value;   adsr[0].set_s(value); break;
        case kRelease0:  release[0] = ap->release[0]   = value;   adsr[0].set_r(value); break;
        case kWave1:     wave[1]    = ap->wave[1]      = value; calcWave(1,long(value*kNumWaves));break;
        case kWspot1:    wspot[1]   = ap->wspot[1]     = value; calcWave(1,long(wave[1]*kNumWaves)); break;
        case kFreq1:     freq[1]    = ap->freq[1]      = value;    break;
        case kVol1:      vol[1]     = ap->vol[1]       = value;    break;
        case kAttack1:   attack[1]  = ap->attack[1]    = value;   adsr[1].set_a(value);  break;
        case kDecay1:    decay[1]   = ap->decay[1]     = value;   adsr[1].set_d(value); break;
        case kSustain1:  sustain[1] = ap->sustain[1]   = value;   adsr[1].set_s(value); break;
        case kRelease1:  release[1] = ap->release[1]   = value;   adsr[1].set_r(value); break;
        case kMod:      modulation  = ap->modulation   = value;    break;
        case kVolume:   volume  = ap->volume    = value;    break;
        case kPan:      pan     = ap->pan       = value;    break;
        case kLfoFreq0:   lfoFreq[0]  = ap->lfoFreq[0]    = value;  lfo[0].setTime(value);  break;
        case kLfoFac0:    lfoFac[0]   = ap->lfoFac[0]     = value;  lfo[0].setSpot(value);  break;
        case kLfoFreq1:   lfoFreq[1]  = ap->lfoFreq[1]    = value;  lfo[1].setTime(value);  break;
        case kLfoFac1:    lfoFac[1]   = ap->lfoFac[1]     = value;  lfo[1].setSpot(value);  break;
        case kLfoFreq2:   lfoFreq[2]  = ap->lfoFreq[2]    = value;  lfo[2].setTime(value);  break;
        case kLfoFac2:    lfoFac[2]   = ap->lfoFac[2]     = value;  lfo[2].setSpot(value);  break;
        case kSKey:   matrix[currentPoti][0] = ap->matrix[currentPoti][0] = value; break;
        case kSVel:   matrix[currentPoti][1] = ap->matrix[currentPoti][1] = value; break;
        case kSPan:   matrix[currentPoti][2] = ap->matrix[currentPoti][2] = value; break;
        case kSExp:   matrix[currentPoti][3] = ap->matrix[currentPoti][3] = value; break;
        case kSLfo0:  matrix[currentPoti][4] = ap->matrix[currentPoti][4] = value; break;
        case kSLfo1:  matrix[currentPoti][5] = ap->matrix[currentPoti][5] = value; break;
        case kSLfo2:  matrix[currentPoti][6] = ap->matrix[currentPoti][6] = value; break;
        case kSEnv0:  matrix[currentPoti][7] = ap->matrix[currentPoti][7] = value; break;
        case kSEnv1:  matrix[currentPoti][8] = ap->matrix[currentPoti][8] = value; break;
        case kFilFreq0:   filFreq[0]  = ap->filFreq[0] = value;  filter[0].set(filFreq[0], filFac[0]);  break;
        case kFilFac0:    filFac[0]   = ap->filFac[0]  = value;    filter[0].set(filFreq[0], filFac[0]);  break;
       // case kFilTyp0:    filTyp[0]   = ap->filTyp[0]  = value;  filter[0].setType(int(3*value));  break;
        case kFilFreq1:   filFreq[1]  = ap->filFreq[1] = value;    filter[1].set(filFreq[1], filFac[1]);  break;
        case kFilFac1:    filFac[1]   = ap->filFac[1]  = value;    filter[1].set(filFreq[1], filFac[1]);  break;
       // case kFilTyp1:    filTyp[1]   = ap->filTyp[1]  = value;  filter[1].setType(int(3*value));  break;
        default: return;
    }
}

//-----------------------------------------------------------------------------------------
float p_analog::getParameter (VstInt32 index)
{
    float value = 0;
    switch (index)
    {
        case kWave0:     value = wave[0];   break;
        case kWspot0:    value = wspot[0];  break;
        case kFreq0:     value = freq[0];   break;
        case kVol0:      value = vol[0]; break;
        case kAttack0:   value = attack[0]; break;
        case kDecay0:    value = decay[0];  break;
        case kSustain0:  value = sustain[0];  break;
        case kRelease0:  value = release[0];  break;
        case kWave1:     value = wave[1];   break;
        case kWspot1:    value = wspot[1];  break;
        case kFreq1:     value = freq[1];   break;
        case kVol1:      value = vol[1]; break;
        case kAttack1:   value = attack[1]; break;
        case kDecay1:    value = decay[1];  break;
        case kSustain1:  value = sustain[1];  break;
        case kRelease1:  value = release[1];  break;
        case kMod:      value = modulation; break;
        case kVolume:   value = volume; break;
        case kPan:      value = pan;    break;
        case kLfoFreq0:   value = lfoFreq[0]; break;
        case kLfoFac0:    value = lfoFac[0];  break;
        case kLfoFreq1:   value = lfoFreq[1]; break;
        case kLfoFac1:    value = lfoFac[1];  break;
        case kLfoFreq2:   value = lfoFreq[2]; break;
        case kLfoFac2:    value = lfoFac[2];  break;
        case kFilFreq0:   value = filFreq[0]; break;
        case kFilFac0:    value = filFac[0];  break;
//        case kFilTyp0:    value = filTyp[0];  break;
        case kFilFreq1:   value = filFreq[1]; break;
        case kFilFac1:    value = filFac[1];  break;
//        case kFilTyp1:    value = filTyp[1];  break;
        case kSKey:   value = matrix[currentPoti][0]; break; 
        case kSVel:   value = matrix[currentPoti][1]; break;
        case kSPan:   value = matrix[currentPoti][2]; break;
        case kSExp:   value = matrix[currentPoti][3]; break;
        case kSLfo0:  value = matrix[currentPoti][4]; break;
        case kSLfo1:  value = matrix[currentPoti][5]; break; 
        case kSLfo2:  value = matrix[currentPoti][6]; break;  
        case kSEnv0:  value = matrix[currentPoti][7]; break; 
        case kSEnv1:  value = matrix[currentPoti][8]; break;
        case kPotiName: value = (float)currentPoti;  break;
    }
    return value;
}

//-----------------------------------------------------------------------------------------
bool p_analog::getOutputProperties (long index, VstPinProperties* properties)
{
    if (index < kNumOutputs)
    {
        sprintf (properties->label, "aNNa %1d", index + 1);
        properties->flags = kVstPinIsActive;
        if (index < 2)
            properties->flags |= kVstPinIsStereo;   // test, make channel 1+2 stereo
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------
bool p_analog::getProgramNameIndexed (long category, long index, char* text)
{
    if (index < kNumPrograms)
    {
        strcpy (text, programs[index].name);
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------
bool p_analog::copyProgram (long destination)
{
    if (destination < kNumPrograms)
    {
        programs[destination] = programs[curProgram];
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------
bool p_analog::getEffectName (char* name)
{
    strcpy (name, "p_analog");
    return true;
}

//-----------------------------------------------------------------------------------------
bool p_analog::getVendorString (char* text)
{
    strcpy (text, "ein zwerg");
    return true;
}

//-----------------------------------------------------------------------------------------
bool p_analog::getProductString (char* text)
{
    strcpy (text, "analogsynth");
    return true;
}

//-----------------------------------------------------------------------------------------
VstInt32 p_analog::canDo (char* text)
{
    if (!strcmp (text, "receiveVstEvents"))
        return 1;
    if (!strcmp (text, "receiveVstMidiEvent"))
        return 1;
    return -1;  // explicitly can't do; 0 => don't know
}

//-----------------------------------------------------------------------------------------
void p_analog::setSampleRate (float sampleRate)
{
    AudioEffectX::setSampleRate (sampleRate);
}

//-----------------------------------------------------------------------------------------
void p_analog::setBlockSize (long blockSize)
{
    AudioEffectX::setBlockSize (blockSize);
    // you may need to have to do something here...
}

//-----------------------------------------------------------------------------------------
void p_analog::resume ()
{
	AudioEffectX::resume ();
///PPP    wantEvents ();
}

//-----------------------------------------------------------------------------------------
void p_analog::calcWave (long n, long w) {
    long i; 
    float d0,d1,v;
    char _acid[152] = "In the Top 40, half the songs are secret messages to the teen world to drop out, turn on, and groove with the chemicals and light shows at discotheques"; 

    // make waveforms
    long ws = (long)(  wspot[n] * (double)kWaveSize );
    switch(w) {
    case 0:
        strcpy( pWaves[n].name, "  sinus" );
        for (i = 0; i < kWaveSize; i++)
            pWaves[n].data[i] = (float)sin( 2.0 * Pi * (double)i / (double)kWaveSize);
        break;
    case 1:
        strcpy( pWaves[n].name, "  triangle" );
        d0 = 1.0f/ws;
        d1 =-1.0f/(kWaveSize-ws);
        v  =-1.0f;
        for (i = 0; i < kWaveSize; i++) {
            v += (i<ws) ? d0 : d1;
            pWaves[n].data[i] = v;
        }
        break;
    case 2:
        strcpy( pWaves[n].name, "  pulse" );
        for (i = 0; i < kWaveSize; i++)
            pWaves[n].data[i] = (i < ws) ? -1.f : 1.f;
        break;
    case 3:
        strcpy( pWaves[n].name, "  acid" );
        for (i = 0; i < kWaveSize; i++)
            pWaves[n].data[i] = float( 1.f - ( _acid[long(3+i*.08f*wspot[n]*wspot[n])%152] - 32 ) / 47.5f ) ;
        break;
    case 4:
        strcpy( pWaves[n].name, "  noize" );
        for (i = 0; i < kWaveSize; i++)
            pWaves[n].data[i] = (rand()%2)?1.0f:-1.0f ;
        break;
    }
}
//-----------------------------------------------------------------------------------------
void p_analog::initProcess ()
{
    noteIsOn = false;
    currentDelta = 0;

    // make waveforms
    calcWave(0,0);
    calcWave(1,0);

    // make frequency (Hz) table:
    float fScaler = (float)((double)kWaveSize / 44100.);  // we don't know the sample rate yet
    double k = 1.059463094359;  // 12th root of 2
    double a = 6.875;   // a
    a *= k; // b
    a *= k; // bb
    a *= k; // c, frequency of midi note 0
    for (long i = 0; i < kNumFrequencies; i++)   // 128 midi notes
    {
        freqtab[i] = (float)a * fScaler;
        a *= k;
    }
}

//-----------------------------------------------------------------------------------------

float p_analog::getSwitch( long index, float defVal ) {
    float *p = matrix[index];
    if ( *p++ ) defVal *= voices[currentVoice].noteScaled;
    if ( *p++ ) defVal *= voices[currentVoice].velocityScaled;  
    if ( *p++ ) defVal *= pan;  
    if ( *p++ ) defVal *= midiExp; 
    if ( *p++ ) defVal *= lfo[0].level;  
    if ( *p++ ) defVal *= lfo[1].level;  
    if ( *p++ ) defVal *= lfo[2].level;  
    if ( *p++ ) defVal *= voices[currentVoice].a_level[0]; 
    if ( *p++ ) defVal *= voices[currentVoice].a_level[1]; 
    return defVal;
}

//-----------------------------------------------------------------------------------------
void p_analog::sub_process (bool add, float **outputs, long sampleFrames)
{   
    if (noteIsOn == false) return;

    float* outl  = outputs[0];
    float* outr  = outputs[1];
    float amp[2], frq[2], main_vol, pan_l, pan_r;
    long i=0;

    for ( i=0; i<3; i++ ) lfo[i].tick();

    if ( currentDelta ) {
        outl += currentDelta;
        outr += currentDelta;
        sampleFrames -= currentDelta;
        currentDelta = 0;
    }
    if ( add ) {
        // WILL YOU PLEEEAAZZZE SHUT UP ??!!
        memset( outl, 0, sampleFrames * sizeof(float) );
        memset( outr, 0, sampleFrames * sizeof(float) );
    }

    if ( filFreq[0] ) filter[0].set( getSwitch( kFilFreq0, filFreq[0] ), getSwitch( kFilFac0,filFac[0] ) );
    if ( filFreq[1] ) filter[1].set( getSwitch( kFilFreq1, filFreq[1] ), getSwitch( kFilFac1, filFac[1] ) );
    pan_l = 1 + (pan-0.5f);
    pan_r = 1 - (pan-0.5f);
    main_vol = getSwitch( kVolume, volume );
    amp[0] = getSwitch( kVol0, vol[0] );
    amp[1] = getSwitch( kVol1, vol[1] );
    frq[0] = getSwitch( kFreq0, kfMul * freq[0] );
    frq[1] = getSwitch( kFreq1, kfMul * freq[1] );

    while ( noteIsOn && (--sampleFrames >= 0) ) {

        int   playing = 0;
        int   ds      = 0;
        float mono    = 0.0f;

        for ( i=0; i<kNumVoices; i++ ) {
            p_analogVoice *av = &voices[i]; 
            if ( av->active == false )    continue;
            float w = 0.0f;
            for ( int osc=0; osc<2; osc++ ) {
                float env = adsr[osc].val( av, osc );
                if ( av->phase[osc] >= kWaveSize ) av->phase[osc] -= kWaveSize;
                if ( av->phase[osc] <  0.0f      ) av->phase[osc] += kWaveSize;
                if ( osc == 1 && (modulation<.6f) && (modulation>0.3f) ) {
                    // osc1 -> ring :
                    w *= amp[osc] * env * pWaves[osc].data[long(av->phase[osc])];  
                } else if ( osc == 1 && (modulation>.6f) ) {
                    // osc1 -> phase:
                    w = amp[osc] * env * pWaves[osc].data[abs(int(av->phase[osc]*w))%kWaveSize];
                } else {
                    // osc0 or mix:
                    w += amp[osc] * env * pWaves[osc].data[long(av->phase[osc])];
                }
                av->phase[osc] += frq[osc] + freqtab[av->note];
            }
            mono += w;
            playing++;
        }
        if ( ! playing ) {
            noteIsOn = false;
            filter[0].stop(); filter[1].stop();
            return; 
        }
        // filter:
        float m1 = ( filFreq[0] ) ? filter[0].value(mono) : mono;
        float m2 = ( filFreq[1] ) ? filter[1].value(mono) : mono;
        // send to outputs:
        mono = (m1+m2) * main_vol / (kNumVoices);
        // normalize:
        if ( mono >  1 ) mono =  1.0f; 
        if ( mono < -1 ) mono = -1.0f;
        if ( add ) { (*outl++) += mono * (pan_l);  (*outr++) += mono * pan_r; }
        else       { (*outl++)  = mono * (pan_l);  (*outr++)  = mono * pan_r; }
    }                       
}

//-----------------------------------------------------------------------------------------
void p_analog::process (float **inputs, float **outputs, VstInt32 sampleFrames) {
    sub_process( true, outputs, sampleFrames );
}
//-----------------------------------------------------------------------------------------
void p_analog::processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames) {
    sub_process( false, outputs, sampleFrames );
}

//-----------------------------------------------------------------------------------------
VstInt32 p_analog::processEvents (VstEvents* ev)
{
    for (long i = 0; i < ev->numEvents; i++)
    {
        if ((ev->events[i])->type != kVstMidiType)
            continue;
        VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
        char* midiData = event->midiData;
        long status = midiData[0] & 0xf0;       // ignoring channel
        if ( status == 0x80 ) {                 // note off
            long note = midiData[1] & 0x7f;
            long velocity = midiData[2] & 0x7f;
            noteOff (note, velocity, event->deltaFrames);
        } else if ( status == 0x90 ) {   // note on
            long note = midiData[1] & 0x7f;
            long velocity = midiData[2] & 0x7f;
            if ( velocity ) {
                noteOn (note, velocity, event->deltaFrames);
            } else {
                noteOff (note, velocity, event->deltaFrames);
            }
        } else if ( status == 0xb0 ) { // controller:
            switch( midiData[1] ) {
            case 0x01 : // expression
                midiExp =  float(midiData[2] / 127.0f ); 
                break;
            case 0x07 : // vol
                setParameter( kVolume, (float)midiData[2] / 127.0f ); 
                break;
            case 0x08 : // balance
            case 0x0a : // pan
                setParameter( kPan, (float)midiData[2] / 127.0f ); 
                break;
            case 0x7e : // all notes off
                for ( long i=0; i<kNumVoices; i++ ) 
                    voices[i].stop();
                noteIsOn = false;
                break;
            }
        }
        event++;
    }
    return 1;   // want more
}
//-----------------------------------------------------------------------------------------
void p_analog::noteOff (long note, long velocity, long delta)
{
    for ( long i=0; i<kNumVoices; i++ ) 
        if ( note == voices[i].note )
            voices[i].hold = false;
}
//-----------------------------------------------------------------------------------------
void p_analog::noteOn (long note, long velocity, long delta)
{
    currentVoice +=1;
    currentVoice %= kNumVoices;
    voices[currentVoice].start(); 
    voices[currentVoice].active = (velocity ? true : false ); 
    voices[currentVoice].note = note;
    voices[currentVoice].velocity = velocity;
    voices[currentVoice].noteScaled = note / 128.0f;
    voices[currentVoice].velocityScaled = velocity / 128.0f;
    currentDelta = delta;
    noteIsOn = true;
    // trigger lfo's:
    if ( matrix[kLfoFac0][0] ) lfo[0].start();
    if ( matrix[kLfoFac1][0] ) lfo[1].start();
    if ( matrix[kLfoFac2][0] ) lfo[2].start(); 
}



//-----------------------------------------------------------------------------
// prototype string convert float -> percent
void percentStringConvert (float value, char* string) {
     sprintf (string, "%d%%", (int)(100 * value));
}
void poti2string(float value, char* string) {
    _effect->getParameterName( (long) value, string );
}


//-----------------------------------------------------------------------------
// p_Editor class implementation
//-----------------------------------------------------------------------------
p_Editor::p_Editor (AudioEffect *effect) : AEffGUIEditor (effect) 
{
    hFaderHandle = 0;
    hKnobHandle = 0;
    hKnobBg = 0;
    hparBG = 0;
    hWave = 0;
    hMat = 0;
    hLfo = 0;

    for( long i=0; i<kNControls; i++ ) 
        control[i] = 0;

    // load the background bitmap
    // we don't need to load all bitmaps, this could be done when open is called
    hBackground  = new CBitmap (kBackgroundId);
    // init the size of the plugin
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = (short)hBackground->getWidth ();
    rect.bottom = (short)hBackground->getHeight ();
}

//-----------------------------------------------------------------------------
p_Editor::~p_Editor ()
{
    // free background bitmap
    if (hBackground)
        hBackground->forget ();
    hBackground = 0;
}



//-----------------------------------------------------------------------------
bool p_Editor::open (void *ptr)
{
    // !!! always call this !!!
    AEffGUIEditor::open (ptr);
    
    // load some bitmaps
    if (!hparBG)
        hparBG = new CBitmap (kparBGId);
    if (!hFaderHandle)
        hFaderHandle = new CBitmap (kFaderHandleId);
    if (!hKnobHandle)
        hKnobHandle = new CBitmap (kKnobHandleId);
    if (!hKnobBg)
        hKnobBg = new CBitmap (kKnobBgId);
    if (!hWave)
        hWave = new CBitmap (kWaveId);
 //   if (!hFil)
 //       hFil = new CBitmap (kFilId);
    if (!hMat)
        hMat = new CBitmap (kMatId);
    if (!hLfo)
        hLfo = new CBitmap (kLfoId);


    //--init background frame-----------------------------------------------
    CRect size (0, 0, hBackground->getWidth (), hBackground->getHeight ());
    frame = new CFrame (size, ptr, this);
    frame->setBackground (hBackground);

    //--init the Knobs------------------------------------------------
    CPoint point (0, 0);
    CPoint offset (1, 0);
    int id = kWave0;
    int fmin=0,fmax=0;
    int x = 11, y = 10;

    //   oscillators:
	long k=0;
    for (  k=0;k<2;k++ ) {
        x=11+k*173; y = 10;
        control[id] = new CVerticalSwitch (
            CRect(x, y, x + hWave->getWidth(), y + 60 ),
            this, id, 5, 60, 6, hWave, point);
        control[id]->setValue (effect->getParameter (id));
        frame->addView(control[id++]);
        x += 28; y += 7; // 39,17
        size (x, y, x + hparBG->getWidth (), y + hparBG->getHeight ());

        fmin = x+3, fmax = x+hparBG->getWidth()-4;
        control[id] = new CHorizontalSlider (
            CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
            this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
        ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        y += (hparBG->getHeight() + 6);
        control[id] = new CHorizontalSlider (
            CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
            this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
        ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        y += (hparBG->getHeight() + 6);
        control[id] = new CHorizontalSlider (
            CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
            this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
        ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        
        x +=64; y = 16;// 103,16
        size (x, y, x + 30, y + 30 );
        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        x +=36; 
        size (x, y, x + 30, y + 30 );
        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        x -= 36; y += 39;
        size (x, y, x + 30, y + 30 );
        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
        x +=36; 
        size (x, y, x + 30, y + 30 );
        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
    }

    //   filtors:
    for ( k=0;k<2;k++ ) {
        x=22+k*90; y = 112;
        size (x, y, x + 30, y + 30 );

        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);

        x -= 9; y += 36; // 13,149
        fmin = x+3, fmax = x+hparBG->getWidth()-4;
        control[id] = new CHorizontalSlider (
            CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
            this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
        ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
/*        
        x +=54; y -= 36; // 67,112;
        control[id] = new CVerticalSwitch (
            CRect(x, y, x + hFil->getWidth(), y + 56 ),
            this, id, 4, 56, 4, hFil, point);
        control[id]->setValue (effect->getParameter (id ));
        frame->addView(control[id++]);
*/
    }

    //   vol/pan:
    x=276; y = 111;
    size (x, y, x + 30, y + 30 );
    control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
    control[id]->setValue (effect->getParameter (id ));
    frame->addView (control[id++]);
    x = 313;
    size (x, y, x + 30, y + 30 );
    control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
    control[id]->setValue (effect->getParameter (id ));
    frame->addView (control[id++]);

    // modulation:
    x = 276; y = 232;
    fmin = x+3, fmax = x+hparBG->getWidth()-4;
    control[id=kMod] = new CHorizontalSlider (
        CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
        this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
    ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
    control[id]->setValue (effect->getParameter (id ));
    frame->addView (control[id++]);

    // lfo
    for ( k=0;k<3;k++ ) {
        x=19+k*60; y = 196;
        size (x, y, x + 30, y + 30 );
        control[id] = new CKnob ( size, this, id, hKnobBg, hKnobHandle, CPoint(0,0));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);

        x -= 9; y += 36; // 10,232
        fmin = x+3, fmax = x+hparBG->getWidth()-4;
        control[id] = new CHorizontalSlider (
            CRect(x,y,x+hparBG->getWidth(),y+hparBG->getHeight()),
            this, id, fmin, fmax, hFaderHandle, hparBG, point, kLeft);
        ((CHorizontalSlider*)control[id])->setOffsetHandle (CPoint(0,4));
        control[id]->setValue (effect->getParameter (id ));
        frame->addView (control[id++]);
    }

    // switches:
    x = 192; y = 105;
    size( x, y, x + hMat->getWidth(), y + hMat->getHeight()/2 );
    for (id=kSKey;id<kSKey+kNumSwitches;id++)  {
        control[id]=  new COnOffButton ( size, this, id, hMat);
        control[id]->setValue( effect->getParameter(id) );
        frame->addView( control[id] );
        size.offset(0, hMat->getHeight()/2) ;
    }

    // potiName:
    x = 192; y = 244;
    size (x, y, x + 50, y + 18);
    control[kPotiName] = new CParamDisplay (size, hparBG, kCenterText);
    ((CParamDisplay*)control[kPotiName])->setFont (kNormalFontSmall);
    ((CParamDisplay*)control[kPotiName])->setFontColor (kYellowCColor);
    control[kPotiName]->setValue (effect->getParameter (kPotiName));
    ((CParamDisplay*)control[kPotiName])->setStringConvert (poti2string);
    frame->addView (control[kPotiName]);

    // strip:
    x = 266; y = 166;
    int wh = hLfo->getHeight()/10;
    control[kStrip] = new CMovieBitmap (
        CRect (x,y,x+hLfo->getWidth(),y+wh),
        this, kStrip, 10, wh, hLfo, point);
    control[kStrip]->setValue (effect->getParameter (kStrip));
    frame->addView (control[kStrip]);

    return true;
}

//-----------------------------------------------------------------------------
void p_Editor::close ()
{
    delete frame;
    frame = 0;

    // free some bitmaps
    if (hFaderHandle)
        hFaderHandle->forget ();
    hFaderHandle = 0;
    if (hKnobHandle)
        hKnobHandle->forget ();
    hKnobHandle = 0;
    if (hKnobBg)
        hKnobBg->forget ();
    hKnobBg = 0;
    if (hparBG)
        hparBG->forget ();
    hparBG = 0;
/*  
    if (hFil)
        hFil->forget ();
    hFil = 0;
*/
    if (hWave)
        hWave->forget ();
    hWave = 0;
    if (hMat)
        hMat->forget ();
    hMat = 0;
    if (hLfo)
        hLfo->forget ();
    hLfo = 0;
}

//-----------------------------------------------------------------------------
void p_Editor:: setParameter (VstInt32 index, float value)
{
    if ( ! frame ) return;
    if ( index < kNControls ) {
        if ( control[index] )
            control[index]->setValue(value);
///PPP        postUpdate ();
    }
}

//-----------------------------------------------------------------------------
void p_Editor::valueChanged (CDrawContext* context, CControl* control)
{
    long tag = control->getTag();
    if ( tag < kNControls ) {
        effect->setParameterAutomated( tag, control->getValue() );
///PPP        control->update( context ); 
    }
}

//-----------------------------------------------------------------------------
p_analogEdit::p_analogEdit (audioMasterCallback audioMaster) : p_analog (audioMaster)
{
     editor = new p_Editor (this);
//    if ( doEdit )   editor = new p_Editor (this);
//    if ( ! editor ) oome   = true;
}

//-----------------------------------------------------------------------------
p_analogEdit::~p_analogEdit ()
{
    // the editor gets deleted by the
    // AudioEffect base class
}

//-----------------------------------------------------------------------------
void p_analogEdit::setProgram (long index)
{
    currentPoti = 0l;
    p_analog::setProgram(index);
    for(long i=0; i<kNControls; i++ )
        ((p_Editor*)editor)->setParameter( i, getParameter(i) );
    updateDisplay();
}

int anim=0,lastSelected = 0;
//-----------------------------------------------------------------------------
void p_analogEdit::setParameter (VstInt32 index, float value)
{
    p_Editor* ed =(p_Editor*)editor;   
    if ( ! ed ) return;

    p_analog::setParameter (index, value);
    ed->setParameter (index, getParameter(index));
    if (index<kNRealPar && index != lastSelected ) {
        lastSelected = index;
        setParameter( kPotiName, (float) index );
        for (long id=kSKey;id<kSKey+kNumSwitches;id++)  
            ed->setParameter( id, getParameter(id) );
    }
    ed->setParameter( kStrip, float(++anim%10)/10 );
}
