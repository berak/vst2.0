#ifndef __tuio_onboard__
#define __tuio_onboard__

namespace Tuio
{
	//
	//!  /tuio/2Dobj set id fi x y a X Y A m r
	//!  /tuio/2Dcur set id    x y   X Y   m
	//
	struct Object
	{
		int id, fi;
		float x, y, a, X, Y, A, m, r;
		int type, state;
	};

	
	struct Listener
	{
		virtual void startBundle() = 0;
		virtual void call( Object & o ) = 0;
		virtual void call( int aliveId ) = 0;
	};


	struct Parser;

	struct MessageQueue
	{
		MessageQueue(Listener & l);
		~MessageQueue();

	private:
		Parser *parser;
	};

};

#endif // __tuio_onboard__

