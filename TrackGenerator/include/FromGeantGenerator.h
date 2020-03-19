#ifndef _FromGeantGenerator_H_
#define _FromGeantGenerator_H_
#include "AbstractGenerator.h"
/*Builder of EventsTPC from MonteCarloSimulations/GELI output
* Usage example:
* FromGeantGenerator g;
* g.loadGeometry(path/to/geometry);
* g.loadDataFile(path/to/input);
* g.setOutput(path/to/output); 
* g.generateEvents();
* g.writeOutput();
* g.closeOutput();
*/
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
    
    //Loads entry from input tree and sets EventCharges event number
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