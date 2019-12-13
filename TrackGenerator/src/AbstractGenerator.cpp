#include "AbstractGenerator.h"
#include <iostream>


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


void AbstractGenerator::project(){
    myProjectorPtr->SetEvent3D(myTrack3D);
    for (int i=0;i!=3;++i){
       projectionsCollection.at(i)=myProjectorPtr->GetStripVsTime_TH2D(i);
    }
}

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
    outputTree->Branch("tracksNo",&tracksNo);
    outputTree->Branch("A", &A);
    outputTree->Branch("Z",&Z);
    outputTree->Branch("momentum",&momentum);
    outputTree->Branch("start",&start);
    outputTree->Branch("stop",&stop);
    outputTree->Branch("energy",&energy);
    outputTree->Branch("length",&length);
}
void AbstractGenerator::writeOutput(){
    outputTree->Write();
}



void AbstractGenerator::setEntry(int i){
    eventNr=i;
}

void AbstractGenerator::clearParameters(){
    tracksNo=0;
    A.clear();
    Z.clear();
    momentum.clear();
    start.clear();
    stop.clear();
    energy.clear();
    length.clear();
}