#include "EasyBMP.h"


int main(int argc, char **argv)
{
	int cr=0,cg=0,cb=0;
	char * iname = "*";
	if ( argc<1 ) return 1;
	if ( argc>1 ) iname = argv[1];
	if ( argc>2 ) cr = atoi(argv[2]);
	if ( argc>3 ) cg = atoi(argv[3]);
	if ( argc>4 ) cb = atoi(argv[4]);

	
	BMP bmp_in, bmp_out;
	if ( ! bmp_in.ReadFromFile(iname) )
	{
		return 1;
	}

	int w = bmp_in.TellWidth();
	int h = bmp_in.TellHeight();
	int d = bmp_in.TellBitDepth();
	bmp_out.SetSize( w,h );
	bmp_out.SetBitDepth( 32 );

	int dr=0,dg=0,db=0;
	for ( int j = 0; j < h; j++) 
	{
		for (int i = 0; i < w; i++) 
		{
		    RGBApixel p = bmp_in.GetPixel(i,j);
			
			dr=abs(p.Red-cr);
			dg=abs(p.Green-cg);
			db=abs(p.Blue-cb);
			int a = (dr+dg+db)/4;
			if ( a>255 ) a=255;	
			if ( a<0   ) a=0;	
				
			p.Alpha = 0xff - a; 
			
			//p.Alpha = ( !dr && !dg && !db ) ? 0xff : 0; 
            bmp_out.SetPixel( i,j, p );
		}
	}

	bmp_out.WriteToFile( "a_out.bmp" );

	return 0;
}
