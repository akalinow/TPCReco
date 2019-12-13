#ifndef _FromTransportGenerator_H_
#define _FromTransportGenerator_H_
#include "AbstractGenerator.h"
#include "SimEvent.hh"
/*Builder of EventsTPC from MonteCarloSimulations/Transport output
* Usage example:
* FromTransportGenerator g;
* g.loadGeometry(path/to/geometry);
* g.loadDataFile(path/to/input);
* g.setOutput(path/to/output); 
* g.generateEvents();
* g.writeOutput();
*/
class FromTransportGenerator: public AbstractGenerator {
public:

    //Default constructor (not RAII)
    FromTransportGenerator();

    //Copies track histogram from input tree and sets simulation parameters (A,Z,length)
    void generateTrack() final;

    //Generate events in loop over input tree. Fills output tree if output is opened
    void generateEvents(int counts=0) final;

    //Loads .root data file -> output from MonteCarloSimulations/Transport
    void loadDataFile(std::string dataFileAddress);

    //Loads entry from input tree and sets EventTPC event number
    void setEntry(int i=0) final;

protected:

    //Sets Branchedressess for input tree
    void setBranches();

    //input:

    TTree* dataTree;
    TFile* dataFile;
    SimEvent* simEvent=nullptr;

};
#endif // _FromTransportGenerator_H_