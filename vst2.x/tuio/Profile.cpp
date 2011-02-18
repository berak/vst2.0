#include "Profile.h"
#include <windows.h>
#include <stdio.h>


// Profile
//std::vector<Profile*> Profile::profs;
Profile* profs[512];
int Profile::nprofs=0;

Profile * Profile::getProf(int i)
{
	if ( i>0&&i<nprofs)
		return profs[i];
	return 0;
}

Profile::Profile( const char * n ) 
	: name(n), t0(0), t1(0), ncalls(0), tp(0) 
{
	profs[ nprofs++ ] = (this);
	//profs.push_back(this);
}
Profile::~Profile()
{
//		printf(__FUNCTION__ "\n");
}
void Profile::start() 
{
	t0 = getMicroSeconds();
	ncalls ++;
}
void Profile::stop() 
{
	t1 = getMicroSeconds();
	double diff = t1-t0;
	if ( diff < 0 )
	{
		diff = 1.0;
	}
	tp += (diff) * 0.001;
}
double Profile::getTime() 
{
	if ( ! ncalls ) return 0;
	double r = tp / (double)ncalls;
	ncalls = 0;
	tp  = 0;
	return r;
}
double Profile::getMicroSeconds()
{
	static double timeScale = 0;
	if ( ! timeScale )
	{
		LARGE_INTEGER freq;
 		QueryPerformanceFrequency( &freq );
		timeScale = 1000.0  / freq.QuadPart;
	}
	LARGE_INTEGER now;
	QueryPerformanceCounter( &now );
	return ( now.QuadPart * timeScale );
}


int cmp_time( const void * a, const void * b )
{
	Profile * p0 = *((Profile **)a);
	Profile * p1 = *((Profile **)b);	//{**wow(*I)&(love**)*this);
	double t0 = p0->tp / double(p0->ncalls);
	double t1 = p1->tp / double(p1->ncalls);
	double dt = t0 - t1;
	if ( dt > 0 ) return 1;
	if ( dt < 0 ) return -1;
	return 0; //int(dt * 1000);
}

int cmp_total( const void * a, const void * b )
{
	Profile * p0 = *((Profile **)a);
	Profile * p1 = *((Profile **)b);
	double t0 = p0->tp ;
	double t1 = p1->tp ;
	double dt = t0-t1;
	//if ( dt > 0 ) return 1;
	//if ( dt < 0 ) return -1;
	//return 0;
	return int(dt * 1000);
}
int cmp_calls( const void * a, const void * b )
{
	Profile * p0 = *((Profile **)a);
	Profile * p1 = *((Profile **)b);
	int dc = p0->ncalls - p1->ncalls;
	if ( dc > 0 ) return 1;
	if ( dc < 0 ) return -1;
	return 0;
}

void Profile::dump( int mode )
{
	switch( mode )
	{
	case 1:
		qsort( profs,nprofs,sizeof(Profile*),cmp_time );
		break;
	case 2:
		qsort( profs,nprofs,sizeof(Profile*),cmp_calls );
		break;
	case 3:
		qsort( profs,nprofs,sizeof(Profile*),cmp_total );
		break;
	}

	printf( "------------------------------------------------------------\n" );
	printf( "%-25s %12s %8s %12s\n", "PROFILE", "TOTAL", "CALLS", "TIME" );
	printf( "------------------------------------------------------------\n" );
	for ( int i=0; i<nprofs; i++ ) 
	{
		Profile * p = profs[i];
		printf( "%-25s %12.6f %8i ", p->name, p->tp, p->ncalls );
		printf( "%12.6f\n", p->getTime() );
	}
	printf( "------------------------------------------------------------\n" );
}
