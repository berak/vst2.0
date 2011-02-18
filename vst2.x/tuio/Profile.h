#ifndef __Profile_onboard__
#define __Profile_onboard__


//#include <vector>


struct Profile
{
	const char * name;
	unsigned int ncalls;
	double t0, t1, tp;

	static int nprofs;
	static Profile * getProf(int i);
	//static std::vector<Profile*> profs;
	
	Profile( const char * n ) ;
	~Profile();

	void start();
	void stop();
	double getTime();
	double getMicroSeconds();

	static void dump(int mode=1); // 0:nosort 1:time 2:calls 3:total

}; // Profile


struct Scope
{
	Profile & p;

	Scope(Profile & p) 
		: p(p) 
	{ 
		p.start(); 
	}

	~Scope() 
	{ 
		p.stop(); 
	}

}; // Scope


#define PROFILE static Profile _13_p(__FUNCTION__); Scope _13_s(_13_p);


#endif // __Profile_onboard__

