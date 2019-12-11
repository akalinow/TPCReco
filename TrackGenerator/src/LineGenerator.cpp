#include "LineGenerator.h"
#include <iostream>
//Default constructor. Creates default space for tracks
LineGenerator::LineGenerator(): AbstractGenerator() {
 //setTrackSpace();
}

// Sets cooridnates of track's origin point  in mm,mm,mm. 
void LineGenerator::setTrackOrigin(double x, double y, double z){
    trackOrigin=TVector3(x,y,z);
}

// Resets old track and generate new track in form of line of set length, origin point and width (sigma).
// Sigma is same for all directions
void LineGenerator::generateTrack(){
    myTrack3D.Reset();
    for(int i=0;i!=trackCounts;++i){\
        //add Z-dependend sigma?
        TVector3 point(trackOrigin+rng.Uniform()*trackVector);
        point+=TVector3(rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma));
        myTrack3D.Fill(point.X(),point.Y(),point.Z());
    }
}
