#include "LineGenerator.h"
#include <iostream>
//Default constructor. Creates default space for tracks
LineGenerator::LineGenerator(): AbstractGenerator(),lines(0) {
}

// Resets old track and generate new track in form of line of set length, origin point and width (sigma).
// Sigma is same for all directions
void LineGenerator::generateTrack(){
    myTrack3D.Reset();
    clearParameters();
    for (auto &line :lines){
        line.update();
        for(int i=0;i!=trackCounts;++i){
            //add Z-dependend sigma?
            TVector3 point=line.GetSample()+TVector3(rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma),rng.Gaus(0,trackSigma));
            myTrack3D.Fill(point.X(),point.Y(),point.Z());
        }
        A.push_back(0);
        Z.push_back(0);
        momentum.push_back(TVector3(0,0,0));
        energy.push_back(0);
        start.push_back(line.getStart());
        stop.push_back(line.getStop());
        length.push_back(line.getLength());
    }
    tracksNo=lines.size();
}


void LineGenerator::generateEvents(int count){
    for (int i=1;i<=count;++i){
        std::cout<<"Event "<<i<<" ..."<<std::endl;
        setEntry(i);
        generateEvent();
        if(outputFile->IsOpen()){
            myEvent.SetGeoPtr(0);
            outputTree->Fill();
        }
    }
    myEvent.SetGeoPtr(myGeometryPtr);
}