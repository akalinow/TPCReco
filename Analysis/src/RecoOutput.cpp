#include <cstdlib>
#include <iostream>

#include "TFile.h"
#include "TTree.h"


#include "Track3D.h"
#include "RecoOutput.h"
#include "colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecoOutput::RecoOutput() {

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecoOutput::~RecoOutput() {
  
  close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecoOutput::setRecTrack(const Track3D & aRecTrack){

  *myTrackPtr = aRecTrack;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecoOutput::setEventInfo(const eventraw::EventInfo & aEventInfo){

  *myEventInfoPtr = aEventInfo;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecoOutput::open(const std::string & fileName){

  if(!myTrackPtr){
    std::cout<<KRED<<"RecoOutput::open"<<RST
	     <<" pointer to fitted track not set!"
	     <<std::endl;
    return;
  }
  std::cout<<KBLU<<"Opening reco output stream to file: "<<RST<<fileName<<std::endl;

  std::string treeName = "TPCRecoData";
  myOutputFilePtr = std::make_shared<TFile>(fileName.c_str(),"RECREATE");
  myOutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  
  myOutputTreePtr->Branch("RecoEvent", "Track3D", &myTrackPtr);
  myOutputTreePtr->Branch("EventInfo", "EventInfo", &myEventInfoPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecoOutput::close(){

  if(!myOutputFilePtr){
     std::cout<<KRED<<"RecoOutput::close"<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
     return;
  }
  myOutputFilePtr->Close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecoOutput::update(){

  if(!myOutputTreePtr){
     std::cout<<KRED<<"RecoOutput::update"<<RST
	     <<" pointer to output tree not set!"
	     <<std::endl;
     return;
  }
  myOutputTreePtr->Fill();
  myOutputTreePtr->Write("", TObject::kOverwrite);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

