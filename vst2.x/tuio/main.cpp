
#include <windows.h>
#include <stdio.h>
#include <gl/gl.h>
#include "gl_tk.h"
#include "syn.h"
#include "tuio.h"
#include "vsthost.h"
#include "bmp/EasyBMP.h"
//#include "ds/dev_dsound.h"
#include "dev_asio.h"
#include "w32.h"
#include "Profile.h"

//#include <vector>

#pragma warning( disable:4099 )
#include "source/xmlParser.h"

#pragma warning( disable:4244 )
#pragma warning( disable:4018 )
#pragma warning( disable:4305 )
#pragma warning( disable:4996 )

#define KONSOLE



LRESULT CALLBACK WndProc (HWND hWnd, UINT message,
WPARAM wParam, LPARAM lParam);
void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC);
void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC);
extern float random( float lo, float hi );

// xml-configurable vars:

// window
float wWidth=400;
float wHeight=300;
float scale = 1.0;
float matchRadius = 0.37f;
char * bgTexture = 0;
// lines
float lColor[4] = { 0.5, 0.7, 0.7, 1.0 };
int lWidth = 4;

// text
float tColor[4] = { 0.5, 0.7, 0.7, 1.0 };
char * tName = "xirod";
int tSize = 10;
// Item time to live after it's corresponding Tuio was lost
int TTL = 20;
// asio
int SampleRate      = 44100;
char *asioDriverName = "ASIO DirectX Full Duplex Driver";


int frame;
int texId[32] = {0};
char * texNames[32] = {0};

int numSounds = 0;

//DSound dsound;
DAsio  asio;

// oops, mutithreading:
w32::CriticalSection critSec;
		
unsigned int loadImage(const char * fname)
{
	if ( ! fname )
	{
		return 0;
	}
	BMP bmp;
	if ( ! bmp.ReadFromFile(fname) )
	{
		printf( "not loaded : %s\n",fname );
		return 0;
	}

	int w = bmp.TellWidth();
	int h = bmp.TellHeight();
	int d = bmp.TellBitDepth() / 8;
    RGBApixel* pix = bmp(0,0);
    char bytes[0x7ffff], *b=bytes;
    for ( int j=0; j<h; j++ )
    {
        for ( int i=0; i<w; i++ )
        {
			RGBApixel pix = bmp.GetPixel(i, j);
            *b++ = pix.Red;
            *b++ = pix.Green;
            *b++ = pix.Blue;
			if ( d == 4 )
				*b++ = pix.Alpha;            
        }        
    }
    size_t i = RGL::texCreate( w, h, d, bytes, true );;
    printf( "created : %d [%d %d %d] %s\n", i, w,h,d,fname );
    return i;
}





//
// collect messsages from the tracking system,
// so i can handle them on a 'per frame' basis.
//
struct TuioMessage : Tuio::Listener
{
	int nAlive,nMessages;

	int alive[512];
	Tuio::Object messages[512];
	
	Tuio::MessageQueue queue;

	TuioMessage() : queue(*this) , nAlive(0),nMessages(0) {}

	virtual void call( Tuio::Object & o )
	{
		if ( nMessages > 511 ) return;
		messages[nMessages++]=o;
	}
	virtual void call( int aliveId ) 
	{
		if ( nAlive > 511 ) return;
		if ( ! isAlive(aliveId) )
			alive[nAlive++] = aliveId;
	}
	virtual void startBundle()
	{
		clear();
	}

	void clear()
	{
		nAlive=0;
		nMessages=0;
	}

	int numMessages()
	{
		return nMessages;
	}

	Tuio::Object getMessage(int i)
	{
		if ( (i >= nMessages) || (i<0) )
		{
			printf(__FUNCTION__ "  index error!\n" );
			//__asm { int 3 };
			return Tuio::Object();
		}
		return messages[i];
	}

	int numAlive()
	{
		return nAlive;
	}

	bool isAlive( int id )
	{
		for ( size_t i=0; i<nAlive; i++ )
		{
			if ( alive[i] == id )
				return 1;
		}
		return 0;
	}

} tuio;




