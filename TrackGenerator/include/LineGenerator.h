#ifndef _LineGenerator_H_
#define _LineGenerator_H_

#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"
#include <TRandom3.h>
#include <TVector3.h>
#include <memory>
#include <vector>
//Builder of eventTPCs in form of straight lines
class LineGenerator {
public:
    LineGenerator();
    EventTPC& generateLineEvent();
    void generateTrack(int option=0);
    void project();
    void fillEvent();

    //Set-up
    
    void loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr);
    void loadGeometry(const std::string & fileName);
    void setTrackOrigin(double x=0,double y=0,double z=0); 
    // Sets length of track in mm
    inline void setTrackLength(double length=1){trackVector.SetMag(length);}
    // Sets track's phi (angle in XY/readout plane) in rad.
    inline void setTrackPhi(double phi=0){trackVector.SetPhi(phi);}
    // Sets track's theta (counting from Z) in rad.
    inline void setTrackTheta(double theta=0){trackVector.SetTheta(theta);}
    // Sets gaussian spread of track in mm. The spread is same in all directions.
    inline void setTrackSigma(double sigma){trackSigma=sigma;}
    // Sets number of entries for generating track.
    inline void setTrackCounts(int counts){trackCounts=counts;}
    // Sets track histogram space
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

    private:
    std::vector<TH2D*> projectionsCollection;
    std::shared_ptr<GeometryTPC> myGeometryPtr;
    std::unique_ptr<UVWprojector> myProjectorPtr;
    TH3D myTrack3D;
    EventTPC myEvent;
    TRandom3 rng;

    double trackLength{100}; //mm
    double trackTheta{0.6}; //deg from z/t axis
    double trackPhi{1.1};  //deg in UWV plane
    TVector3 trackOrigin{0,0,0}; 
    TVector3 trackVector{1,0,0};
    double trackSigma{3}; //mm
    double trackCounts{10000};
};    
#endif // _LineGenerator_H_