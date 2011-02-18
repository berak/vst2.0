#include "UdpLog.h"

#include <stdio.h>
#include <assert.h>

#define MAXBUF 0xfffe

#ifdef _WIN32  
#include <winsock.h>
#include <windows.h>
#include <time.h>
#ifdef _MBCS /* mvs 6 defines this */
#include <malloc.h>
#define PORT         unsigned short
#else        /* any other win32 compiler */
#define PORT         unsigned long
#endif       /* _MCBS */
#define ADDRPOINTER   int*
#else          /* ! win32 */
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT         unsigned short
#define SOCKET       int
#define HOSTENT      struct hostent
#define SOCKADDR     struct sockaddr
#define SOCKADDR_IN  struct sockaddr_in
#define ADDRPOINTER  unsigned int*
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#endif /* _WIN32 */

struct _INIT_W32DATA
{
	WSADATA w;
	_INIT_W32DATA() {	WSAStartup( MAKEWORD( 2, 1 ), &w ); }
} _init_once;

typedef unsigned int uint;

struct Client
{
	SOCKET me;
    SOCKADDR_IN   address;

	Client()
		: me(INVALID_SOCKET)
	{}

	static unsigned long addressFromString( const char * name )  
	{
		if ( ! name ) return INADDR_NONE;

		unsigned long i_addr = inet_addr( name );
		if ( i_addr == INADDR_NONE ) {   // else : it was already an address
			HOSTENT *hostentry  = gethostbyname( name );
			if ( hostentry )
				i_addr =  *(unsigned long *)hostentry->h_addr_list[0];
		}		
		if ( i_addr == INADDR_NONE )
		{
			printf( "%s() error : couldn't resolve hostname '%s' !", __FUNCTION__, name );
		}
		return i_addr;
	}

	uint setup( uint port=7777, const char *addr="127.0.0.1" )
	{
		me = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if ( me == INVALID_SOCKET  )
		{
			printf( "%s() error : couldn't create socket %x on port %d !", __FUNCTION__, me, port);
			return 0;
		}
		
		address.sin_family      =  AF_INET;
		address.sin_addr.s_addr =   addressFromString( addr );
		address.sin_port        =  htons(port);

		return 1;
	}


	uint send( const char *line,  uint len )
	{
		return ::sendto ( me, line, len, 0, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) );
	}

};

namespace UdpLog	
{
	Client cli;

	void print( const char * msg )
	{
		cli.send(msg,strlen(msg));
	}
	void printf( const char * format, ... )
	{
		static char msg[2048] = {0};
		msg[0] = 0;
		va_list args;
		va_start( args, format );
		vsprintf( msg, format, args );
		va_end( args );
		cli.send(msg,strlen(msg));
	}

	void setup( int port, const char * host )
	{
		cli.setup(port,host);
	}
};


#ifdef TEST_PORT
int main(int argc, char **argv)
{
	uint port = 7777;
	if ( argc > 1 ) port = atoi(argv[1]);

	const char *addr = "127.0.0.1";

	Client cli;
	cli.setup(port,addr);

	char line[512];
	while(1)
	{
		gets(line);
		cli.send(line,strlen(line));
	}

	return 0;
}
#endif
