#include "LineGenerator.h"
#include <iostream>
//Default constructor. Creates default space for tracks
LineGenerator::LineGenerator(): projectionsCollection(3) {
  const unsigned int NbinsX = 50;
  const unsigned int NbinsY = 50;
  const unsigned int NbinsZ = 150;
  const double xmin = -100.0;  // mm
  const double xmax =  100.0;  // mm
  const double ymin = -100.0; // mm
  const double ymax =  100.0; // mm  
  const double zmin = -100; // mm
  const double zmax =  100; // mm

  myTrack3D =TH3D("h_xz", "Pseudo data;X [mm];Z [mm];Charge/bin [arb.u.]",
		    NbinsX, xmin, xmax,
            NbinsY, ymin, ymax,
		    NbinsZ, zmin, zmax);
}




void LineGenerator::loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr){
  myGeometryPtr=geometryPtr;
  myProjectorPtr.reset(new UVWprojector(myGeometryPtr.get()));
  myEvent.Clear();
  myEvent.SetGeoPtr(myGeometryPtr);
}

void LineGenerator::loadGeometry(const std::string & fileName){
  loadGeometry(std::make_shared<GeometryTPC>(fileName.c_str()));
}

// Sets cooridnates of track's origin point  in mm,mm,mm. 
void LineGenerator::setTrackOrigin(double x, double y, double z){
    trackOrigin=TVector3(x,y,z);
}

// Resets old track and generate new track in form of line of set length, origin point and width (sigma).
// Sigma is same for all directions
void LineGenerator::generateTrack(int option){
    myTrack3D.Reset();
    if(!option){
        for(int i=0;i!=trackCounts;++i){\
            //add Z-dependend sigma?
            TVector3 point(trackOrigin+rng.Uniform()*trackVector);
            point+=TVector3(rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma));
            myTrack3D.Fill(point.X(),point.Y(),point.Z());
        }
    }
}


//Generates three 2D projections (UWV) of 3D track
void LineGenerator::project(){
    myProjectorPtr->SetEvent3D(myTrack3D);
    for (int i=0;i!=3;++i){
       projectionsCollection.at(i)=myProjectorPtr->GetStripVsTime_TH2D(i);
    }
}

//Clears and creates EventTPC basing on generated projections.
//Skips under- and overflowing bins
void LineGenerator::fillEvent(){
    myEvent.Clear();
    myEvent.SetGeoPtr(myGeometryPtr);
    for (int dir=0;dir!=3;++dir){
        TH2D* projection=projectionsCollection.at(dir);
        for(int i=1; i!=projection->GetNbinsX();++i){
            for(int j=1;j!=projection->GetNbinsY();++j){
                myEvent.AddValByStrip(dir,j,i-1,projection->GetBinContent(i,j));
            }
        }
    }
}

//Return EventTPC generated from line of set origin, length and width
EventTPC& LineGenerator::generateLineEvent(){
    generateTrack();
    project();
    fillEvent();
    return myEvent;
}