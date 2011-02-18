
#ifndef STKADSRH
#define STKADSRH



class ADSR 
{
public:

	//! ADSR envelope states.
	enum {
		ATTACK,   //!< Attack
		DECAY,    //!< Decay 
		SUSTAIN,  //!< Sustain 
		RELEASE,  //!< Release 
		DONE      //!< End of release 
	};

	//! Default constructor.
	ADSR( void );

	//! Class destructor.
	~ADSR( void );

	//! Set target = 1, state = \e ADSR::ATTACK.
	void keyOn( void );

	//! Set target = 0, state = \e ADSR::RELEASE.
	void keyOff( void );

	//! Set the Sample rate.
	void setSampleRate( float rate );

	//! Set the sustain level.
	void setSustainLevel( float level );

	//! Set the attack rate based on a time duration.
	void setAttackTime( float time );

	//! Set the decay rate based on a time duration.
	void setDecayTime( float time );

	//! Set the release rate based on a time duration.
	void setReleaseTime( float time );

	//! Set sustain level and attack, decay, and release time durations.
	void setAllTimes( float aTime, float dTime, float sLevel, float rTime );

	//! Set the target value.
	void setTarget( float target );

	//! Return the current envelope \e state (ATTACK, DECAY, SUSTAIN, RELEASE, DONE).
	int getState( void ) const { return state; };

	//! Set to state = ADSR::SUSTAIN with current and target values of \e value.
	void setValue( float value );

	//! Compute and return one output sample.
	float tick( void );


protected:  


	int   state;
	float value;
	float target;
	float sampleRate;
	float attackRate;
	float decayRate;
	float releaseRate;
	float sustainLevel;
};



inline float ADSR :: tick( void )
{
	switch ( state ) 
	{
	case ATTACK:
		value += attackRate;
		if ( value >= target ) 
		{
			value  = target;
			target = sustainLevel;
			state  = DECAY;
		}
		break;

	case DECAY:
		value -= decayRate;
		if ( value <= sustainLevel ) 
		{
			value = sustainLevel;
			state = SUSTAIN;
		}
		break;

	case RELEASE:
		value -= releaseRate;
		if ( value <= 0.0 ) 
		{
			value = (float) 0.0;
			state = DONE;
		}
		break;
	}
	return value;
}


#endif
