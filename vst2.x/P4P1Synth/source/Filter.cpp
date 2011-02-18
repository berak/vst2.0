#include "Filter.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>



const float Pi = 3.141592653f;

namespace FilterBessel
{
	struct Filter
	{
		float State0,State1,State2,State3;
		float A0,A1,A2,A3,A4;
		float B0,B1,B2,B3,B4;

		void calc( float q, float Frequency, float Samplerate  )
		{
			float K,K2,K4;
			K  = tan(Pi * Frequency / Samplerate);
			K2 = K*K; // speed improvement
			K4 = K2*K2; // speed improvement


			A0 =  ((((105*K + 105)*K + 45)*K + 10)*K + 1);
			A1 = -( ((420*K + 210)*K2        - 20)*K - 4)*q;
			A2 = -(  (630*K2         - 90)*K2        + 6)*q;
			A3 = -( ((420*K - 210)*K2        + 20)*K - 4)*q;
			A4 = -((((105*K - 105)*K + 45)*K - 10)*K + 1)*q;

			A1 = A1/A0;
			A2 = A2/A0;
			A3 = A3/A0;
			A4 = A4/A0;

			B0 = 105*K4;
			B1 = 420*K4;
			B2 = 630*K4;
			B3 = 420*K4;
			B4 = 105*K4;
		}
		float value( float Input )
		{
			float Output = B0*Input       + State0;
			State0 = B1*Input + A1*Output + State1;
			State1 = B2*Input + A2*Output + State2;
			State2 = B3*Input + A3*Output + State3;
			State3 = B4*Input + A4*Output;
			return Output;
		}

	} filtor;

	void set( float q, float freq, float sampleFreq  )
	{
		filtor.calc( q, freq, sampleFreq );
	}

	float process( float sound )
	{
		return filtor.value(sound);
	}
}

namespace FilterFormant
{

	// Public source code by alex@smartelectronix.com
	// Simple example of implementation of formant filter
	// Vowelnum can be 0,1,2,3,4 <=> A,E,I,O,U
	// Good for spectral rich input like saw or square

	//-------------------------------------------------------------VOWEL COEFFICIENTS
	const double coeff[5][11] = 
	{
		{ 
			8.11044e-06,
			8.943665402,    -36.83889529,    92.01697887,    -154.337906,    181.6233289,
			-151.8651235,   89.09614114,    -35.10298511,    8.388101016,    -0.923313471  ///A
		},
		{
			4.36215e-06,
			8.90438318,    -36.55179099,    91.05750846,    -152.422234,    179.1170248,  ///E
			-149.6496211,87.78352223,    -34.60687431,    8.282228154,    -0.914150747
		},
		{ 
			3.33819e-06,
			8.893102966,    -36.49532826,    90.96543286,    -152.4545478,    179.4835618,
			-150.315433,    88.43409371,    -34.98612086,    8.407803364,    -0.932568035  ///I
		},
		{
			1.13572e-06,
			8.994734087,    -37.2084849,    93.22900521,    -156.6929844,    184.596544,   ///O
			-154.3755513,    90.49663749,    -35.58964535,    8.478996281,    -0.929252233
		},
		{
			4.09431e-07,
			8.997322763,    -37.20218544,    93.11385476,    -156.2530937,    183.7080141,  ///U
			-153.2631681,    89.59539726,    -35.12454591,    8.338655623,    -0.910251753
		}
	};

	struct TheFilter
	{
		//---------------------------------------------------------------------------------
		double memory[10];
		int vowelnum;
		TheFilter()
			: vowelnum(0)
		{
			memset(memory,0,10*sizeof(double));
		}
		//---------------------------------------------------------------------------------

