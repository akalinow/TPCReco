#ifndef _FromGeantGenerator_H_
#define _FromGeantGenerator_H_
#include "AbstractGenerator.h"
#include "TTree.h"
#include "TFile.h"
#include <iostream>
class FromGeantGenerator: public AbstractGenerator {
public:
friend TTree;
    FromGeantGenerator();
    EventTPC& generateEvent() final;
    void generateTrack();
    void loadDataFile(std::string dataFileAddress);
    void setEntry(int i=0){depTree->GetEntry(i);eventNr=i;}
protected:
    void setBranches();
    std::vector<double>* x=0;
    std::vector<double>* y=0;
    std::vector<double>* z=0;
    std::vector<double>* Edep=0;
    TTree* depTree;
    TTree* primTree;
    TFile* dataFile;
    
};
#endif // _FromGeantGenerator_H_