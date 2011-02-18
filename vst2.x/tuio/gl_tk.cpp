
#include "gl_tk.h"

#ifdef WIN32
 #include <windows.h>
 // 'double' to 'float' :
 #pragma warning (disable:4244)
 // 'size_t' nach 'GLsizei'
 #pragma warning (disable:4267)
#endif


#include <GL/gl.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdio>


#ifndef M_PI
    #define M_PI  3.14159265358979323846f  // Pi, again.
#endif



char * RGL::checkError () 
{   
    static GLenum last_err = 0;
    GLenum err = glGetError();
    if ( err == last_err )
        return 0;
    last_err = err;

    switch ( err ) {
        default:
        case  GL_NO_ERROR :            break;
        case  GL_INVALID_ENUM :        return ( "GL_INVALID_ENUM");     
        case  GL_INVALID_VALUE :       return ( "GL_INVALID_VALUE");    
        case  GL_INVALID_OPERATION :   return ( "GL_INVALID_OPERATION");
        case  GL_STACK_OVERFLOW :      return ( "GL_STACK_OVERFLOW");   
        case  GL_STACK_UNDERFLOW :     return ( "GL_STACK_UNDERFLOW");  
        case  GL_OUT_OF_MEMORY  :      return ( "GL_OUT_OF_MEMORY");    
    }    
    return 0;
}


char * RGL::driverInfo() 
{
	static char str[8012] = {0};
	str[0] = 0;
    sprintf( str, 
        "vendor : %s\nrenderer : %s\nversion : %s\n",
        (char*)glGetString(GL_VENDOR),
        (char*)glGetString(GL_RENDERER),
        (char*)glGetString(GL_VERSION) 
    );   

    char *e = str + strlen(str);

	const char *supportedExt = NULL;
	// Try To Use wglGetExtensionStringARB On Current DC, If Possible
	PROC wglGetExtString = wglGetProcAddress("wglGetExtensionsStringARB");
	if (wglGetExtString)
	{
		supportedExt = ((char*(__stdcall*)(HDC))wglGetExtString)(wglGetCurrentDC());
	    strcat( str, supportedExt );
	}
	supportedExt = (char*)glGetString(GL_EXTENSIONS);
    strcat( str, supportedExt );

    for ( char *c=e; *c; c++ ) 
	{
        if ( *c == ' ' ) 
            *c = '\n';
    };
    return str; 
}






//////////////////////////////////////////////////////////////////////////////
//  drawing:
//////////////////////////////////////////////////////////////////////////////


void RGL::drawGrid( float r,float g,float b )
{
     glColor3f( r,g,b );
 
     int g_siz = 1000, g_step=10;
     for ( int i=-g_siz; i<g_siz; i+=g_step ) 
     {
         glBegin( GL_LINES );
         glVertex3i( -g_siz, 0, i );
         glVertex3i(  g_siz, 0, i );
         glVertex3i( i, 0, -g_siz );
         glVertex3i( i, 0,  g_siz );
         glEnd();
     }
}

void RGL::drawAABB( float m[3], float M[3])
{
    glPushMatrix();
	glBegin(GL_LINES);

	glVertex3d(m[0],m[1],m[2]);
	glVertex3d(M[0],m[1],m[2]);
	glVertex3d(m[0],m[1],M[2]);
	glVertex3d(M[0],m[1],M[2]);
	glVertex3d(m[0],M[1],m[2]);
	glVertex3d(M[0],M[1],m[2]);
	glVertex3d(m[0],M[1],M[2]);
	glVertex3d(M[0],M[1],M[2]);

    glVertex3d(m[0],m[1],m[2]);
	glVertex3d(m[0],M[1],m[2]);
	glVertex3d(m[0],m[1],M[2]);
	glVertex3d(m[0],M[1],M[2]);
	glVertex3d(M[0],m[1],m[2]);
	glVertex3d(M[0],M[1],m[2]);
	glVertex3d(M[0],m[1],M[2]);
	glVertex3d(M[0],M[1],M[2]);

    glVertex3d(m[0],m[1],m[2]);
	glVertex3d(m[0],m[1],M[2]);
	glVertex3d(m[0],M[1],m[2]);
	glVertex3d(m[0],M[1],M[2]);
	glVertex3d(M[0],m[1],m[2]);
	glVertex3d(M[0],m[1],M[2]);
	glVertex3d(M[0],M[1],m[2]);
	glVertex3d(M[0],M[1],M[2]);
	glEnd();
	glPopMatrix();
}

