#ifndef __dev_asio_onboard__
#define __dev_asio_onboard__



class DAsio
{
public:
	DAsio();
	~DAsio();
	typedef short ** (*Callback)(int buffSize);

	bool loadDriver( const char * name, DAsio::Callback cb );

	bool startDriver();
	bool stopDriver();

	bool unloadDriver();
};

//extern short ** doAudioAsio(int buffSize);


#endif // __dev_asio_onboard__

