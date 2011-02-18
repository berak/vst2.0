#ifndef __syn_h_onboard__
#define __syn_h_onboard__

#include "sound.h" // consts
//// fwd:
//class XMLNode;

struct Item
{
    typedef bool (*Processor) (Item *item, Item *parent, const void *data);
    
	enum Types
	{
		IT_CURSOR = 0,
		IT_MIDI   = 1,
		IT_AMP    = 2,
		IT_MIDI_SEQ  = 3,

		//IT_SYN,
		
		// sound buffers:
		//IT_SOUND  = 2,
		//IT_FILTER = 3,

		// fiducial
		IT_VOLUME     = 7, // the default conroller
		//IT_SPEED      = 8,
		//IT_FREQUENCY  = 9,
		//IT_QUALITY    = 11,

		IT_MIDI_PROG  = 21,
		IT_MIDI_NOTE  = 22,
		IT_SIN        = 23,

		IT_MAX		  = 24 // limited by num fiducials of the small amoeba set
	};

	inline bool hasType( int id, int t )
	{
		return ( ( conTypes[id] & (1<<t) ) != 0 );
	}
	inline void setType( int id, int t )
	{
		conTypes[id] |= ( 1<<t ); 
	}

    float *v;
    float x,y,a,A;
    int type,bSize,id;
	int lostCycles;
	int tex;
	int _refs;

   	Item *con[MaxConnections];
	unsigned int conTypes[MaxConnections];
	float conMult[MaxConnections];
	int ncon;

	int release();
	int addref();

	virtual float value() { return a; }

	virtual const char * typeName();
	
	float distance( const Item * it );

    virtual bool connect( Item * upstream );
    virtual bool disconnect( Item * upstream );
	virtual bool autoDisconnect( float radius );

	virtual bool onConnect( Item * downstream );
    virtual bool onDisconnect( Item * downstream );

    bool process( Processor pro, Item *parent, const void * data, bool pre );

	virtual bool processAudio( float ** buffer, int nBuffer, int nElems ) { return 1; }
	virtual bool updateFrame( float frameTime, int frameNo ) { return 1; }

	Item( int bufSize, int type, int nc=0, int i=-1 );
	virtual ~Item() ;
};


int _nIt();
int numItems();
Item * getItem( int index );
Item * createSynItem( int type );
int clearItems();


#endif // __syn_h_onboard__

