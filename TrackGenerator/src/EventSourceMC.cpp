#include <iostream>

#include "colorText.h"
#include "EventSourceMC.h"

//#include "IonRangeCalculator.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMC::EventSourceMC(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);

  nEntries = 9999;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMC::~EventSourceMC(){ }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadDataFile(const std::string & fileName){
   
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadFileEntry(unsigned long int iEntry){

  generateEvent();
  myCurrentEntry = iEntry;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceMC::numberOfEvents() const{ return nEntries; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMC::getNextEvent(){

  generateEvent();
  ++myCurrentEntry;
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMC::getPreviousEvent(){

  generateEvent();
  ++myCurrentEntry;
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadEventId(unsigned long int iEvent){

  generateEvent();
 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadGeometry(const std::string & fileName){

  EventSourceBase::loadGeometry(fileName);
  mySegment.setGeometry(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TVector3 & EventSourceMC::getVertex(){

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  
  myVertex.SetX(x);
  myVertex.SetY(y);
  myVertex.SetZ(z);
  return myVertex;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment3D  & EventSourceMC::getSegment(const TVector3 vertexPos, pid_type ion_id){

  double range = 80;
  double theta = 0.0;
  double phi = 0.0;
  TVector3 trackVect;
  trackVect.SetMagThetaPhi(range, theta, phi);	
  
  mySegment.setStartEnd(vertexPos, vertexPos + trackVect);
  mySegment.setPID(ion_id);
  
  return mySegment;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH1F & EventSourceMC::getChargeProfile(double ion_range, pid_type ion_id){

  TGraph* braggGraph_alpha = new TGraph("dEdx_corr_alpha_10MeV_CO2_250mbar.dat", "%lg %lg");
  //TGraph* braggGraph_12C = new TGraph("dEdx_corr_12C_5MeV_CO2_250mbar.dat", "%lg %lg");
  //double nominalPressure = 250.0;
  //IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2, nominalPressure, 293.15);
  //double energy = 10.0;
  //double braggGraph_alpha_energy = 10;
  double graph_range = 297.23;//myRangeCalculator.getIonRangeMM(ionId, braggGraph_alpha_energy);
  double delta = graph_range - ion_range;

  double x = 0.0;
  double value = 0.0;
  for(int iBinX=1;iBinX<myChargeProfile.GetNbinsX();++iBinX){
    x = myChargeProfile.GetBinCenter(iBinX);
    if(x<0) continue;
    x *= ion_range;
    if(x+delta>graph_range) continue;
    value = braggGraph_alpha->Eval(x+delta);

    myChargeProfile.SetBinContent(iBinX, value);
  }
  
  return myChargeProfile;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::createTrack(){

  pid_type ion_id = pid_type::ALPHA;
  const TVector3 & aVtx = getVertex();
  const TrackSegment3D &aSegment = getSegment(aVtx, ion_id);
  const TH1F & hChargeProfile = getChargeProfile(aSegment.getLength(), ion_id);
  myTrack.addSegment(aSegment);
  myTrack.setChargeProfile(hChargeProfile);

  std::cout<<myTrack<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::fillChargeMap(const Track3D & aTrack){

  double lambda = 0.0;
  double value = 0.0;
  int iCell = 0, iPolyBin = 0;
  TVector3 depositPosition;
  TH1F hChargeProfile = aTrack.getChargeProfile();
  for(int iBin=0;iBin<hChargeProfile.GetNbinsX();++iBin){
    value = hChargeProfile.GetBinContent(iBin);    
    iCell = 256;
    lambda = hChargeProfile.GetBinCenter(iBin);
    depositPosition = aTrack.getSegments().front().getStart() + lambda*aTrack.getSegments().front().getTangent();

    aTrack.getSegments().front().getStart().Print();
    aTrack.getSegments().front().getTangent().Print();
    
    iPolyBin = myGeometryPtr->GetTH2Poly()->FindBin(depositPosition.X(), depositPosition.Y());
    std::shared_ptr<StripTPC> aStrip = myGeometryPtr->GetTH2PolyStrip(iPolyBin);
    if(aStrip){
      myCurrentPEvent->AddValByStrip(aStrip, iCell, value);
      //std::cout<<"(x,y): "<<depositPosition.X()<<", "<<depositPosition.Y()<<" "
      //       <<*aStrip<<" value: "<<value<<std::endl;
    }
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateEvent(){

  createTrack();
  fillChargeMap(myTrack);
  fillEventTPC();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