		void voc(int v)
		{
			if ( v>=0 && v<5 )
				vowelnum = v;
		}
		float value(float in)
		{
			return formant_filter(in, vowelnum);
		}
		float formant_filter(float in, int vowelnum)
		{
			double res	=  ( 
					 coeff[vowelnum][0]  * in +
					 coeff[vowelnum][1]  * memory[0] +  
					 coeff[vowelnum][2]  * memory[1] +
					 coeff[vowelnum][3]  * memory[2] +
					 coeff[vowelnum][4]  * memory[3] +
					 coeff[vowelnum][5]  * memory[4] +
					 coeff[vowelnum][6]  * memory[5] +
					 coeff[vowelnum][7]  * memory[6] +
					 coeff[vowelnum][8]  * memory[7] +
					 coeff[vowelnum][9]  * memory[8] +
					 coeff[vowelnum][10] * memory[9] );

			memory[9]= memory[8];
			memory[8]= memory[7];
			memory[7]= memory[6];
			memory[6]= memory[5];
			memory[5]= memory[4];
			memory[4]= memory[3];
			memory[3]= memory[2];
			memory[2]= memory[1];                    
			memory[1]= memory[0];
			memory[0]=(double) res;
			return (float)res;
		}
	} filtor;

	void set( int vnum )
	{
		filtor.voc(vnum);
	}

	float process( float sound )
	{
		return filtor.value(sound);
	}
}


namespace Filter4Pole
{
	struct TheFilter
	{
		double coef[9];
		double d[4];
		double omega; //peak freq
		double g;     //peak mag

		TheFilter()
			: omega(0.5)
			, g(0.5)
		{
			memset(coef,0,9*sizeof(double));
			memset(d,0,4*sizeof(double));
			calc(omega,g);
		}

		void calc( double f, double r, bool LP=true )
		{
			// calculating coefficients:
			if ( f < 0.001 ) f = 0.001;
			omega = f;
			g = (1.0 - r);
			double k,p,q,a;
			double a0,a1,a2,a3,a4;

			k=(4.0*g-3.0)/(g+1.0);
			p=1.0-0.25*k;p*=p;

			if ( LP )
			{
				a=1.0/(tan(0.5*omega)*(1.0+p));
				p=1.0+a;
				q=1.0-a;
				        
				a0=1.0/(k+p*p*p*p);
				a1=4.0*(k+p*p*p*q);
				a2=6.0*(k+p*p*q*q);
				a3=4.0*(k+p*q*q*q);
				a4=    (k+q*q*q*q);
				p=a0*(k+1.0);
		    
				coef[0]=p;
				coef[1]=4.0*p;
				coef[2]=6.0*p;
				coef[3]=4.0*p;
				coef[4]=p;
				coef[5]=-a1*a0;
				coef[6]=-a2*a0;
				coef[7]=-a3*a0;
				coef[8]=-a4*a0;
			}
			else
			{   // HP:
				a=tan(0.5*omega)/(1.0+p);
				p=a+1.0;
				q=a-1.0;
				        
				a0=1.0/(p*p*p*p+k);
				a1=4.0*(p*p*p*q-k);
				a2=6.0*(p*p*q*q+k);
				a3=4.0*(p*q*q*q-k);
				a4=    (q*q*q*q+k);
				p=a0*(k+1.0);
				        
				coef[0]=p;
				coef[1]=-4.0*p;
				coef[2]=6.0*p;
				coef[3]=-4.0*p;
				coef[4]=p;
				coef[5]=-a1*a0;
				coef[6]=-a2*a0;
				coef[7]=-a3*a0;
				coef[8]=-a4*a0;
			}
		}

		double value( double sound )
		{
			// per sample:
			double out=0.3 * (coef[0]*sound+d[0]);
			if ( out > 1.0 ) out = 1.0;
			if ( out < -1.0 ) out = -1.0;
			d[0]=coef[1]*sound+coef[5]*out+d[1];
			d[1]=coef[2]*sound+coef[6]*out+d[2];
			d[2]=coef[3]*sound+coef[7]*out+d[3];
			d[3]=coef[4]*sound+coef[8]*out;
			return out;
		}

	} filtor;

	void set( float q, float freq, float sampleFreq  )
	{
		filtor.calc( freq, q, true );
	}

	float process( float sound )
	{
		return filtor.value(sound);
	}

}


namespace FilterBP
{
	// wot, - no pi ?

