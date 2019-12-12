#ifndef _FromGeantGenerator_H_
#define _FromGeantGenerator_H_
#include "AbstractGenerator.h"
#include <iostream>
class FromGeantGenerator: public AbstractGenerator {
public:
    FromGeantGenerator();
    void generateTrack() final;
    void generateEvents(int counts=0) final;
    void loadDataFile(std::string dataFileAddress);
    void setEntry(int i=0) final;
protected:
    void setBranches();
    std::vector<double>* x=nullptr;
    std::vector<double>* y=nullptr;
    std::vector<double>* z=nullptr;
    std::vector<double>* Edep=nullptr;
    TTree* depTree;
    TTree* primTree;
    TFile* dataFile;
    
};
#endif // _FromGeantGenerator_H_