#ifndef __gl_onboard__
#define __gl_onboard__

namespace RGL	
{
	char * checkError () ;
	char * driverInfo() ;

	void leave2D() ;
	void enter2D() ;
	void ortho( float siz, float znear, float zfar );
	void perspective(float fov, float np, float fp, float *res=0) ;


    unsigned int makeRasterFont();
	unsigned int setFont(unsigned int size, const char * fName);

	// 2d :
	void drawText( int x,int y,char *text );
    void drawTextFont( char * txt, int x, int y, int texFontID );

	void drawTexQuad2D( float vx1, float vy1, float vx2, float vy2,
                        float tx1, float ty1, float tx2, float ty2  );

	// 3d :
	void drawAABB( float m[3], float M[3] );
	void drawPlane( float w, float h, int DIM );
	void drawSphere( float radius, int numMajor, int numMinor);
	void drawCylinder( float radius, int numMajor, int numMinor, float height);
	void drawTorus(float majorRadius, float minorRadius, int numMajor, int numMinor);
	void drawAxes( float px, float py, float pz, float sx, float sy, float sz ) ;
    void drawGrid( float r,float g,float b );
    void drawTri();

    // texturing:
    unsigned int texCreate( unsigned int width, unsigned int height, unsigned int bpp, void *data, bool mipmap, unsigned int id=0 );
	bool  texIsValid( unsigned int id );
    void  texBind( unsigned int id ) ;
    void  texRelease( unsigned int id ) ;
    void  texParams( int minf, int magf, int wrap, int env=0 ) ;
    void  texGenOn( int mode, float *s, float *t ) ;
    void  texGenOff() ;
    bool  texIsResident(unsigned int id);
    long  texMaxSize();
    long  texProxyMem();
    int   texChecker( int size, int tile );

    int screenShot( char *fName, int w, int h );
};


#endif // __gl_onboard__