	//
	// 2nd order latfir bandpass, orig. from 'sox':
	// y(t) = A * x(t) - B * y(t-1) - C * y(t-2);
	//
	class Filter {
		enum  { 
			kCMax  = 8000,     // max. centerfreq.  
			kSRate = 44100     // est. samplefreq.
		}; 
	protected:
		float A, B, C, v0, v1, v2, om;
	public:
		Filter() { om=2*Pi*kCMax/kSRate; stop(); }
		void stop() {       
			v0=v1=v2=0.0f;
			calc(0.0f,0.0f);
		}
		void calc( float c, float w ) {
			float center = om*c*c;
			float width  = om*(1-w*w); 
			C = expf(-width);
			 float D = 1+C; // helper
			B = -4*C/D*cosf(center);
			A = sqrtf((D*D-B*B)*(1-C)/D); // 'noizy' version
		}
		inline float value( float in ) {
    		v0 = A * in - B * v1 - C * v2; 
			v2 = v1; v1 = v0;
			return v0;
		}
	} bpFilter;

	void set( float q, float freq, float sampleFreq )
	{
		if ( freq < 0.03f ) freq = 0.03f;
		if ( q < 0.0001f ) q = 0.0001f;
		if ( q > 1.0f ) q = 1.0f;
		bpFilter.calc( freq, q );
	}

	float process( float sound )
	{
		return bpFilter.value(sound);
	}
}




namespace FilterLP
{
	const int FILTER_SECTIONS = 2;   /* 2 filter sections for 24 db/oct filter */

	//
	// hmm, the original calloc thing seemd to break vc2009..
	//		iir.coef = (float *) calloc(4 * iir.length + 1, sizeof(float));
	// so history & coef got fixed sized arrays.
	//

	struct FILTER {
		unsigned int length;       /* size of filter */
		float history[ FILTER_SECTIONS * 2 ];            /* pointer to history in filter */
		float coef[ FILTER_SECTIONS * 4 + 1 ];               /* pointer to coefficients of filter */
	};


	struct BIQUAD {
		double a0, a1, a2;       /* numerator coefficients */
		double b0, b1, b2;       /* denominator coefficients */
	} ;


	/*
	 * --------------------------------------------------------------------
	 * 
	 * main()
	 * 
	 * Example main function to show how to update filter coefficients.
	 * We create a 4th order filter (24 db/oct roloff), consisting
	 * of two second order sections.
	 * --------------------------------------------------------------------
	 */

	class TheFiltor
	{
		BIQUAD   ProtoCoef[FILTER_SECTIONS];      /* Filter prototype coefficients, 1 for each section */
		FILTER   iir;
		float    *coef;
	public:
		double   fs, fc;     /* Sampling frequency, cutoff frequency */
		double   Q;			 /* Resonance > 1.0 < 1000 */
		double   k;          /* Set overall filter gain */

		TheFiltor()
		{
			Q = 1;           /* Resonance */
			fc = 5000;       /* Filter cutoff (Hz) */
			fs = 44100;      /* Sampling frequency (Hz) */
			k = 1.0;         /* Set overall filter gain */

			/*
			 * Setup filter s-domain coefficients
			 */
			/* Section 1 */       
			ProtoCoef[0].a0 = 1.0;
			ProtoCoef[0].a1 = 0;
			ProtoCoef[0].a2 = 0;
			ProtoCoef[0].b0 = 1.0;
			ProtoCoef[0].b1 = 0.765367;
			ProtoCoef[0].b2 = 1.0;

			/* Section 2 */       
			ProtoCoef[1].a0 = 1.0;
			ProtoCoef[1].a1 = 0;
			ProtoCoef[1].a2 = 0;
			ProtoCoef[1].b0 = 1.0;
			ProtoCoef[1].b1 = 1.847759;
			ProtoCoef[1].b2 = 1.0;

			iir.length = FILTER_SECTIONS;         /* Number of filter sections */

			calcCoeff();
		}


		float value( float v )
		{
			return iir_filter( v, &(iir) );
		}


