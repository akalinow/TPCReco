#ifndef TPCSOFT_MODULEEXCHANGESPACE_H
#define TPCSOFT_MODULEEXCHANGESPACE_H
//A structure to hold information exchanged between the modules

#include "SimEvent.h"
#include "PEventTPC.h"

struct ModuleExchangeSpace {
    SimEvent simEvt;
    PEventTPC tpcPEvt;
};


#endif //TPCSOFT_MODULEEXCHANGESPACE_H
