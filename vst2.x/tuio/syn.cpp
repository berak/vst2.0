#include <map>
#include <vector>
#include <stdio.h>
#include <math.h>
#include <windows.h> // FOR MIDI ??? !!!*****
#include <time.h> // for random
#include "syn.h"
#include "source/xmlParser.h" // xml
#include "Profile.h"

#define SYNRELEASE(x) if(x)x->release(); x=0;

float random( float lo, float hi )
{
    static time_t t_init=0;
    if ( ! t_init ) srand( (unsigned int)time(&t_init) );
    float fval = (float)(rand() & RAND_MAX) / (float)RAND_MAX;
    return (lo + (fval * (hi - lo)));
}
char __b[256];
char*str( int i )   { sprintf(__b,"%i",i);return __b; }
char*str( float f ) { sprintf(__b,"%.3f",f);return __b; }
void num( const char*s, int &i )   { if(s) sscanf(s,"%i",&i); }
void num( const char*s, float &f ) { if(s) sscanf(s,"%f",&f); }

int error(const char*a,const char*b="",int e=0 )
{
	printf( "Error:%s%s\n", a,b );
	return e;
}

std::vector < Item * > all;


#define $$$ printf("%4d %-8x %s\n", __LINE__, this,__FUNCTION__ );
#define $$(x) printf("%4d %-8x %s %s\n", __LINE__, this,__FUNCTION__, x );

static int _nItems = 0;
int _nIt() 
{
	return _nItems; 
}

inline float clamp(float v, float lo, float hi )
{
	if ( v<lo ) return lo;
	if ( v>hi ) return hi;
	return v;
}

float Item::distance( const Item * it )
{
	float dx = x - it->x;
	float dy = y - it->y;
	float ds = dx*dx + dy*dy;
	return sqrt(ds);
}

bool Item::autoDisconnect( float radius )
{
	bool dirty = false;
	for ( int i=0; i<ncon; i++ )
	{
		if ( ! con[i] ) 
			continue;
		if ( radius )
		{
			float d = distance(con[i]);
			if ( d < radius )
				continue;
		}
		con[i]->onDisconnect( this );
		SYNRELEASE(con[i]);
		dirty=true;
	}
	return dirty; 
}

bool Item::onDisconnect( Item * downstream )
{ 
	return true;
}

bool Item::onConnect( Item * downstream )
{ 
	return true;
}

bool Item::connect( Item * upstream )
{ 
	if ( ! upstream )
	{
		return 0;
	}
	for ( int i=0; i<ncon; i++ )
	{
		if ( hasType( i , upstream->type ) )
		{
			if ( con[i] ) 
				continue;

			if ( upstream->onConnect( this ) )
			{
				con[i] = upstream;
				con[i]->addref();
				return 1;
			}
		}
	}
	return 0; 
}

bool Item::disconnect( Item * upstream )
{ 
	if ( ! upstream )
	{
		return 0;
	}
	for ( int i=0; i<ncon; i++ )
	{
		if ( con[i] == upstream )
		{
			con[i]->onDisconnect( this );
			SYNRELEASE(con[i]);
			return 1;
		}
	}
	return 0; 
}

const char * Item::typeName()
{
	return "Item";
}
//


bool Item::process( Item::Processor pro, Item *parent, const void * data, bool pre )
{
	//printf(__FUNCTION__ " id(%d)\ttype(%d)\n",id,type );
	bool r = 1;
	if ( pre )
		r = pro(this, parent, data);
	if ( ! r ) 
		return 0;
	
	for ( int i=0; i<ncon; i++ )
	{
		if ( ! con[i] )
			continue;

		r = con[i]->process(pro,this,data,pre);
		if ( ! r ) 
			break;
	}

	if ( ! pre )
		r = pro(this, parent, data);
	
	return r;
}


Item::Item( int bufSize, int type, int nc, int i )
	: v( new float[bufSize] )
	, type(type)
	, bSize(bufSize)
	, x(random(0.0,1.0))
	, y(random(0.0,1.0))
	, a(0)
	, A(1.0)
	, ncon(nc)
	, lostCycles(0)
	, tex(0)
	, _refs(1)
{
	static int iiiihhh = 0;
	if ( i==-1 )
		id = iiiihhh ++;
	else id = i;

	memset(con,0,MaxConnections*sizeof(Item*));
	memset(conTypes,0,MaxConnections*sizeof(unsigned int));
	//memset(conMult,1.0f,MaxConnections*sizeof(float));
	for ( int i=0; i<MaxConnections; i++ )
		conMult[i] = 1.0f;
	for ( int i=0; i<bSize; i++ )
		v[i] = 0.0f;

	all.push_back( this );

	_nItems ++;
}

