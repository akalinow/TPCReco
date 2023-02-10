#ifndef TPCSOFT_DATABUFFER_H
#define TPCSOFT_DATABUFFER_H

#include "SimEvent.h"


//so far it is (almost)identical to ModuleExchangeSpace, we keep it separate to have flexibility
struct DataBuffer {
    SimEvent *simEv;
};

#endif //TPCSOFT_DATABUFFER_H