bool hit( Item * it, float x, float y, float r )
{
    float dx = x - it->x;
    float dy = y - it->y;
    float ds = sqrt(dx*dx+dy*dy);  
    return ds < r;
}


Item * connectItem( Item * it, float dist )
{
	PROFILE;
    Item * match = 0;
    float dm = 0xffff;
	for ( size_t i=0; i<numItems(); i++ )
	{
        Item * target = getItem(i);
		if ( it->id == target->id ) 
			continue;
		float d = it->distance( target );
		if ( d > dist )
			continue;
		if ( d > dm )
			continue;

		if ( it->connect( target ) )
		{
			for ( size_t j=0; j<numItems(); j++ )
			{
		        Item * old = getItem(j);
				if ( old->id == it->id || old->id == target->id )
					continue;
				if ( old->disconnect( target ) )
					break;
			}
			match = target;
			dm = d;
		}
    }
    return match;
}


bool updateConnections( float radius )
{
	PROFILE;
	bool dirty = 0; 

	for ( size_t i=0; i<numItems(); i++ )
	{
		Item *  it = getItem(i);
		dirty |= it->autoDisconnect( radius );;
		dirty |= (0!=connectItem( it, radius ));;
    }
	return dirty;
}

//
// items may disappear for a short time (mistracking),
// and reappear later with a new id.
// so if the id-check fails, try to find the nearest item
// of the same type whithin a search distance
//
Item * matchItem( const Tuio::Object & o, float dist )
{
	PROFILE;

	Item * match = 0;
    float dm = 0xffff;
	float x = 1.0 - o.x;
	float y = 1.0 - o.y;
	for ( size_t i=0; i<numItems(); i++ )
	{
        Item * it = getItem(i);
        if ( it->id == o.id )
        {
            return it;
        }

		int type = (o.fi==-1 ? Item::IT_CURSOR : o.fi);
        if ( it->type != type )
        {
            continue;
        }

		//if ( it->id < o.id )
		//{
		//	continue;
		//}
		// // .. counter toggles direction at 1000...

		float d = sqrt( (it->x-x)*(it->x-x)+(it->y-y)*(it->y-y) );
        if ( dist < d )
            continue;
        if ( dm < d )
            continue;
        match = it;
    }
    return match;
}


void setItem( Item * it, const Tuio::Object & o )
{
	if ( ! it  ) return ;
	it->id = o.id;
    it->x  = 1.0 - o.x;
    it->y  = 1.0 - o.y;
	if ( o.fi>Item::IT_MIDI_SEQ )	
	{
		it->A  = o.A;
	}
	it->a = (o.a / 6.28);
}


int getIntAtt( XMLNode & xml,const char * a, int v=0 )
{
	const char * s = xml.getAttribute(a);
	if ( s )
	{
		int i=0;
		if ( sscanf( s, "%i", &i ) == 1 )
		{
			return i;
		}
	}
	return v;
}
float getFloatAtt( XMLNode & xml,const char * a, float v=0 )
{
	const char * s = xml.getAttribute(a);
	if ( s )
	{
		float f=0;
		if ( sscanf( s, "%f", &f ) == 1 )
		{
			return f;
		}
	}
	return v;
}
const char * getStringAtt( XMLNode & xml,const char * a, const char * v=0 )
{
	const char * s = xml.getAttribute(a);
	if ( s )
	{
		return s;
	}
	return v;
}

static XMLNode & getRoot()
{
	static XMLNode _root;
	if ( _root.isEmpty() )
		_root = XMLNode::parseFile( "syn.xml" );
	return _root;
}