Item::~Item() 
{
	delete [] v ;

	// unsubscribe the list:
	std::vector<Item*>::iterator it=all.begin();
	for ( ; it!=all.end(); it++ )
	{
		Item * c = *it;
		if ( c == this )
		{
			all.erase(it);
			break;
		}
	}

	// clear pins:
	autoDisconnect(0);

	// search for downstream con
	for ( it=all.begin(); it!=all.end(); it++ )
	{
		Item * c = *it;
		if ( c->disconnect(this) )
			break;
	}

	_nItems --;
}

int Item::release()
{
	_refs--;
	if ( _refs == 0 )
	{
		printf( "-@@ %s(%i)\n", typeName(), id );
		delete this;
		return 0;
	}
	return _refs;
}
int Item::addref() 
{
	return ++_refs; 
}


struct Controller
    : Item
{
	
	Controller( int t=Item::IT_VOLUME )
		: Item(0,t,0)
	{
	}

	
	virtual const char * typeName()
	{
		return "Controller";
	}
};

struct Sin
    : Item
{
	float _val;

	Sin()
		: Item(0,IT_SIN,0)
		, _val(0)
	{
	}

	virtual const char * typeName()
	{
		return "Sinus";
	}
	virtual bool updateFrame( float t, int f)
	{
		_val = 0.5f + 0.5f * sin( t * (a) );
		return 1;
	}
	virtual float value()
	{
		return _val;
	}
	
};



//struct Sound
//    : Item
//{
//	float frq,vol;
//	Sound()
//		: Item(SizeAudioBuffer,Item::IT_SOUND,2)
//		, frq(100)
//		, vol(0.5)
//	{
//		setType( 0, IT_VOLUME );
//		setType( 1, IT_FREQUENCY );
//		setType( 1, IT_SIN );
//	}
//   
//	virtual const char * typeName()
//	{
//		return "Sound";
//	}
//
//	virtual bool updateFrame( float t, int f)
//	{
//		frq = ( 1.0f + a * 100.0f );
//
//		if ( con[0] ) vol = con[0]->value();
//		float fmul = 1.0f;
//		if ( con[1] ) fmul = con[1]->value();
//		
//		for ( int i=0; i<bSize; i++ )
//		{
//			v[i] = clamp( vol * sin((i % int(frq) ) / (frq*fmul)), -1, 1 );
//		}
//		return 1;
//	}
//};
//
struct Amplifier
    : Item
{
	float vol;
	bool needsClear;

	Amplifier()
		: Item(SizeAudioBuffer,Item::IT_AMP,2)
		, vol(0.5)
		, needsClear(0)
	{
		//setType( 0, IT_SOUND );
		//setType( 0, IT_FILTER );
		setType( 1, IT_VOLUME );
	}
   
	virtual const char * typeName()
	{
		return "Amplifier";
	}

	bool processAudio( float ** buffer, int nBuffer, int nElems )
	{
		if ( con[0] ) 
		{
			con[0]->processAudio( buffer, nBuffer, nElems );

			for ( int b=0; b<nBuffer; b++ )
			{
				float *chan = buffer[b];
				for ( int i=0; i<nElems; i++ )
				{
					*chan++ *= vol;
				}
			}
		}
		return 1;
	}

	virtual bool updateFrame( float t, int f)
	{
		PROFILE;

		vol = a;
		if ( con[1] )
		{
			vol = clamp( vol * con[1]->value(), 0, 1 );
		}

		return 1;
	}
};

//struct Filter
//    : Item
//{  
//	
//	Filter()
//		: Item(SizeAudioBuffer,Item::IT_FILTER,3)
//	{
//		setType( 0, IT_SOUND );
//		setType( 0, IT_FILTER );
//		setType( 1, IT_FREQUENCY );
//		setType( 2, IT_QUALITY );
//	}
//
//	virtual const char * typeName()
//	{
//		return "Filter";
//	}
//
//	virtual bool updateFrame( float t, int f)
//	{
//		if ( ! con[0] ) return 0;
//		//if ( ! con[0]->bSize==bSize ) return 0;
//		//for ( int i=0; i<bSize; i++ )
//		//{
//		//	v[i] = con[0]->v[i];
//		//}
//		return 1;
//	}
//};

struct Cursor
    : Item
{
	Cursor()
		: Item(1,Item::IT_CURSOR,0)
	{
		A = a = 1.0f;
	}
   
	virtual const char * typeName()
	{
		return "Cursor";
	}
	virtual bool onDisconnect( Item * downstream )
	{ 
		A = a = 0;
		return true;
	}

	virtual bool onConnect( Item * downstream )
	{ 
		A = a = 1;
		return true;
	}
};


namespace Midi
{
	unsigned int  _pitch[16];
	unsigned char _ctl[16][128];
	unsigned char _note[16][128];
//	Callback 	* _call  = 0;
	unsigned int		  _e_cmd = 0;	//! cached event for running status
	unsigned int		  _e_chn = 0;	//! cached event for running status
	unsigned int		  _e_b0  = 0;	//! cached event for running status
	unsigned int		  _e_b1  = 0;	//! cached event for running status


