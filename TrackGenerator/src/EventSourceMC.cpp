#include <iostream>

#include "colorText.h"
#include "EventSourceMC.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMC::EventSourceMC(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);

  int NbinsX=50, NbinsY=50, NbinsZ=50;
  double xmin=-100,  ymin=-100, zmin=-100;
  double xmax=100,  ymax=100,  zmax=100; 

  my3DChargeCloud = TH3D("h3DChargeCloud", "charge cloud [arb. units];X [mm];Y [mm];Z [mm]",
			 NbinsX, xmin, xmax,
			 NbinsY, ymin, ymax,
			 NbinsZ, zmin, zmax);
  
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
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMC::getPreviousEvent(){

  generateEvent();
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadEventId(unsigned long int iEvent){

  myCurrentEntry = iEvent;
  generateEvent();
 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::loadGeometry(const std::string & fileName){

  EventSourceBase::loadGeometry(fileName);
  myProjectorPtr.reset(new UVWprojector(myGeometryPtr));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 EventSourceMC::createVertex() const{

  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  TVector3 aVertex(x,y,z);
  return aVertex;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D EventSourceMC::createSegment(const TVector3 vertexPos, pid_type ion_id) const{

  double length = 60;
  double theta = 0, phi = 0.0;
 
  if(myCurrentEntry==0){
    theta = 0.0;
    phi = 0.0;
  }
  else if(myCurrentEntry==1){
    theta = M_PI/2.0;
    phi = M_PI/4.0;
  }
  else if(myCurrentEntry==2){
    theta = M_PI/4.0;
    phi = M_PI/4.0;
  }
  else{
    theta = myRndm.Uniform(0, M_PI);
    phi = myRndm.Uniform(0, 2*M_PI);
    length = myRndm.Uniform(20, 60);
  }

  TVector3 tangent;
  tangent.SetMagThetaPhi(1.0, theta, phi);

  TrackSegment3D aSegment;
  aSegment.setGeometry(myGeometryPtr);
  aSegment.setStartEnd(vertexPos, vertexPos + tangent*length);
  aSegment.setPID(ion_id);
  
  return aSegment;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH1F EventSourceMC::createChargeProfile(double ion_range, pid_type ion_id) const{

  TGraph* braggGraph_alpha = new TGraph("dEdx_corr_alpha_10MeV_CO2_250mbar.dat", "%lg %lg");
  //TGraph* braggGraph_12C = new TGraph("dEdx_corr_12C_5MeV_CO2_250mbar.dat", "%lg %lg");
  //double nominalPressure = 250.0;
  //IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2, nominalPressure, 293.15);
  //double energy = 10.0;
  //double braggGraph_alpha_energy = 10;
  double max_ion_range = 297.23;//myRangeCalculator.getIonRangeMM(ionId, braggGraph_alpha_energy);
  double graph_range = 299;//myRangeCalculator.getIonRangeMM(ionId, braggGraph_alpha_energy);
  double delta = max_ion_range - ion_range;

  double x = 0.0;
  double value = 0.0;
  TH1F aChargeProfile{"hChargeProfile",";x [mm];dE/dx [keV/mm]", 1024, -0.2*ion_range, 1.2*ion_range};
  for(int iBinX=1;iBinX<=aChargeProfile.GetNbinsX();++iBinX){
    x = aChargeProfile.GetBinCenter(iBinX);
    if(x<0) continue;
    if(x+delta>graph_range) continue;
    value = braggGraph_alpha->Eval(x+delta);
    aChargeProfile.SetBinContent(iBinX, value);
  }
  
  std::cout<<"ion energy: "<<braggGraph_alpha->Integral(delta, graph_range)<<std::endl;
  aChargeProfile.SaveAs("generated_charge_profile.root");
  return aChargeProfile;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D EventSourceMC::createTrack() const{

  Track3D aTrack;
  pid_type ion_id = pid_type::ALPHA;
  TVector3 aVtx = createVertex();
  TrackSegment3D aSegment = createSegment(aVtx, ion_id);
  TH1F hChargeProfile = createChargeProfile(aSegment.getLength(), ion_id);
  aTrack.addSegment(aSegment);
  aTrack.setChargeProfile(hChargeProfile);
  return aTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::fill3DChargeCloud(const Track3D & aTrack){

  my3DChargeCloud.Reset();
  TVector3 depositPosition;
  TH1F hChargeProfile = aTrack.getChargeProfile();
  double lambda = 0.0, value = 0.0;
  double scale = 1.0;
  int iBinTmp = 0;
  for(int iBin=0;iBin<hChargeProfile.GetNbinsX();++iBin){
    value = hChargeProfile.GetBinContent(iBin);
    if(!value) continue;
    lambda = hChargeProfile.GetBinCenter(iBin);
    depositPosition = aTrack.getSegments().front().getStart() + lambda*aTrack.getSegments().front().getTangent(); 
    iBinTmp = my3DChargeCloud.FindBin(depositPosition.X(), depositPosition.Y(), depositPosition.Z());
    my3DChargeCloud.SetBinContent(iBinTmp, value*scale);
  }
  std::cout<<"total charge cloud: "<<my3DChargeCloud.Integral()<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::fillPEventTPC(const Track3D & aTrack){

  /*
  myProjectorPtr->SetEvent3D(my3DChargeCloud);
  myProjectorPtr->fillPEventTPC(myCurrentPEvent);
  */
  double sigma = 2.0;
  int nTries = 100;
  double smearWeight = 1.0;
  
  double lambda = 0.0;
  double value = 0.0;
  int iCell = 0, iPolyBin = 0;
  bool err_flag = false;
  TVector3 depositPosition, smearedPosition, smearVect;
  TH1F hChargeProfile = aTrack.getChargeProfile();
  double totalCharge = 0.0;
  
  for(int iBin=0;iBin<hChargeProfile.GetNbinsX();++iBin){
    value = hChargeProfile.GetBinContent(iBin);
    if(!value) continue;
    lambda = hChargeProfile.GetBinCenter(iBin);
    depositPosition = aTrack.getSegments().front().getStart() + lambda*aTrack.getSegments().front().getTangent();

    for(int iTry=0;iTry<nTries;++iTry){
      smearedPosition = TVector3(myRndm.Gaus(depositPosition.X(), sigma),
				 myRndm.Gaus(depositPosition.Y(), sigma),
				 myRndm.Gaus(depositPosition.Z(), sigma));
      smearWeight = TMath::Gaus((smearedPosition-depositPosition).Mag(), 0, sigma, true);
      iPolyBin = myGeometryPtr->GetTH2Poly()->FindBin(smearedPosition.X(), smearedPosition.Y());
      iCell = myGeometryPtr->Pos2timecell(smearedPosition.Z(), err_flag);
      std::shared_ptr<StripTPC> aStrip = myGeometryPtr->GetTH2PolyStrip(iPolyBin);
      if(aStrip){
	auto key = std::make_tuple(aStrip->Dir(), aStrip->Section(), aStrip->Num(), iCell);
	if(true || myCurrentPEvent->GetChargeMap().find(key)==myCurrentPEvent->GetChargeMap().end()){
	  myCurrentPEvent->AddValByStrip(aStrip, iCell, value*smearWeight);
	  totalCharge+=value*smearWeight;
	}
      }
    }
  }
  std::cout<<"Total charge generated: "<<totalCharge<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateEvent(){

  myCurrentEventInfo.SetEventId(myCurrentEntry);
  myCurrentEventInfo.SetRunId(0);
  myCurrentEventInfo.SetEventTimestamp(0);
  myCurrentEventInfo.SetAsadCounter(4);
  myCurrentEventInfo.SetPedestalSubtracted(true);
  
  myCurrentPEvent->Clear();
  myCurrentPEvent->SetEventInfo(myCurrentEventInfo);
  
  myTrack3D = createTrack();
  fill3DChargeCloud(myTrack3D);
  fillPEventTPC(myTrack3D);
  fillEventTPC();
  ++myCurrentEntry;

  std::cout<<KBLU<<"Generated track: "<<RST<<std::endl;
  std::cout<<myTrack3D<<std::endl;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
