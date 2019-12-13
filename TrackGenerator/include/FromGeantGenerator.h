#ifndef _FromGeantGenerator_H_
#define _FromGeantGenerator_H_
#include "AbstractGenerator.h"
#include <iostream>
//Builder of EventsTPC from MonteCarloSimulations/GELI output
class FromGeantGenerator: public AbstractGenerator {
public:

    //Default constructor (not RAII)
    FromGeantGenerator();

    //Fills histogram (remember to setup it first!) with information from input
    void generateTrack() final;

    //Generates events in loop over entries in input tree
    void generateEvents(int counts=0) final;

    //Loads .root data file -> output from MonteCarloSimulations/GELI
    void loadDataFile(std::string dataFileAddress);
    
    //Loads entry from input tree and sets EventTPC event number
    void setEntry(int i=0) final;
    
protected:

    //Sets Branchedressess for input tree
    void setBranches();

    //input:

    std::vector<double>* x=nullptr;
    std::vector<double>* y=nullptr;
    std::vector<double>* z=nullptr;
    std::vector<double>* Edep=nullptr;
    TTree* depTree;
    TTree* primTree;
    TFile* dataFile;
    
};
#endif // _FromGeantGenerator_H_