		void calcCoeff()
		{
			/*
			 * Compute z-domain coefficients for each biquad section
			 * for new Cutoff Frequency and Resonance
			 */
			unsigned nInd;
			double   a0, a1, a2, b0, b1, b2;

			k = 1.0;
			coef = iir.coef + 1;     /* Skip k, or gain */
			for (nInd = 0; nInd < iir.length; nInd++)
			{
				 a0 = ProtoCoef[nInd].a0;
				 a1 = ProtoCoef[nInd].a1;
				 a2 = ProtoCoef[nInd].a2;

				 b0 = ProtoCoef[nInd].b0;
				 b1 = ProtoCoef[nInd].b1 / Q;      /* Divide by resonance or Q */
				 b2 = ProtoCoef[nInd].b2;
				 szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
				 coef += 4;                       /* Point to next filter section */
			}

			/* Update overall filter gain in coef array */
			iir.coef[0] = k;
		}

	private:

		/*
		 * --------------------------------------------------------------------
		 * 
		 * iir_filter - Perform IIR filtering sample by sample on floats
		 * 
		 * Implements cascaded direct form II second order sections.
		 * Requires FILTER structure for history and coefficients.
		 * The length in the filter structure specifies the number of sections.
		 * The size of the history array is 2*iir->length.
		 * The size of the coefficient array is 4*iir->length + 1 because
		 * the first coefficient is the overall scale factor for the filter.
		 * Returns one output sample for each input sample
		 * 
		 * float iir_filter(float input,FILTER *iir)
		 * 
		 *     float input        new float input sample
		 *     FILTER *iir        pointer to FILTER structure
		 * 
		 * Returns float value giving the current output.
		 * --------------------------------------------------------------------
		 */
		float iir_filter(float input,FILTER *iir)
		{
			unsigned int i;
			float *hist1_ptr,*hist2_ptr,*coef_ptr;
			float output,new_hist,history1,history2;

			coef_ptr = iir->coef;                /* coefficient pointer */

			hist1_ptr = iir->history;            /* first history */
			hist2_ptr = hist1_ptr + 1;           /* next history */

				/* 1st number of coefficients array is overall input scale factor,
				 * or filter gain */
			output = input * (*coef_ptr++);

			for (i = 0 ; i < iir->length; i++)
				{
				history1 = *hist1_ptr;           /* history values */
				history2 = *hist2_ptr;

				output = output - history1 * (*coef_ptr++);
				new_hist = output - history2 * (*coef_ptr++);    /* poles */

				output = new_hist + history1 * (*coef_ptr++);
				output = output + history2 * (*coef_ptr++);      /* zeros */

				*hist2_ptr++ = *hist1_ptr;
				*hist1_ptr++ = new_hist;
				hist1_ptr++;
				hist2_ptr++;
			}

			return(output);
		}
		/*
		 * ----------------------------------------------------------
		 *      bilinear.c
		 *
		 *      Perform bilinear transformation on s-domain coefficients
		 *      of 2nd order biquad section.
		 *      First design an analog filter and use s-domain coefficients
		 *      as input to szxform() to convert them to z-domain.
		 *
		 * Here's the butterworth polinomials for 2nd, 4th and 6th order sections.
		 *      When we construct a 24 db/oct filter, we take to 2nd order
		 *      sections and compute the coefficients separately for each section.
		 *
		 *      n       Polinomials
		 * --------------------------------------------------------------------
		 *      2       s^2 + 1.4142s +1
		 *      4       (s^2 + 0.765367s + 1) (s^2 + 1.847759s + 1)
		 *      6       (s^2 + 0.5176387s + 1) (s^2 + 1.414214 + 1) (s^2 + 1.931852s + 1)
		 *
		 *      Where n is a filter order.
		 *      For n=4, or two second order sections, we have following equasions for each
		 *      2nd order stage:
		 *
		 *      (1 / (s^2 + (1/Q) * 0.765367s + 1)) * (1 / (s^2 + (1/Q) * 1.847759s + 1))
		 *
		 *      Where Q is filter quality factor in the range of
		 *      1 to 1000. The overall filter Q is a product of all
		 *      2nd order stages. For example, the 6th order filter
		 *      (3 stages, or biquads) with individual Q of 2 will
		 *      have filter Q = 2 * 2 * 2 = 8.
		 *
		 *      The nominator part is just 1.
		 *      The denominator coefficients for stage 1 of filter are:
		 *      b2 = 1; b1 = 0.765367; b0 = 1;
		 *      numerator is
		 *      a2 = 0; a1 = 0; a0 = 1;
		 *
		 *      The denominator coefficients for stage 1 of filter are:
		 *      b2 = 1; b1 = 1.847759; b0 = 1;
		 *      numerator is
		 *      a2 = 0; a1 = 0; a0 = 1;
		 *
		 *      These coefficients are used directly by the szxform()
		 *      and bilinear() functions. For all stages the numerator
		 *      is the same and the only thing that is different between
		 *      different stages is 1st order coefficient. The rest of
		 *      coefficients are the same for any stage and equal to 1.
		 *
		 *      Any filter could be constructed using this approach.
		 *
		 *      References:
		 *             Van Valkenburg, "Analog Filter Design"
		 *             Oxford University Press 1982
		 *             ISBN 0-19-510734-9
		 *
		 *             C Language Algorithms for Digital Signal Processing
		 *             Paul Embree, Bruce Kimble
		 *             Prentice Hall, 1991
		 *             ISBN 0-13-133406-9
		 *
		 *             Digital Filter Designer's Handbook
		 *             With C++ Algorithms
		 *             Britton Rorabaugh
		 *             McGraw Hill, 1997
		 *             ISBN 0-07-053806-9
		 * ----------------------------------------------------------
		 */




