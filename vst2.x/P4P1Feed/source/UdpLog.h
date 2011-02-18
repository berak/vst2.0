#ifndef __UdpLog_onboard__
#define __UdpLog_onboard__

namespace UdpLog	
{
	void setup( int port, const char * host );
	void print( const char * msg );
	void printf( const char * format, ... );
};


#endif // __UdpLog_onboard__

