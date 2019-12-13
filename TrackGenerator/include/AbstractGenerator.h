#ifndef _AbstractGenerator_H_
#define _AbstractGenerator_H_

#include "UVWprojector.h"
#include "EventTPC.h"
#include <TTree.h>
#include <TFile.h>
#include <TVector3.h>
#include <iostream>

//Base class for builder of EventTPCs via UWVProjector
class AbstractGenerator {
public:

    //Default constructor (not RAII)
    AbstractGenerator();

    //Generate single Event basing on loaded  track 3D histogram
    virtual EventTPC& generateEvent();

    //Generating events within loop. Fills output tree if output file is opened
    virtual void generateEvents(int count=0)=0;

    //Creates track 3D histogram
    virtual void generateTrack()=0;

    //Creates projections of track histogram via UWVProjector
    virtual void project();

    //Creates EventTPC basing on track's projections
    //Skips under- and overflowing bins
    virtual void fillEvent();

    //Sets event number
    virtual void setEntry(int i=0);
    
    
    //Set-up:
    
    //Loads GeometryTPC to EventTPC and UVWProjector
    virtual void loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr);
    //Loads GeometryTPC to EventTPC and UVWProjector
    virtual void loadGeometry(const std::string & fileName);
    
    //Creates empty track 3D histogram. Redundant if histogram is loaded from other source
    void setTrackSpace(int NbinsX=50, double xmin=-100,double xmax=100,
                        int NbinsY=50, double ymin=-100,double ymax=100,
                        int NbinsZ=50, double zmin=-100,double zmax=100);

    //Opens output file, creates and sets branches for output tree 
    void setOutput(std::string);

    //Writes output tree on disk
    void writeOutput();

    // Closes output file
    inline void closeOutput(){outputFile->Close();}
    //Getters:

    // Returns UVW projections vector
    inline std::vector<TH2D*> getProjections(){return projectionsCollection;}

    // Returns track 3D histogram
    inline TH3D &getTrack(){return myTrack3D;}

    // Returns EventTPC
    inline EventTPC &getEventTPC(){return myEvent;}

    protected:

    std::vector<TH2D*> projectionsCollection;
    std::shared_ptr<GeometryTPC> myGeometryPtr;
    UVWprojector* myProjectorPtr;
    TH3D myTrack3D;
    EventTPC myEvent;
    int eventNr=0;

    //Output:

    TFile* outputFile;
    TTree* outputTree;
    EventTPC* persistentEvent;
    
    //Simulation parameters:

    unsigned int tracksNo;
    std::vector<unsigned int> A;
    std::vector<unsigned int> Z;
    std::vector<TVector3> momentum;
    std::vector<TVector3> start;
    std::vector<TVector3> stop;
    std::vector<double> energy;
    std::vector<double> length;

    //Clears simulations parameters
    void clearParameters();
};    
#endif // _AbstractGenerator_H_