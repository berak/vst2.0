#include "Blep.h"

#include <stdio.h>
#include <malloc.h>
#include <math.h>



/*
 * BLEP Table
 */


#define SAMPLE_FREQUENCY 44100
#define KTABLE 64 // BLEP table oversampling factor


#define LERP(A,B,F) ((B-A)*F+A)

class Blep
{
	enum  oscwave_t
	{
		OT_SAW,
		OT_SQUARE,
	};

	struct osc_t
	{
		double f; // frequency
		double p; // phase
		double v; // output

		bool bSync;		// hardsync on/off
		double fSync;	// hardsync frequency
		double fPWM;		// pulse width

		oscwave_t type; // square or saw
		
		double *buffer; // circular output buffer
		int cBuffer;	// buffer size
		int iBuffer;	// current buffer position
		int nInit;		// amount of initialized entries

		osc_t() : buffer(0) {}
		~osc_t() { delete [] buffer; }
	};


	struct minbleptable_t
	{
		double *lpTable;
		int c;
		minbleptable_t() : lpTable(0) {}
		~minbleptable_t() { delete [] lpTable; }
	};

	minbleptable_t gMinBLEP;

	osc_t osc;

	bool minBLEP_Init()
	{
		// load table
		FILE *fp=fopen("minblep.mat","rb");
		unsigned int iSize;

		if (!fp) return false;

		fseek(fp,0x134,SEEK_SET);

		fread(&iSize,sizeof(int),1,fp);
		gMinBLEP.c=iSize/sizeof(double);

		gMinBLEP.lpTable= new double[ iSize ];
		if (!gMinBLEP.lpTable) return false;

		fread(gMinBLEP.lpTable,iSize,1,fp);

		fclose(fp);

		return osc_init();
	}

	bool osc_init()
	{
		// frequency
		//osc.f=440;
		osc.p=0;
		osc.v=0;

		// stuff
		osc.bSync=false;
		osc.fPWM=0.5;
		osc.type=OT_SAW;

		// buffer
		osc.cBuffer=gMinBLEP.c/KTABLE;
		osc.buffer= new double[ osc.cBuffer ];
		osc.iBuffer=0;
		osc.nInit=0;

		return true;
	}


	// add impulse into buffer
	void osc_AddBLEP(double offset, double amp)
	{
		int i;
		double *lpOut=osc.buffer+osc.iBuffer;
		double *lpIn=gMinBLEP.lpTable+(int)(KTABLE*offset);
		double frac=fmod(KTABLE*offset,1.0);
		int cBLEP=(gMinBLEP.c/KTABLE)-1;
		double *lpBufferEnd=osc.buffer+osc.cBuffer;
		double f;

		// add
		for (i=0; i<osc.nInit; i++, lpIn+=KTABLE, lpOut++)
		{
			if (lpOut>=lpBufferEnd) lpOut=osc.buffer;
			f=LERP(lpIn[0],lpIn[1],frac);
			*lpOut+=amp*(1-f);
		}

		// copy
		for (; i<cBLEP; i++, lpIn+=KTABLE, lpOut++)
		{
			if (lpOut>=lpBufferEnd) lpOut=osc.buffer;
			f=LERP(lpIn[0],lpIn[1],frac);
			*lpOut=amp*(1-f);
		}

		osc.nInit=cBLEP;
	}

public:
	void setPw( double p )
	{
		osc.fPWM = p;
	}
	void setWaveform( int w )
	{
		osc.type = oscwave_t(w);
	}

	// play waveform
	double osc_Play(float freq, float sampleFreq)
	{
		double v;
		double fs = freq / sampleFreq;

		// create waveform
		osc.p=osc.p+fs;

		// add BLEP at end of waveform
		if (osc.p>=1)
		{
			osc.p=osc.p-1.0;
			osc.v=0.0f;
			osc_AddBLEP(osc.p/fs,1.0f);
		}

		// add BLEP in middle of wavefor for squarewave
		if (!osc.v && osc.p>osc.fPWM && osc.type==OT_SQUARE)
		{
			osc.v=1.0f;
			osc_AddBLEP((osc.p-osc.fPWM)/fs,-1.0f);
		}

		// sample value
		if (osc.type==OT_SAW)
			v=osc.p;
		else
			v=osc.v;

		// add BLEP buffer contents
		if (osc.nInit)
		{
			v+=osc.buffer[osc.iBuffer];
			osc.nInit--;
			if (++osc.iBuffer>=osc.cBuffer) osc.iBuffer=0;
		}

		return v;
	}

}; // Blep