void RGL::drawAxes( float px, float py, float pz, float sx, float sy, float sz ) 
{
    static float zero[3]    = { 0,0,0 };
    static float axes[3][3] = { {1,0,0},  {0,1,0},  {0,0,1} };

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    //glColor3f(1,1,0);
    glPointSize( 6.0f );

    glPushMatrix();
    glTranslatef( px,py,pz );
    glScalef( sx,sy,sz );

    glBegin( GL_POINTS );
      glVertex3fv( zero );
    glEnd();

    for ( int i=0; i<3; i++ ) {
        glColor3f(axes[i][0],axes[i][1],axes[i][2]);
	    glBegin( GL_LINES );
	     glVertex3fv( zero );
	     glVertex3fv( axes[i] );
	    glEnd();
        glBegin( GL_POINTS );
          glVertex3fv( axes[i] );
        glEnd();
    }
    glColor3f(1,1,1);
    glPopMatrix();
    glPopAttrib();
}

void RGL::drawSphere( float radius, int numMajor, int numMinor)
{
  double majorStep = (M_PI / numMajor);
  double minorStep = (2.0 * M_PI / numMinor);
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    double a = i * majorStep;
    double b = a + majorStep;
    double r0 = radius * sin(a);
    double r1 = radius * sin(b);
    GLfloat z0 = radius * cos(a);
    GLfloat z1 = radius * cos(b);

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double c = j * minorStep;
      GLfloat x = cos(c);
      GLfloat y = sin(c);

      glNormal3f((x * r0) / radius, (y * r0) / radius, z0 / radius);
      glTexCoord2f(j / (GLfloat) numMinor, i / (GLfloat) numMajor);
      glVertex3f(x * r0, y * r0, z0);

      glNormal3f((x * r1) / radius, (y * r1) / radius, z1 / radius);
      glTexCoord2f(j / (GLfloat) numMinor, (i + 1) / (GLfloat) numMajor);
      glVertex3f(x * r1, y * r1, z1);
    }
    glEnd();
  }
}

void RGL::drawCylinder( float radius, int numMajor, int numMinor, float height)
{
  double majorStep = height / numMajor;
  double minorStep = 2.0 * M_PI / numMinor;
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    GLfloat z0 = 0.5 * height - i * majorStep;
    GLfloat z1 = z0 - majorStep;

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double a = j * minorStep;
      GLfloat x = radius * cos(a);
      GLfloat y = radius * sin(a);

      glNormal3f(x / radius, y / radius, 0.0);
      glTexCoord2f(j / (GLfloat) numMinor, i / (GLfloat) numMajor);
      glVertex3f(x, y, z0);

      glNormal3f(x / radius, y / radius, 0.0);
      glTexCoord2f(j / (GLfloat) numMinor, (i + 1) / (GLfloat) numMajor);
      glVertex3f(x, y, z1);
    }
    glEnd();
  }
}
//~ void RGL::drawCone( float radius1, float radius2, int numMinor, float height)
//~ {
  //~ double minorStep = 2.0 * M_PI / numMinor;
  //~ int i, j;

    //~ glBegin(GL_TRIANGLE_STRIP);
    //~ for (j = 0; j <= numMinor; ++j) {
      //~ double a = j * minorStep;
      //~ GLfloat x = radius * cos(a);
      //~ GLfloat y = radius * sin(a);

      //~ glNormal3f(x / radius, y / radius, 0.0);
      //~ glTexCoord2f(j / (GLfloat) numMinor, i / (GLfloat) numMajor);
      //~ glVertex3f(x, y, 0);

      //~ glNormal3f(x / radius, y / radius, 0.0);
      //~ glTexCoord2f(j / (GLfloat) numMinor, (i + 1) / (GLfloat) numMajor);
      //~ glVertex3f(x, y, height);
    //~ }
    //~ glEnd();
