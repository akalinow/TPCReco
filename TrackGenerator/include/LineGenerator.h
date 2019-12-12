#ifndef _LineGenerator_H_
#define _LineGenerator_H_
#include "AbstractGenerator.h"
#include <TRandom3.h>
#include <TVector3.h>
#include <memory>
#include <vector>
//Builder of eventTPCs in form of straight lines
class LineGenerator: public AbstractGenerator {
public:
    LineGenerator();
    void generateTrack();
    void generateEvents(int count=0) final;
    //Set-up
    
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
    
    private:
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