		/*
		 * ----------------------------------------------------------
		 *      Pre-warp the coefficients of a numerator or denominator.
		 *      Note that a0 is assumed to be 1, so there is no wrapping
		 *      of it.
		 * ----------------------------------------------------------
		 */
		void prewarp(
			double *a0, double *a1, double *a2,
			double fc, double fs)
		{
			double wp, pi;

			pi = 4.0 * atan(1.0);
			wp = 2.0 * fs * tan(pi * fc / fs);

			*a2 = (*a2) / (wp * wp);
			*a1 = (*a1) / wp;
		}


		/*
		 * ----------------------------------------------------------
		 * bilinear()
		 *
		 * Transform the numerator and denominator coefficients
		 * of s-domain biquad section into corresponding
		 * z-domain coefficients.
		 *
		 *      Store the 4 IIR coefficients in array pointed by coef
		 *      in following order:
		 *             beta1, beta2    (denominator)
		 *             alpha1, alpha2  (numerator)
		 *
		 * Arguments:
		 *             a0-a2   - s-domain numerator coefficients
		 *             b0-b2   - s-domain denominator coefficients
		 *             k               - filter gain factor. initially set to 1
		 *                                and modified by each biquad section in such
		 *                                a way, as to make it the coefficient by
		 *                                which to multiply the overall filter gain
		 *                                in order to achieve a desired overall filter gain,
		 *                                specified in initial value of k.
		 *             fs             - sampling rate (Hz)
		 *             coef    - array of z-domain coefficients to be filled in.
		 *
		 * Return:
		 *             On return, set coef z-domain coefficients
		 * ----------------------------------------------------------
		 */
		void bilinear(
			double a0, double a1, double a2,    /* numerator coefficients */
			double b0, double b1, double b2,    /* denominator coefficients */
			double *k,           /* overall gain factor */
			double fs,           /* sampling rate */
			float *coef          /* pointer to 4 iir coefficients */
		)
		{
			double ad, bd;

			/* alpha (Numerator in s-domain) */
			ad = 4. * a2 * fs * fs + 2. * a1 * fs + a0;
			/* beta (Denominator in s-domain) */
			bd = 4. * b2 * fs * fs + 2. * b1* fs + b0;
			/* update gain constant for this section */
			*k *= ad/bd; /* Denominator */
			*coef++ = (2. * b0 - 8. * b2 * fs * fs) / bd;         /* beta1 */
			*coef++ = (4. * b2 * fs * fs - 2. * b1 * fs + b0)  / bd; /* beta2 */
			/* Nominator */
			*coef++ = (2. * a0 - 8. * a2 * fs * fs)  / ad;         /* alpha1 */
			*coef = (4. * a2 * fs * fs - 2. * a1 * fs + a0)  / ad;   /* alpha2 */
		}