//~ }

void RGL::drawTorus(float majorRadius, float minorRadius, int numMajor, int numMinor)
{
  double majorStep = 2.0 * M_PI / numMajor;
  double minorStep = 2.0 * M_PI / numMinor;
  int i, j;

  for (i = 0; i < numMajor; ++i) {
    double a0 = i * majorStep;
    double a1 = a0 + majorStep;
    GLfloat x0 = cos(a0);
    GLfloat y0 = sin(a0);
    GLfloat x1 = cos(a1);
    GLfloat y1 = sin(a1);

    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j <= numMinor; ++j) {
      double b = j * minorStep;
      GLfloat c = cos(b);
      GLfloat r = minorRadius * c + majorRadius;
      GLfloat z = minorRadius * sin(b);

      glNormal3f(x0 * c, y0 * c, z / minorRadius);
      glTexCoord2f(i / (GLfloat) numMajor, j / (GLfloat) numMinor);
      glVertex3f(x0 * r, y0 * r, z);

      glNormal3f(x1 * c, y1 * c, z / minorRadius);
      glTexCoord2f((i + 1) / (GLfloat) numMajor, j / (GLfloat) numMinor);
      glVertex3f(x1 * r, y1 * r, z);
    }
    glEnd();
  }
}
//
//  *--v1
//  |   | 
//  |   |
//  v0--*
//
void RGL::drawTexQuad2D( float vx1, float vy1, float vx2, float vy2,
                        float tx1, float ty1, float tx2, float ty2 )
{
    glBegin(GL_QUADS);
    glTexCoord2d( tx1, ty1 );  glVertex2d( vx1, vy1 );
    glTexCoord2d( tx2, ty1 );  glVertex2d( vx2, vy1 );
    glTexCoord2d( tx2, ty2 );  glVertex2d( vx2, vy2 );
    glTexCoord2d( tx1, ty2 );  glVertex2d( vx1, vy2 );
    glEnd();  
}

void _renderPoint( int i, int j, float wn, float hn, int DIM)
{
	float px = wn + (float)i * wn;
	float pz = hn + (float)j * hn;
	float tx = (float)i /DIM; 
	float ty = (float)j /DIM; 

	glNormal3f( 0,1,0 );
	glTexCoord2f( tx,ty );
	glVertex3f( px,0,pz );
}

void RGL::drawPlane( float w, float h, int DIM ) 
{
	float	wn = (float)w / (float)(DIM + 1);  /* Grid element width */
	float	hn = (float)h / (float)(DIM + 1);  /* Grid element height */

	for (int i=0, j=0; j < DIM - 1; j++) 
	{
		glBegin(GL_TRIANGLE_STRIP);

		i = 0;
		_renderPoint( i, j, wn, hn, DIM  );									
		for (i = 0; i < DIM - 1; i++) 
		{
			_renderPoint( i, j+1, wn, hn, DIM  );
			_renderPoint( i+1, j, wn, hn, DIM  );
		}
		_renderPoint( DIM-1, j+1, wn, hn, DIM  );

		glEnd();
	}
}

int gl_font_base = -1;
void RGL::drawText(int x,int y,char *text)
{
    if ( gl_font_base == -1 ) {
    #ifdef _WIN32
        HDC dc = wglGetCurrentDC();
		gl_font_base = glGenLists(256);
		wglUseFontBitmaps(dc, 0, 255, gl_font_base);
    #else 
        base = RGL::makeRasterFont();
    #endif 
    }
    glRasterPos2i(x,y);
	glListBase(gl_font_base);
	glCallLists(strlen(text), GL_BYTE, text);
}


