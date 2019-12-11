#include "FromGeantGenerator.h"
FromGeantGenerator::FromGeantGenerator(): AbstractGenerator(){
}

void FromGeantGenerator::setEntry(int i){
    depTree->GetEntry(i);
    eventNr=i;
}

void FromGeantGenerator::generateTrack(){
    for (uint i=0; i!=x->size();++i){
        myTrack3D.Fill(x->at(i),y->at(i),z->at(i),Edep->at(i));
    }
}

void FromGeantGenerator::loadDataFile(std::string dataFileAddress){
    dataFile=new TFile(dataFileAddress.c_str(),"READ");
    dataFile->GetObject("EDep",depTree);
    dataFile->GetObject("prim",primTree);
    setBranches();
    
}

void FromGeantGenerator::setBranches(){
    depTree->SetBranchAddress("x",&x);
    depTree->SetBranchAddress("y",&y);
    depTree->SetBranchAddress("z",&z);
    depTree->SetBranchAddress("Edep",&Edep);
}