void setupInputs( Item * it, XMLNode & pin, int conId )
{
	if ( pin.isEmpty() )
		return;
	int nCons = pin.nChildNode();
	int k=0;
	while ( k<nCons )
	{
		XMLNode con = pin.getChildNode( "con", &k );
		if ( con.isEmpty() )
			return;

		int fid = getIntAtt( con, "fiducial" );
		it->setType( conId, fid );
	}
}
void loadSynSettings( XMLNode & root )
{
	if ( root.isEmpty() )
		return;
	XMLNode settings = root.getChildNode( "settings" );
	if ( settings.isEmpty() )
		return;

	XMLNode asio = settings.getChildNode( "asio" );
	if ( ! asio.isEmpty() )
	{
		asioDriverName = (char*)getStringAtt( asio, "driver", "ASIO DirectX Full Duplex Driver" );
		SampleRate = getIntAtt( asio, "samplerate", 44100 );
	}
	XMLNode window = settings.getChildNode( "window" );
	if ( ! window.isEmpty() )
	{
		wWidth = getIntAtt( window, "width", 400 );
		wHeight = getIntAtt( window, "height", 300 );
		bgTexture = (char*)getStringAtt( window, "bgTexture", 0 );
	}
	XMLNode text = settings.getChildNode( "text" );
	if ( ! text.isEmpty() )
	{
		tName = (char*)getStringAtt( text, "font", "lucida" );
		tSize = getIntAtt( text, "size", 10 );
		tColor[0] = getFloatAtt( text, "r", 0.0  );
		tColor[1] = getFloatAtt( text, "g", 0.0  );
		tColor[2] = getFloatAtt( text, "b", 0.0  );
		tColor[3] = getFloatAtt( text, "a", 1.0  );
	}
	XMLNode lines = settings.getChildNode( "lines" );
	if ( ! lines.isEmpty() )
	{
		lWidth = getIntAtt( lines,  "width", 4 );
		lColor[0] = getFloatAtt( lines, "r", 0.0  );
		lColor[1] = getFloatAtt( lines, "g", 0.0  );
		lColor[2] = getFloatAtt( lines, "b", 0.0  );
		lColor[3] = getFloatAtt( lines, "a", 1.0  );
	}
	XMLNode graph = settings.getChildNode( "graph" );
	if ( ! graph.isEmpty() )
	{
		TTL = getIntAtt( graph, "ttl", 20 );
	}
}

Item * loadXml(  int type )
{
	PROFILE;
	Item * it = 0;
	XMLNode & root = getRoot();
	if ( root.isEmpty() )
		return 0;

	XMLNode items = root.getChildNode( "items" );
	if ( items.isEmpty() )
		return 0;

	int n = items.nChildNode();
	int i=0;
	while ( i<n && (!it) )
	{
		XMLNode plug = items.getChildNode( "fiducial", &i );
		if ( plug.isEmpty() )
			break;


		int fid = getIntAtt( plug, "id" );
		if ( fid != type )
		{
			continue;
		}

		const char * tex = getStringAtt( plug, "tex" );
		if ( ! texId[ fid ] )
		{
			texId[fid] = loadImage( tex );
		}

		bool isVstHost = false;
		int nInputs = plug.nChildNode();
		const char * dllName = plug.getAttribute( "plugin" );
		if ( dllName )
		{
			int ma = getIntAtt( plug, "map", -1 );

			it = createVstHost( dllName, fid, nInputs, ma );
			isVstHost = true;
		}
		else 
		{
			it = createSynItem( fid );
			isVstHost = false;
		}

		if ( ! it ) 
		{
			break;
		}
		int j=0;
		while ( j<nInputs )
		{
			XMLNode pin = plug.getChildNode( "pin", &j );
			if ( pin.isEmpty() )
				continue;

			it->conMult[ j - 1 ] = getFloatAtt( pin, "multiply", 1.0f );

			if ( isVstHost )
				setVstHostMapping( it, getIntAtt( pin, "param" ), (j - 1) );

			setupInputs( it, pin, j-1 );
		}
		break;
	}
	return it;
}



inline float saturate(float input, float fMax)
{
	static const float fGrdDiv = 0.5f;
	float x1 = fabsf(input + fMax);
	float x2 = fabsf(input - fMax);
	return fGrdDiv * (x1 - x2);
}