void RGL::drawTextFont( char *txt, int x, int y, int font_texture )
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font_texture);

	static int font_listbase = -1;
    if ( font_listbase == -1 ) 
    {
    	glBindTexture(GL_TEXTURE_2D, font_texture);
        font_listbase = glGenLists(256);
    	for( int i=0; i<256; i++)
	    {
    		float cx=float(i%16)/16.0f;
	       	float cy=float(i/16)/16.0f;
    		glNewList(font_listbase + i, GL_COMPILE);
			glBegin(GL_QUADS);
				glTexCoord2f(cx, 1-cy-0.0625f);
				glVertex2i(0, 0);
				glTexCoord2f(cx+0.0625f, 1-cy-0.0625f);
				glVertex2i(16, 0);
				glTexCoord2f(cx+0.0625f, 1-cy);
				glVertex2i(16, 16);
				glTexCoord2f(cx, 1-cy);
				glVertex2i(0, 16);							
			glEnd();
			glTranslatef(16,0,0);
    		glEndList();
	    }
    }
	glPushMatrix();
	glLoadIdentity();
	glTranslated(x, y, 0);
    glListBase(font_listbase-32+(128*0));
	glCallLists(strlen(txt), GL_BYTE,txt);
    glPopMatrix();	
}



void RGL::drawTri()
{
	glPushMatrix();
		glBegin( GL_TRIANGLES );
			glColor3ub(0xff,0,0);		glVertex3f(-5,0,-20);
			glColor3ub(0,0xff,0);		glVertex3f( 5,0,-20);
			glColor3ub(0,0,0xff);		glVertex3f( 0,5,-20);
		glEnd();
    glPopMatrix();
}

//////////////////////////////////////////////////////////////////////////////
//  projection
//////////////////////////////////////////////////////////////////////////////


void RGL::perspective(float fov, float np, float fp, float *res) 
{  
    float vp[4];
    glGetFloatv( GL_VIEWPORT, vp );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective( fov, float(vp[2])/vp[3], np, fp );
	if ( res )
	{
		glGetFloatv( GL_PROJECTION_MATRIX, res );
	}
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RGL::ortho( float siz, float znear, float zfar )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -siz, siz, -siz, siz, znear, zfar );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}




void RGL::enter2D() 
{
    static GLint view[4];
    glGetIntegerv( GL_VIEWPORT, view );
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D( view[0], view[2], view[1], view[3] );

    glPushAttrib( GL_ALL_ATTRIB_BITS );
}


void RGL::leave2D() 
{
    glPopAttrib();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glColor3f(1,1,1);
//	glDisable(GL_BLEND);
//	glEnable(GL_DEPTH_TEST);
}



//////////////////////////////////////////////////////////////////////////////
//  texturing:
//////////////////////////////////////////////////////////////////////////////




unsigned int RGL::texCreate( unsigned int width, unsigned int height, unsigned int bpp, void *data, bool mipmap, unsigned int id )
{
    if ( ! data ) return 0;

    int type = bpp;
	int gl_dim = GL_TEXTURE_2D;
    switch( bpp ) {
        case 1:
			type   = GL_LUMINANCE; 
			gl_dim = GL_TEXTURE_1D;
			break;
        
        case 3: type = GL_RGB;   break;
        case 4: type = GL_RGBA;  break;
        //~ case 3: type = GL_BGR_EXT;   break;
        //~ case 4: type = GL_RGBA_EXT;  break;

		default:  return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//    glEnable( GL_TEXTURE_2D, 1 );
    if ( id < 1 )
        glGenTextures( 1, &id );
    if ( id < 1 ) {
        printf( "GlEngine could not create texObject !\n" );
        return 0;
    }
    glBindTexture( gl_dim, id );
    

    if ( mipmap && (type!=GL_LUMINANCE) )  // no mip for cellshading!
        gluBuild2DMipmaps( gl_dim, bpp, width, height, type, GL_UNSIGNED_BYTE, data );
    else
        glTexImage2D( gl_dim, 0, bpp, width, height, 0, type, GL_UNSIGNED_BYTE, data );
    return id;
}



void RGL::texRelease( unsigned int id ) 
{
    if ( glIsTexture( id ) ) {
       // glBindTexture( GL_TEXTURE_2D, id );
       // glTexImage2D( GL_TEXTURE_2D, 0, 3, 0, 0, 0, 3, GL_UNSIGNED_BYTE, 0 );
        glDeleteTextures( 1, &id );
    }
}

void RGL::texBind( unsigned int id ) 
{
    if ( glIsTexture( id ) ) {
	    glBindTexture( GL_TEXTURE_2D, id );
    }
}
bool RGL::texIsValid( unsigned int id ) 
{
    return ( glIsTexture( id ) != 0 );
}


void RGL::texParams( int minf, int magf, int wrap, int env ) 
{
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf); 
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf); 
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap );
    if ( env )
        glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
}