	// event callback:
	void CALLBACK _midiInProc(
		HMIDIIN hMidiIn,  
		unsigned int wMsg,        
		DWORD dwInstance, 
		DWORD dwParam1,   
		DWORD dwParam2    
	)
	{
		// get the bytes from lparam into array ;-) 
		BYTE  in_buf[4];
		memcpy( in_buf, &dwParam1, 4 );

		if ( in_buf[0] < 128 ) 
		{
			// data<127, runningstatus
			// 2-byte message ( running status on):
			_e_b0 = in_buf[0];        // 1st & 2nd byte
			_e_b1 = in_buf[1];
		} 
		else 
		{
			// 3-byte message ( running status off ):
			_e_chn  = (in_buf[0] & 0x0f); //channels[0-15] !!!
			_e_cmd  = (in_buf[0] & 0xf0);
			_e_b0   = in_buf[1];     //  2nd & 3rd byte
			_e_b1   = in_buf[2];
		}

		switch( _e_cmd )
		{
		case 0x90://note_on
			_note[_e_chn][_e_b0] = _e_b1;
			break;
		case 0xb0://ctl
			_ctl[_e_chn][_e_b0] = _e_b1;
			break;
		case 0xe0://pitch ///PPP fixme!
			_pitch[_e_chn] = (_e_b0<<7) | _e_b1; //float!!!!vl;
			break;
		}
		
		//if ( _call )
		//{
		//	_call->onMidiEvent( _e_cmd, _e_chn, _e_b0, _e_b1 );
		//}
	}

	struct CDevice
	{
		HMIDIIN       _midiIn;
	    char          _err[1024];

		CDevice()
		{
//			_call = 0;
			_clear();
		}
		
		virtual ~CDevice()
		{
			_close();
		}


		bool _error( MMRESULT res ) 
		{
			if ( ! res )
				return 1;
			midiInGetErrorText( res, _err, 1024 );
			printf( "Midi Error: %s\n", _err );
			return 0; //MessageBox( NULL, _err, "midi in", MB_OK );     
		}

		bool _clear()
		{
			_midiIn = 0;
			_e_chn = 0;
			_e_cmd = 0;
			_e_b0 = 0;
			_e_b1 = 0;

			_err[0] = 0;

			memset( _ctl,   0, 128*16 );
			memset( _note,  0, 128*16 );
			memset( _pitch, 0, 16*sizeof(unsigned int) );

			return 1;
		}


		bool  _open( unsigned int _deviceID )
		{
			if ( _midiIn ) 
			{
				return true; // shared
			}

			MMRESULT res = 0;
			DWORD_PTR hinst = (DWORD_PTR)::GetModuleHandle(NULL);

			res = midiInOpen( &_midiIn, _deviceID, (DWORD_PTR)_midiInProc, hinst, CALLBACK_FUNCTION );
			if ( res ) return _error( res );   

			res = midiInStart( _midiIn );
			return _error( res ) ;   
		}


		bool _close() 
		{
			MMRESULT res;
			if ( ! _midiIn ) return 0;
			res = midiInStop( _midiIn );
			if ( res ) return _error( res );   
			res = midiInClose( _midiIn );
			if ( res ) return _error( res );   
			_midiIn = 0;
//			_call = 0;
			return 1;
		}



		bool _reset(unsigned int _deviceID) 
		{
			MMRESULT res;
			if ( ! _midiIn ) return 0;
			res = midiInStop( _midiIn );
			if ( res ) return _error( res );   
			res = midiInReset( _midiIn );
			if ( res ) return _error( res );   
			res = midiInStart( _midiIn );
			return  _error( res );   
		}


		bool _getLastError(char * buf)
		{
			if ( _err[0] == 0 ) return false;
			strcpy( buf, _err );
			_err[0] = 0;
			return 1;
		}

		
		virtual unsigned int init( unsigned int n )
		{
			_clear();
			unsigned int r = _open(n);
			if ( ! r )
			{
				printf(__FUNCTION__ " : no device for id %i.\n",n);
				for ( unsigned int i=0; i<8; i++ )
				{
					if ( i==n ) continue;
					r = _open( i );
					if ( r )
					{
						printf(__FUNCTION__ " : created device %i instead.\n",i);
						return r;
					}
				}
				return 0;
			}
			printf(__FUNCTION__ " : created device %i .\n",r);

			return r;
		}

		
		//virtual unsigned int setCallback(Callback * cb) 	
		//{
		//	_call = cb;
		//	return 1;
		//}

		
		virtual unsigned int getState( unsigned int cmd, unsigned int channel, unsigned int b0 )	
		{
			switch( cmd )
			{
				case 0x90:
					return _note[channel][b0];
				case 0xb0:
					return _ctl[channel][b0];
				case 0xe0:
					return _pitch[channel];
			}
			return 0;
		}

	}; // CDevice
} // Midi