//
// this is called from asio
//
short ** doAudioAsio(int buffSize)
{
	PROFILE;
	w32::Lock lock(&critSec);

	static const int numOutputs = 2;
	static const int kBlockSize = 0xffff;

	// 8 channels, just to be safe.
	static float * outputs[] = 
	{ 
		new float[kBlockSize], new float[kBlockSize], new float[kBlockSize], new float[kBlockSize],
		new float[kBlockSize], new float[kBlockSize], new float[kBlockSize], new float[kBlockSize]
	};
	static float * sum[] = 
	{ 
		new float[kBlockSize], new float[kBlockSize]
	};

	static short * b[] = 
	{ 
		new short[kBlockSize], new short[kBlockSize]
	};

	memset( outputs[0], 0, buffSize * sizeof(float) );
	memset( outputs[1], 0, buffSize * sizeof(float) );
	memset( sum[0], 0, buffSize * sizeof(float) );
	memset( sum[1], 0, buffSize * sizeof(float) );
	memset( b[0], 0, buffSize * sizeof(short) );
	memset( b[1], 0, buffSize * sizeof(short) );

	// sum up audio outputs:
	int numVoices = 0;
	for ( size_t i=0; i<numItems(); i++ )
	{
        Item * it = getItem(i);
		if ( it->type != Item::IT_AMP )
            continue;

		if ( it->con[0] )
		{
			it->processAudio( outputs, numOutputs, buffSize );
			for ( int o=0; o<numOutputs; o++ )
			for ( int n=0; n<buffSize; n++ )
			{
				sum[o][n] += outputs[o][n];
			}
			numVoices ++;
		}
    }

	// mix to asio buffer:
	if ( numVoices )
	{
		for ( int o=0; o<numOutputs; o++ )
		for ( int n=0; n<buffSize; n++ )
		{
			b[o][n] = (short)((saturate( sum[o][n], 1.0f ) * 32767.505f) - .5f);
		}
	}
	return b;
}

//void doAudioDSound()
//{
//	static const int numOutputs = 2;
//	static const int kBlockSize = 0xffff;
//	static float * outputs[] = { new float[kBlockSize], new float[kBlockSize] };
//	static short b[0xffff];
//
//	int v = 0;
//	for ( size_t i=0; i<numItems(); i++ )
//	{
//        Item * it = getItem(i);
//		if ( it->type != Item::IT_AMP )
//            continue;
//
//		if ( it->con[0] )
//		{
//			it->processAudio( outputs, 2, 1024 );
//
//			for ( int n=0; n<it->bSize; n++ )
//			{
//				b[n] = (short)((saturate( outputs[0][n], 1.0f ) * 32767.505f) - .5f);
//			}
//			dsound.LoadVoice( v, b, it->bSize * 2 );
//		} else
//		{
//			// input lost, shutup sound
//			for ( int n=0; n<SizeAudioBuffer; n++ )
//			{
//				b[n] = 0;
//			}
//
//			dsound.LoadVoice( v, b, SizeAudioBuffer * 2 );
//			//dsound.Stop(v,true);
//		}
//
//		v++;
//    }
//
//}


bool loaded = false;
void print(const Tuio::Object & o)
{
	printf( "obj %2i %2i [%3.3f %3.3f %3.3f]\n",
            o.id,o.fi,o.x,o.y,o.a );
}

void parseMessages()
{
	PROFILE;

	// parse set messages:
    int n = tuio.numMessages();
	for ( int i=0; i<n; i++ )
    {
        const Tuio::Object & o = tuio.getMessage(i);
		int fid = ( o.fi == -1 ? 0 : o.fi );
		Item * it = matchItem( o, matchRadius );
        if ( ! it )
        {
			it = loadXml( fid );
			if ( ! it ) 
				it = createSynItem( fid );
			if ( ! it ) 
				continue;
		}
		setItem( it, o );
    }

	// update alive list:
	int a = tuio.numAlive();
	if ( a < numItems() )
	{
		for ( int i=0; i<numItems(); i++ )
		{
			Item * it = getItem(i);
			if ( ! tuio.isAlive( it->id ) )
			{
				it->autoDisconnect(0);

				if ( it->type == Item::IT_CURSOR )
				{
					it->a = 0; // say goodbye, reset trigger
				}
				it->lostCycles ++;			
				if ( it->lostCycles >= TTL )
				{
					printf( "--(%4d) lost(%d): %s \n", frame, it->id, it->typeName() );
					it->release();
					break;
				}
			}
			else
			{
				it->lostCycles = 0;			
			}
		}
	}
}



