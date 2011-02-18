//
// original code by Perry R. Cook and Gary P. Scavone.
//

#include "ADSR.h"


const float minRate = 0.000001f;

ADSR :: ADSR( void )
{
	target = 0.0f;
	value = 0.0f;
	sampleRate = 44100.0f;
	attackRate = 0.001f;
	decayRate = 0.001f;
	releaseRate = 0.005f;
	sustainLevel = 0.5f;
	state = DONE;
}

ADSR :: ~ADSR( void )
{
}


void ADSR :: keyOn()
{
	target = 1.0f;
	state = ATTACK;
}

void ADSR :: keyOff()
{
	target = 0.0f;
	state = RELEASE;
}

void ADSR :: setSampleRate( float rate )
{
	if ( rate < 0.0f ) 
		sampleRate = 0.0f;
	else 
		sampleRate = rate;
}

void ADSR :: setSustainLevel( float level )
{
	if ( level < 0.0f ) 
		sustainLevel = 0.0f;
	else 
		sustainLevel = level;
}

void ADSR :: setAttackTime( float time )
{
	if ( time < 0.0f ) 
		attackRate = 1.0f / ( -time * sampleRate );
	else 
	if ( time > 0.0f ) 
		attackRate = 1.0f / ( time * sampleRate );
	else 
		attackRate = target;
}

void ADSR :: setDecayTime( float time )
{
	if ( time < 0.0f ) 
		decayRate = 1.0f / ( -time * sampleRate );
	else 
	if ( time > 0.0f ) 
		decayRate = 1.0f / ( time * sampleRate );
	else 
		decayRate = sustainLevel;
}

void ADSR :: setReleaseTime( float time )
{
	if ( time < 0.0f ) 
		releaseRate = sustainLevel / ( -time * sampleRate );
	else 
	if ( time > 0.0f ) 
		releaseRate = sustainLevel / ( time * sampleRate );
	else 
		releaseRate = 1.0f;
}

void ADSR :: setAllTimes( float aTime, float dTime, float sLevel, float rTime )
{
	this->setAttackTime( aTime );
	this->setDecayTime( dTime );
	this->setSustainLevel( sLevel );
	this->setReleaseTime( rTime );
}

void ADSR :: setTarget( float target )
{
	target = target;
	if ( value < target ) 
	{
		state = ATTACK;
		this->setSustainLevel( target );
	}
	if ( value > target ) 
	{
		this->setSustainLevel( target );
		state = DECAY;
	}
}

void ADSR :: setValue( float value )
{
	state = SUSTAIN;
	target = value;
	value = value;
	this->setSustainLevel( value );
}

