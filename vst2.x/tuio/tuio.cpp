//
//  $ cl /nologo /DWIN32 test.cpp sock.cpp wsock32.lib user32.lib winmm.lib
//

//#include <vector>
#include <windows.h>
#include <stdio.h>
#include "source/sock.h"
#include "tuio.h"
#include "Profile.h"
#include "w32.h"

extern w32::CriticalSection critSec;

namespace Tuio
{

	static Object emptyObject = {0};

	struct Parser
	{
 
		struct Packet 
		{
			int len;
			char * head;
			char * pinfo;
			char * cmd;
		};



		int align4( int z )
		{
			int m = z % 4;
			if ( ! m ) return z;

			int o = 4 - m;
			return z + o;
		}

		
		//! osc layer packet
		int parsePacketHeader( char * data, Packet & packet )
		{
			int k = 0;
			k += getInt( data, packet.len );

			k += getString( data+k, &(packet.head) );
			k += getString( data+k, &(packet.pinfo) );
			k += getString( data+k, &(packet.cmd) );

			return k;
		}

		int parse( char * data, int n )
		{
			w32::Lock lock( &critSec );
			PROFILE;
			
			//int a[512];
			//int nAlive = 0;
			int k=0,mode=0;
			char * b = data;
			while ( k < n )
			{
				//fprintf( stderr, "$ %d %s\n", k, &b[k] );
				if ( strstr( &b[k], "#bundle" ) )
				{
					mode = 0;
					/// moved, see below
					//listener.startBundle();
					
					// ok, we'll skip 'all the seconds there ever were'
					k += 16;
				}
				else
				{
					Packet packet = {0};
					k += parsePacketHeader( &b[k], packet );
					//printf("p %d\t'%s'\t'%s'\t'%s'\n",packet.len,packet.head,packet.cmd,packet.pinfo);

					// cursor or object ?
					if ( ! strcmp( packet.head, "/tuio/2Dobj" ) )
					{
						//
						// this hack keeps our objects alive -
						// we only start a bundle for objects, 
						// sending the cursors to the current bundle:
						//
						if ( mode == 0 )
							listener.startBundle();
						mode = 1;
					} 
					else
					if ( ! strcmp( packet.head, "/tuio/2Dcur" ) )
					{
						mode = 2;
					}
					
					// check the cmd:
					if ( ! strcmp( packet.cmd, "set" ) )
					{
						Object o = {0};
						o.type = mode;
						k += parseSet( &b[k], &o, mode );
						listener.call( o );
					}
					else
					if ( ! strcmp( packet.cmd, "fseq" ) )
					{
						int s = 0;
						k += getInt( &b[k], s );
						if ( seq > s )
						{
							//fprintf( stderr, "!!!seq  %d > %d\n", seq,s );
						}
						seq = s;
					}
					else
					if ( ! strcmp( packet.cmd, "alive" ) )
					{
						int na = strlen(packet.pinfo) - 2; // skip ',s'
						if ( na > 0 )
						{
							for ( int i=0; i<na; i++ )
							{
								int a = 0;
								k += getInt( &b[k], a );
								listener.call( a );
							}
						}
					}
				}			
			}
			return n;
		}
		
		
		static void debug( char * b, int n, FILE * str=stderr )
		{
			for ( int i=0; i<n; i++ )
			{
				if( b[i]>0x20 && b[i]<'z' )
				{
					fprintf( str, " %c", b[i] );
				}
				else
				{
					fprintf( str, " %02x", b[i] );
				}
			}
			fprintf( str,  "\n" );
		}

		int getSequence()
		{
			return seq;
		}

		// 4 byte padded strings !
		int getString( char * data, char **output )
		{
			size_t l = strlen(data)+1;
			*output = data;
			return align4(l);
		}

		//! big endian !
		int getFloat( char * data, float & output )
		{
			char d[4];
			d[0] = data[3];
			d[1] = data[2];
			d[2] = data[1];
			d[3] = data[0];
			float o = *((float*)(d));
			output = o;
			return 4;
		}

		//! big endian !
		int getInt( char * data, int & output )
		{
			int o =  (*(data  ) << 24)
				   + (*(data+1) << 16)
				   + (*(data+2) << 8)
				   + (*(data+3) );
			output = o;
			return 4;
		};

		int parseSet( char * data, Object *o, int mode )
		{
			int off = 0;
			off += getInt( data+off, o->id );
			if ( mode == 1 )
				off += getInt( data+off, o->fi );
			else
				o->fi = -1;
			off += getFloat( data+off, o->x );
			off += getFloat( data+off, o->y );
			if ( mode == 1 )
				off += getFloat( data+off, o->a );
			off += getFloat( data+off, o->X );
			off += getFloat( data+off, o->Y );
			if ( mode == 1 )
				off += getFloat( data+off, o->A );
			off += getFloat( data+off, o->m );
			if ( mode == 1 )
				off += getFloat( data+off, o->r );
			return off;
		}

		Parser(Listener & l)
			: listener(l) 
			, sock(SOCK_DGRAM, IPPROTO_UDP)
			, thread(0)
			, locked(false)
		{
			start();
		}

		~Parser()
		{
			stop();
		}


		void run() 
		{
			char b[0xffff];
			for (int ever=1; ever; ever)
			{
				PROFILE;
				if ( this->sock.udpDataReady() )
				{
					int n = this->sock.read(b,0xfffe);
					if ( n > 0 )
						this->parse( b, n );
				}
				else
				{
					printf( "socket not connected.\n" ); 
				}
			}
		}


		
		static unsigned long WINAPI serve_thread( void * ptr ) 
		{
			Tuio::Parser * tuio = ((Tuio::Parser*)ptr);
			tuio->run();
			return 1;
		}

		bool start()
		{
			sock.listen( 3333 );
			sock.write("hiho!\r\n");

			unsigned long id;
			thread = CreateThread( 0, 0,
				(LPTHREAD_START_ROUTINE)serve_thread,
				(void*)this,
				0,
				&id );
			if ( thread ) 
			{
				return 1;
			}
			return 0;
		}

		bool stop()
		{
			//~ sock.close();

			if ( thread ) 
			{
				TerminateThread( thread, 0 );
				CloseHandle( thread );
				thread = 0;
				return 1;
			}
			return 0;
		}

		Listener & listener;

		int seq;
		w32::Socket sock;

		HANDLE thread;

		bool locked;
	}; //Parser


	MessageQueue::MessageQueue(Listener &l)
	{
		parser = ( new Tuio::Parser(l) );
	}
	MessageQueue::~MessageQueue()
	{
		delete parser;
	}

	
}; // Tuio



