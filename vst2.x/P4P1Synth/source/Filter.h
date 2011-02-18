#ifndef __Filter_onboard__
#define __Filter_onboard__

namespace FilterFormant
{
	void set(int vowelnum );
	float process( float sound );
}
namespace Filter4Pole
{
	void set( float q, float freq, float sampleFreq );
	float process( float sound );
}

namespace FilterBessel
{
	void set( float q, float freq, float sampleFreq );
	float process( float sound );
}


namespace FilterLP
{
	void set( float q, float freq, float sampleFreq );
	float process( float sound );
}
namespace FilterBP
{
	void set( float q, float freq, float sampleFreq );
	float process( float sound );
}

#endif // __Filter_onboard__