void RGL::texGenOn( int mode, float *s, float *t ) 
{
    // Set up texture coordinate generation
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, mode);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, mode);
	float fS[4] = { -0.1f, 0.0f, 0.0f, 0.0f };
	float fT[4] = { 0.0f, 0.0f, -0.1f, 0.0f };
    if ( mode == GL_OBJECT_LINEAR ) {
        glTexGenfv(GL_S, GL_OBJECT_PLANE, ( s ? s : fS ) );
        glTexGenfv(GL_T, GL_OBJECT_PLANE, ( t ? t : fT ) );
    } else if ( mode == GL_EYE_LINEAR ) {
        glTexGenfv(GL_S, GL_EYE_PLANE, ( s ? s : fS ) );
        glTexGenfv(GL_T, GL_EYE_PLANE, ( t ? t : fT ) );
    }
    // Turn on the coordinate generation,
    // DON'T FORGET TO SWITCH THIS OFF AFTER RENDERING !!!
    glEnable(GL_TEXTURE_GEN_S); 
    glEnable(GL_TEXTURE_GEN_T);
}

void  RGL::texGenOff() 
{
    glDisable(GL_TEXTURE_GEN_S); 
    glDisable(GL_TEXTURE_GEN_T);
}



bool RGL::texIsResident(unsigned int id)
{
    unsigned char res=false;
    glAreTexturesResident( 1, &id, &res );
    return (res!=0);
}

long RGL::texMaxSize()
{
    int s=0;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &s );
    return s;
}


