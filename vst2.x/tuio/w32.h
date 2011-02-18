#ifndef __w32_onboard__
#define __w32_onboard__

#include <windows.h>

namespace w32
{

	struct CriticalSection : public CRITICAL_SECTION 
	{
		CriticalSection() 
		{ 
			InitializeCriticalSection(this); 
		}
		
		~CriticalSection() 
		{ 
			DeleteCriticalSection (this);    
		}
		
		operator CRITICAL_SECTION * () 
		{ 
			return (this); 
		}
	};

	class Lock
	{
		CriticalSection * cri;
	public:

		Lock( CriticalSection * cs )
			: cri(cs)
		{
			EnterCriticalSection( cri );
		}

		~Lock()
		{
			LeaveCriticalSection( cri );
		}
	};

//	static CriticalSection critSec0;
} // w32

#endif // __w32_onboard__

