#ifndef _AbstractGenerator_H_
#define _AbstractGenerator_H_

#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"
#include <memory>
#include <vector>
//Builder of eventTPCs in form of straight lines
class AbstractGenerator {
public:
    AbstractGenerator();
    virtual EventTPC& generateEvent()=0;
    virtual void project();
    virtual void fillEvent();

    //Set-up
    
    virtual void loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr);
    virtual void loadGeometry(const std::string & fileName);
    
    void setTrackSpace(int NbinsX=50, double xmin=-100,double xmax=100,
                        int NbinsY=50, double ymin=-100,double ymax=100,
                        int NbinsZ=50, double zmin=-100,double zmax=100);
    //Getters:

    // Returns UVW projectionsCollections vector
    inline std::vector<TH2D*> getProjections(){return projectionsCollection;}
    // Returns 3D histogrtam of track
    inline TH3D &getTrack(){return myTrack3D;}
    // Returns EventTPC
    inline EventTPC &getEventTPC(){return myEvent;}

    protected:
    std::vector<TH2D*> projectionsCollection;
    std::shared_ptr<GeometryTPC> myGeometryPtr;
    std::unique_ptr<UVWprojector> myProjectorPtr;
    TH3D myTrack3D;
    EventTPC myEvent;
    int eventNr=0;

};    
#endif // _AbstractGenerator_H_