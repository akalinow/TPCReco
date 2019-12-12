#ifndef _FromTransportGenerator_H_
#define _FromTransportGenerator_H_
#include "AbstractGenerator.h"
#include "SimEvent.hh"
#include <iostream>
class FromTransportGenerator: public AbstractGenerator {
public:
    FromTransportGenerator();
    void generateTrack() final;
    void generateEvents(int counts=0) final;
    void loadDataFile(std::string dataFileAddress);
    void setEntry(int i=0) final;

protected:
    void setBranches();
    SimEvent* simEvent=nullptr;
    TTree* dataTree;
    TFile* dataFile;
};
#endif // _FromTransportGenerator_H_