struct MidiKeyboard
    : Item
	, Midi::CDevice
{
	MidiKeyboard()
		: Item(3,1,0)
	{
		Midi::CDevice::init(1);
	}

	virtual const char * typeName()
	{
		return "MidiKeyboard";
	}
	virtual bool updateFrame(float t, int f)
	{
		v[0] = float(Midi::_e_cmd | Midi::_e_chn) / 255.0f;
		v[1] = float(Midi::_e_b0) / 127.0f;
		v[2] = float(Midi::_e_b1) / 127.0f;
		A = a = 1.0f;
		return 1;
	}
	virtual float value()
	{
		int i = 1;
		if ( Midi::_e_cmd == 0xb0 )
			i = 2;
		A *= 0.9f;
		return float( v[i] );
	}
};

struct MidiSequencer
    : Item
{
	float s[256][3];
	int pos;
	int len;
	int speed;

	MidiSequencer()
		: Item(3,Item::IT_MIDI_SEQ,2)
		, pos(0)
		, len(8)
		, speed(600)
	{
		for ( int i=0; i<len; i++ )
		{
			s[i][0] = (float(0x90) / 255.0f);
			s[i][1] = random(0.1f,0.9f);
			s[i][2] = random(0.3f,0.7f);
		}
	}

	virtual const char * typeName()
	{
		return "Sequencer";
	}
	virtual bool updateFrame(float t, int f)
	{
		int MaxSpeed = 1000;
		speed = int(MaxSpeed*a);
		if ( speed > MaxSpeed )
			speed = MaxSpeed;
		pos = int(f/(MaxSpeed/speed)) % len;

		if ( con[0] )
		{
			// len
			len = int(127.0f*con[0]->value());
		}
		//if ( con[1] )
		//{
		//	// pos
		//	speed = int(127.0f*con[1]->value());
		//}
		if ( con[1] )
		{
			// note
			//s[pos][0] = (float(0x90) / 255.0f);
			s[pos][1] = int(127.0f*con[1]->value());
		}

		v[0] = s[pos][0];
		v[1] = s[pos][1];
		v[2] = s[pos][2];

		A = 1.0f;
		return 1;
	}

	virtual float value()
	{
		A *= 0.9f;
		return float( v[1] );
	}
};




//
// this is more like a midi 'state' ..
//


struct MidiEvent
    : Item
{
	int mapId;
	float vel;

	MidiEvent( int fid, int b0, int b2=0, int m=1 )
		: Item(3,fid,0)
		, mapId(m)
	{
		v[0] = b0 / 255.0f;
		v[2] = b2 / 127.0f;
		vel = v[2];
	}

	virtual const char * typeName()
	{
		if ( type == Item::IT_MIDI_NOTE ) return "Note";
		if ( type == Item::IT_MIDI_PROG ) return "Prog";
		return "Mi??";
	}
	virtual bool updateFrame(float t, int f)
	{
		//v[2] = A;//( A != 0.0f ? vel : 0 );
		v[mapId] = a;
		return 1;
	}
	virtual float value()
	{
		return float( v[mapId] );
	}
};


// 'global' interface:


int clearItems()
{
	size_t i=0;
	while ( ! all.empty() )
	{
		Item * last = all.back();
		if( last ) last->release();
		else break;
		i ++;
	}
	return i;
}
int numItems()
{
	return all.size();
}
Item * getItem( int index )
{
	return all.at(index);
}



Item * createSynItem( int type )
{
	switch( type )
	{
		case Item::IT_MIDI:
		{
			return new MidiKeyboard;
		}
		//case Item::IT_SYN:
		//{
		//	return new Syn;
		//}
		case -1:
		case Item::IT_CURSOR:
		{
			return new Cursor;
		}
		case Item::IT_AMP:
		{
			return new Amplifier;
		}
		case Item::IT_MIDI_SEQ :
		{
			return new MidiSequencer();
		}
		case Item::IT_MIDI_NOTE :
		{
			return new MidiEvent( type, 0x90, 0x7f );
		}
		case Item::IT_MIDI_PROG:
		{
			return new MidiEvent( type, 0xc0, 0 );
		}
		case Item::IT_SIN:
		{
			return new Sin;
		}

		default:
		//case Item::IT_FREQUENCY:
		//case Item::IT_QUALITY:
		//case Item::IT_VOLUME:
		{
			if ( type > 6 && type < 32 )
				return new Controller(type);
		}
	}
	//printf("Error: " __FUNCTION__ "stumbled over type %d\n", type);
	return 0; 
}

