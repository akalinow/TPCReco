#include "FromTransportGenerator.h"
#include "SimEvent.hh"
FromTransportGenerator::FromTransportGenerator(): AbstractGenerator(){
    persistentEvent=&myEvent;
}

void FromTransportGenerator::generateTrack(){
    myTrack3D=*(reinterpret_cast<TH3D*>(simEvent->GetAfterTransportHisto()));
    //myTrack3D=*(reinterpret_cast<TH3D*>(simEvent->GetAfterTransportHisto()));
}

void FromTransportGenerator::loadDataFile(std::string dataFileAddress){
    dataFile=new TFile(dataFileAddress.c_str(),"READ");
    dataFile->GetObject("t",dataTree);
    setBranches();
}

void FromTransportGenerator::setBranches(){
    dataTree->SetBranchAddress("evt",&simEvent);
}
void FromTransportGenerator::setEntry(int i){
    dataTree->GetEntry(i);
    eventNr=i;
}

