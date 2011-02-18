
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

struct Server
{
	uint me;
    SOCKADDR_IN   address;

	Server()
		: me(INVALID_SOCKET)
	{}

	uint setup( uint port=7777, bool reuseAddress=false )
	{
		me = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if ( me == INVALID_SOCKET  )
		{
			printf( "%s() error : couldn't create socket %x on port %d !", __FUNCTION__, me, port);
			return 0;
		}
		
		if ( reuseAddress  )
		{
			int optval = 1; 
			if ( ::setsockopt( me, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof (optval) ) == SOCKET_ERROR )
			{
				printf( "%s() error : couldn't setsockopt  for sock %x on port %d !", __FUNCTION__, me, port);
				return 0;
			}
		}

		address.sin_family      =  AF_INET;
		address.sin_addr.s_addr =  INADDR_ANY;
		address.sin_port        =  htons(port);

		if ( ::bind( me, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) ) == SOCKET_ERROR )
		{
			printf( "%s() error : couldn't bind sock %x to port %d !", __FUNCTION__, me, port);
			return 0;
		}

		return 1;
	}
	
	bool udpDataReady(uint timeout=30000000) const
	{
		fd_set rread;
		FD_ZERO( &rread );
		FD_SET( me, &rread );	

		struct timeval to;
		memset( &to, 0, sizeof(to) );
		to.tv_usec = timeout;

		if ( select( 128, &rread, NULL, NULL, &to ) > 0 && FD_ISSET( me, &rread ) )
			return true;
		return false;
	}

	virtual uint read( char *buffer, uint numBytes )  
	{
		assert(me!=INVALID_SOCKET);
		int addrlen = sizeof(SOCKADDR_IN);
		int res = 0;
		buffer[0] = 0;
		//if ( udpDataReady() )  // block
		{
			res = ::recvfrom ( me, buffer, numBytes, 0, (SOCKADDR*) &address, (ADDRPOINTER) &addrlen );
		}
		return (res!=SOCKET_ERROR) ? res : 0;
	}


	void run() 
	{
		char b[0xffff];
		for (int ever=1; ever; ever)
		{
			if ( udpDataReady() )
			{
				int n = read(b,0xfffe);
				printf( "%3d %s\n", n, b ); 
				memset(b,0,n+1);
			}
			else
			{
				//~ printf( "socket not connected.\n" ); 
			}
		}
	}
};


int main(int argc, char **argv)
{
	uint port = 7777;
	if ( argc > 1 ) port = atoi(argv[1]);

	Server serv;
	serv.setup(port);
	serv.run();

	return 0;
}