long RGL::texProxyMem()
{
	int w  = 1;
	int ok = 1;
	while ( ok ) {
		glTexImage2D( GL_PROXY_TEXTURE_2D, 0, GL_RGBA, w, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
		glGetTexLevelParameteriv( GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &ok );
        if ( checkError() )
            break;
        w  <<= 1;
	}
	return w*w*4;
}


int RGL::texChecker( int size, int tile )
{
    unsigned char *pix = new unsigned char[ size* size* 3];
    unsigned char *d = pix;
    for ( int i = 0; i < size; i++) {
        for ( int j = 0; j < size; j++) {
            unsigned char c = ((((i&tile)==0)^((j&tile)==0)))*255;
            *d++ = 0xff;
            *d++ = c;
            *d++ = c;
        }
    }
    return RGL::texCreate( size,size,3,pix, 1, 1 );
}



//////////////////////////////////////////////////////////////////////////////
//  font
//////////////////////////////////////////////////////////////////////////////
unsigned int RGL::setFont(unsigned int fSize, const char * fName) 
{ 
    #ifdef _WIN32
		LOGFONT lf; 
		ZeroMemory(&lf, sizeof(lf)); 
		lstrcpy(lf.lfFaceName, fName ); 
		lf.lfHeight = fSize; 
		lf.lfWeight = 100;//FW_HEAVY; 
		//lf.lfItalic = TRUE; 
		//lf.lfUnderline = TRUE; 
		HFONT font = CreateFontIndirect(&lf); 
		if ( font )
		{
			HDC dc = wglGetCurrentDC();
			SelectObject( dc, font );
			//SendMessage( GetActiveWindow(), WM_SETFONT, (WPARAM)font, 1 );
		
			gl_font_base = glGenLists(256);
			wglUseFontBitmaps(dc, 0, 255, gl_font_base);
			return true;
		}
    #endif 
	return gl_font_base;
} 

unsigned int RGL::makeRasterFont() 
{
    // private bitmap table (pbm)
    // only big letterz A-Z !
    static unsigned char letters[][13] = {
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0x66, 0x3c, 0x18}, 
    {0x00, 0x00, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe}, 
    {0x00, 0x00, 0x7e, 0xe7, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e}, 
    {0x00, 0x00, 0xfc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xce, 0xfc}, 
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xff}, 
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xff}, 
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xcf, 0xc0, 0xc0, 0xc0, 0xc0, 0xe7, 0x7e}, 
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3}, 
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7e}, 
    {0x00, 0x00, 0x7c, 0xee, 0xc6, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06}, 
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xe0, 0xf0, 0xd8, 0xcc, 0xc6, 0xc3}, 
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0}, 
    {0x00, 0x00, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xdb, 0xff, 0xff, 0xe7, 0xc3}, 
    {0x00, 0x00, 0xc7, 0xc7, 0xcf, 0xcf, 0xdf, 0xdb, 0xfb, 0xf3, 0xf3, 0xe3, 0xe3}, 
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e}, 
    {0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe}, 
    {0x00, 0x00, 0x3f, 0x6e, 0xdf, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66, 0x3c}, 
    {0x00, 0x00, 0xc3, 0xc6, 0xcc, 0xd8, 0xf0, 0xfe, 0xc7, 0xc3, 0xc3, 0xc7, 0xfe}, 
    {0x00, 0x00, 0x7e, 0xe7, 0x03, 0x03, 0x07, 0x7e, 0xe0, 0xc0, 0xc0, 0xe7, 0x7e}, 
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff}, 
    {0x00, 0x00, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3}, 
    {0x00, 0x00, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3}, 
    {0x00, 0x00, 0xc3, 0xe7, 0xff, 0xff, 0xdb, 0xdb, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3}, 
    {0x00, 0x00, 0xc3, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3}, 
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x3c, 0x66, 0x66, 0xc3}, 
    {0x00, 0x00, 0xff, 0xc0, 0xc0, 0x60, 0x30, 0x7e, 0x0c, 0x06, 0x03, 0x03, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // space
    {0x00, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x7c, 0x3c, 0x1c, 0x00}, // 1
	{0x00, 0x00, 0x7e, 0x7e, 0x30, 0x18, 0x0e, 0x06, 0x06, 0x4c, 0x38, 0x30, 0x00}, // 2
	{0x00, 0x00, 0x78, 0x7c, 0x0c, 0x1c, 0x38, 0x1c, 0x4c, 0x3c, 0x38, 0x00, 0x00}  // 3
    };

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLuint fontOffset = glGenLists (128);
    GLuint i, j;
    for (i = 0,j = 'A'; i < 30; i++,j++) {
        glNewList(fontOffset + j, GL_COMPILE);
        glColor3f( 1.0f, 1.0f, 1.0f );
        glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, letters[i]);
        glEndList();
    }
    return fontOffset;
}





int RGL::screenShot (char *fName, int w, int h)
{
    //FIXME!
    w -= (w%4);
    //w = 256;
    //h = 256;
    unsigned int size = w * h * 3;
	unsigned char *fBuffer = new unsigned char [ size ];
	if (fBuffer == NULL)
		return 0;

//    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glReadPixels (0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, fBuffer);

	//unsigned char header[] = "\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	//unsigned char info[6] = { w, (w >> 8), h, (h >> 8), 24, 0 };
 	FILE *ppm = fopen (fName, "wb");
    fprintf( ppm, "P6\n# SCREENDUMP\n%d %d \n255\n", w, h );
	//fwrite (header, sizeof (char), 12, s);
	//fwrite (info,   sizeof (char), 6, s);
    /* 	unsigned char *p = fBuffer;
	for (unsigned int i=0; i<size; i+=3)
	{
    	unsigned char temp = p[i];
		p[i] = p[i + 2];
		p[i + 2] = temp;
	}*/
	// dump the image data to the file
	fwrite (fBuffer, sizeof (char), size, ppm);
	fclose (ppm);

	delete [] fBuffer;
	return size;
}