void update()            
{
	static float t=0;
	t += 0.02f;
	frame++;

    if ( ! loaded ) return;
    
	PROFILE;
	w32::Lock lock( &(critSec) );

	for ( size_t i=0; i<numItems(); i++ )
	{
        Item * it = getItem(i);
		it->updateFrame( t, frame );
	}

//	doAudioDSound();

    parseMessages();
    
    if ( ! numItems() )
    {
        return;
    }
    
	updateConnections( matchRadius );

	if ( frame % 100 == 0 )
	{
		printf( "items(%d) alive(%d) ", numItems(), tuio.nAlive );
		for ( int i=0; i<tuio.nAlive; i++ )
		{
			printf( " %d ", tuio.alive[i] );
		}
		printf( "\n" );
	}
}



void startTex()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

    glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA );
    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_ALPHA );
}

void endTex()
{
	glPopAttrib();
}


bool renderLines( Item * it, Item *parent, const void *data )
{
	PROFILE;
    if ( parent )
    {
        if ( parent->type<Item::IT_CURSOR )
            return 1;
        glColor3f (lColor[0], lColor[1], lColor[2]); 
        glBegin (GL_LINES);
         glVertex2f (wWidth * it->x / scale,   wHeight * it->y / scale);
         glVertex2f (wWidth * parent->x/scale, wHeight * parent->y/scale);
        glEnd ();
    }
    return 1;
}

bool renderIt( Item * it, Item *parent, const void *data )
{
	PROFILE;
    if ( it->type<Item::IT_CURSOR )
        return 1;
    float s = 17.5f;
    float x = wWidth  * it->x / scale;
    float y = wHeight * it->y / scale;
    glPushMatrix();
    glTranslatef( x, y, 0.0 );
    glRotatef (360.0*it->value(), 0.0f, 0.0f, -1.0f);

	int t = texId[it->type];
	if ( ! RGL::texIsValid(t) )
		t=1;

    RGL::texBind(t);
    RGL::drawTexQuad2D( -s,-s,s,s, 0,0,1,1 );
    glPopMatrix();
    return 1;
}


bool renderBg()
{
	if ( ! RGL::texIsValid(texId[25]) )
	{
		return 0;
	}

	startTex();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA );
    glDisable( GL_BLEND );
    glDisable( GL_ALPHA );

	glColor3f( 1,1,1 );
    RGL::texBind(texId[25]);

	glPushMatrix();
    glTranslatef( wWidth/2,wHeight/2, -30.0 );
	RGL::drawTexQuad2D( -wWidth/2,-wHeight/2,wWidth/2,wHeight/2, 0,0,1,1 );
    glPopMatrix();

	endTex();
    return 1;
}


bool renderNames( Item * it, Item *parent, const void *data )
{
	PROFILE;
    if ( it->type<Item::IT_CURSOR )
        return 1;
    float x = wWidth  * it->x / scale;
    float y = wHeight * it->y / scale;
    char t[60];
    const char * name = it->typeName();
	char * c = (char*)name;
	if ( strstr( name, "plugins/" ) )
	{
		c += 8;
	}
	sprintf(t,"%c%c%c(%i %i)",c[0],c[1],c[2],it->type,it->id);
	//sprintf(t,"%c%c%c%c%c(%i)",c[0],c[1],c[2],c[3],c[4],it->type);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
    glColor3f (tColor[0], tColor[1], tColor[2]); 
    RGL::drawText( x-8,y+16, t );
	glPopAttrib();

    return 1;
}

void render()            
{
	PROFILE;
	renderBg();

    for ( int i=0; i<numItems(); i++ )
    {
		Item * it = getItem(i);
		for ( int j=0; j<it->ncon; j++ )
		{
			if ( ! it->con[j] )
				continue;
		    renderLines( it->con[j], it, 0 );   
		}

	}    
    for ( int i=0; i<numItems(); i++ )
    {
		renderNames(getItem(i), 0, 0 );   
	}

	startTex();
    for ( int i=0; i<numItems(); i++ )
    {
        renderIt( getItem(i), 0, 0 );
    }
    endTex();
}