		/*
		 * ----------------------------------------------------------
		 * Transform from s to z domain using bilinear transform
		 * with prewarp.
		 *
		 * Arguments:
		 *      For argument description look at bilinear()
		 *
		 *      coef - pointer to array of floating point coefficients,
		 *                     corresponding to output of bilinear transofrm
		 *                     (z domain).
		 *
		 * Note: frequencies are in Hz.
		 * ----------------------------------------------------------
		 */
		void szxform(
			double *a0, double *a1, double *a2, /* numerator coefficients */
			double *b0, double *b1, double *b2, /* denominator coefficients */
			double fc,         /* Filter cutoff frequency */
			double fs,         /* sampling rate */
			double *k,         /* overall gain factor */
			float *coef)       /* pointer to 4 iir coefficients */
		{
			/* Calculate a1 and a2 and overwrite the original values */
			prewarp(a0, a1, a2, fc, fs);
			prewarp(b0, b1, b2, fc, fs);
			bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
		}


	} filtor;


	void set( float q, float freq, float sampleFreq )
	{
		if ( freq < 33.0f ) freq = 33.0f;
		if ( q < 0.0001f ) q = 0.0001f;
		if ( q > 10.0f ) q = 10.0f;
		filtor.Q = q;
		filtor.fc = freq;
		filtor.fs = sampleFreq;
		filtor.calcCoeff();
	}

	float process( float sound )
	{
		return filtor.value(sound);
	}

}










//
// cl /nologo /D TEST_PLOT filter.cpp
//
#ifdef TEST_PLOT
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define myPI 3.1415926535897932384626433832795

#define FP double
#define DWORD unsigned long
#define CUTOFF       5000
#define SAMPLERATE  44100

// take enough samples to test the 20 herz frequency 2 times
#define TESTSAMPLES (SAMPLERATE/20) * 2


int
main(int argc, char **argv)
{
    DWORD  freq;
    DWORD  spos;
    double sIn;
    double sOut;
    double tIn;
    double tOut;
    double dB;
    DWORD  tmp;

	double cut = 123.0;
	double res = 1.0;
	if ( argc > 1 ) cut = atof(argv[1]); 
	if ( argc > 2 ) res = atof(argv[2]); 

    // define the test filter
	Filter4Pole::TheFilter filter;

    printf("   freq    dB     9dB     6dB            3dB                         0dB\n");
    printf("                   |       |              |                           | \n");

    // test frequencies 20 - 20020 with 100 herz steps
    for (freq=20; freq<20020; freq+=100) 
	{
        // (re)initialize the filter
        filter.calc(cut,res);

        // let the filter do it's thing here
        tIn = tOut = 0;
        for (spos=0; spos<TESTSAMPLES; spos++) 
		{
            sIn  = sin((2 * myPI * spos * freq) / SAMPLERATE);
            sOut = filter.value(sIn);
            if ((sOut>1) || (sOut<-1)) 
			{
                // If filter is no good, stop the test
                //printf("Error! Clipping!\n");
                //return(1);
            }
            if (sIn >0) tIn  += sIn;
            if (sIn <0) tIn  -= sIn;
            if (sOut>0) tOut += sOut;
            if (sOut<0) tOut -= sOut;
        }

        // analyse the results
        dB = 20*log(tIn/tOut);
        printf("%5d %5.1f ", freq, dB);
        tmp = (DWORD)(60.0/pow(2.0, (dB/3.0)));
		if ( tmp > 59 ) tmp = 59;
        while (tmp--) 
            putchar('#');
        putchar('\n');
    }
    return 0;
}

#endif // TEST_PLOT