#include "AbstractGenerator.h"
#include <iostream>
//Default constructor. Creates default space for tracks
AbstractGenerator::AbstractGenerator(): projectionsCollection(3) {
persistentEvent=&myEvent;
}

void AbstractGenerator::setTrackSpace(int NbinsX, double xmin, double xmax,
                        int NbinsY, double ymin, double ymax,
                        int NbinsZ, double zmin, double zmax){
    myTrack3D= TH3D("h_xz", "Pseudo data;X [mm];Z [mm];Charge/bin [arb.u.]",
		    NbinsX, xmin, xmax,
            NbinsY, ymin, ymax,
		    NbinsZ, zmin, zmax);
}


void AbstractGenerator::loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr){
  myGeometryPtr=geometryPtr;
  myProjectorPtr.reset(new UVWprojector(myGeometryPtr.get()));
  myEvent.Clear();
  myEvent.SetGeoPtr(myGeometryPtr);
}

void AbstractGenerator::loadGeometry(const std::string & fileName){
  loadGeometry(std::make_shared<GeometryTPC>(fileName.c_str()));
}

EventTPC& AbstractGenerator::generateEvent(){
    generateTrack();
    project();
    fillEvent();
    return myEvent;
}


//Generates three 2D projections (UWV) of 3D track
void AbstractGenerator::project(){
    myProjectorPtr->SetEvent3D(myTrack3D);
    for (int i=0;i!=3;++i){
       projectionsCollection.at(i)=myProjectorPtr->GetStripVsTime_TH2D(i);
    }
}

//Clears and creates EventTPC basing on generated projections.
//Skips under- and overflowing bins
void AbstractGenerator::fillEvent(){
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
    myEvent.SetEventId(eventNr);
}

void AbstractGenerator::setOutput(std::string outputName){
    outputFile=new TFile(outputName.c_str(), "RECREATE");
    outputTree=new TTree("TPCData","");
    outputTree->Branch("Event", &persistentEvent);
}
void AbstractGenerator::writeOutput(){
 
    outputTree->Write();
    //outputFile->Close();
}

void AbstractGenerator::generateEvents(){
    for (int i=0;i<5;++i){
        setEntry(i);
        generateEvent();
        if(outputFile->IsOpen()){
            myEvent.SetGeoPtr(0);
            outputTree->Fill();
        }
    }
    myEvent.SetGeoPtr(myGeometryPtr);
}

void AbstractGenerator::setEntry(int i){
    eventNr=i;
}