void idle(HDC hDC)
{
	PROFILE;

	update();

    glClearColor (0.4f, 0.4f, 0.4f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT);
    //~ RGL::perspective( 80, 1.0, 400 ) ;
    RGL::ortho(200,1,400);
    glTranslatef( -wWidth/2, -wHeight/2, -1.0f );
    glTranslatef( 0, 0, -1.0f );
    glPushMatrix ();

	render();

    glPopMatrix ();
    SwapBuffers (hDC);
    Sleep (1);
}

int start(int argc, char **argv)
{
	RGL::setFont( tSize, tName );
	glLineWidth( lWidth );
	startTex();
    RGL::texParams( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPLACE );
    RGL::texChecker( 16, 4 );

	texNames[25] = bgTexture;
	for ( int i=0; i<32; i++ )
	{
		if ( texNames[i] )
			texId[i] = loadImage(texNames[i]);
	}
    //t1 = loadImage(texName1);
    // texId[25] = loadImage(bgTexture);
    endTex();

    loaded = 1; 


	loaded = asio.loadDriver( asioDriverName, doAudioAsio );
	if ( loaded )
	{
		asio.startDriver();
	}

	numSounds = 2;
	//dsound.InitDirectSound(GetDesktopWindow());
	//dsound.caps();
	//for ( int i=0; i<numSounds; i++ )
	//{
	//	dsound.AddVoice(i,SizeAudioBuffer * 2 * 2);
	//	dsound.Play(i,1);
	//}
	frame = 0;
    return loaded;
}



#ifdef KONSOLE
void synexit()
{
	clearItems();
	printf( "alive : %d /%d.\n", _nIt(), numItems() );
//	asio.unloadDriver();	
}
int main(int argc, char**argv)
{
    HINSTANCE hInstance = GetModuleHandle(0);
	atexit( synexit );

#else
int WINAPI 
WinMain (HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPSTR lpCmdLine,
         int iCmdShow)
{
#endif

	{

		PROFILE;

		loadSynSettings( getRoot() );

		WNDCLASS wc;
		HWND hWnd;
		HDC hDC;
		HGLRC hRC;        
		MSG msg;
		BOOL bQuit = FALSE;

		/* register window class */
		wc.style = CS_OWNDC;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = "Syn";
		RegisterClass (&wc);

		hWnd = CreateWindow (
		  "Syn", "Syn", 
		  WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE,
		  0, 0, wWidth, wHeight + 30, // offset for titlebar
		  NULL, NULL, hInstance, NULL);

		EnableOpenGL (hWnd, &hDC, &hRC);

		start(0,0);

		while (!bQuit)
		{
			if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					bQuit = TRUE;
				}
				else
				{
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				}
			}
			else
			{
				idle( hDC );
			}
		}


		DisableOpenGL (hWnd, hDC, hRC);
		DestroyWindow (hWnd);

	}


	Profile::dump();

	return 	0; //startupAsio(argc,argv);
}




LRESULT CALLBACK 
WndProc (HWND hWnd, unsigned int message,
         WPARAM wParam, LPARAM lParam)
{
	PROFILE;

    switch (message)
    {
    case WM_CREATE:
		{
		//HFONT hFont = (HFONT) GetStockObject( ANSI_VAR_FONT ); 
		//SendMessage( hWnd, WM_SETFONT, (WPARAM) hFont, 1 );
		}
        return 0;
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;

    case WM_DESTROY:
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;
        }
        return 0;

    default:
        return DefWindowProc (hWnd, message, wParam, lParam);
    }
}



void EnableOpenGL (HWND hWnd, HDC *hDC, HGLRC *hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC (hWnd);

    /* set the pixel format for the DC */
    ZeroMemory (&pfd, sizeof (pfd));
    pfd.nSize = sizeof (pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | 
      PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFormat = ChoosePixelFormat (*hDC, &pfd);
    SetPixelFormat (*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext( *hDC );
    wglMakeCurrent( *hDC, *hRC );

}



void DisableOpenGL (HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent (NULL, NULL);
    wglDeleteContext (hRC);
    ReleaseDC (hWnd, hDC);
}

