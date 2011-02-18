#ifndef __vsthost_onboard__
#define __vsthost_onboard__

#include "syn.h"



// factory
Item * createVstHost( const char * dllName, int itemType, int nInputs, int map_angle=-1 );

// map item connection to vst param
bool setVstHostMapping( Item * it, int param, int con );



#endif // __vsthost_onboard__

