#ifndef TPCSOFT_MODULEEXCHANGESPACE_H
#define TPCSOFT_MODULEEXCHANGESPACE_H
//A structure to hold information exchanged between the modules

#include "SimEvent.h"
#include "PEventTPC.h"
#include "Track3D.h"

struct ModuleExchangeSpace {
    SimEvent simEvt;
    PEventTPC tpcPEvt;
    Track3D track3D;
    eventraw::EventInfo eventInfo;
};


#endif //TPCSOFT_MODULEEXCHANGESPACE_H
