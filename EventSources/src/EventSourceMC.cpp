#include <iostream>

#include "TPCReco/colorText.h"
#include "TPCReco/EventSourceMC.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMC::EventSourceMC(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);

  int NbinsX=50, NbinsY=50, NbinsZ=50;
  double xmin=-150,  ymin=-100, zmin=-100;
  double xmax=150,  ymax=100,  zmax=100;

  braggGraph_alpha = new TGraph("dEdx_corr_alpha_10MeV_CO2_250mbar.dat", "%lg %lg");
  braggGraph_12C = new TGraph("dEdx_corr_12C_5MeV_CO2_250mbar.dat", "%lg %lg");
  double nominalPressure = 250.0; //[mbar]
  braggGraph_alpha_energy = 10; // [MeV]
  braggGraph_12C_energy = 5; // [MeV]

  keVToChargeScale = 100.0; // 1 keV = 100 charge in arb. units
  
  myRangeCalculator.setGasPressure(nominalPressure);

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
void EventSourceMC::loadDataFile(const std::string & fileName){ }
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

  double minX = -150, minY = -10, minZ = -10;
  double maxX = 150, maxY = 10, maxZ = 10;
  double x = myRndm.Uniform(minX, maxX);
  double y = myRndm.Uniform(minY, maxY);
  double z = myRndm.Uniform(minZ, maxZ);

/*
  x = -0.433035;
  y = -0.75;
  z =  0.0;
  */
/*
  x = 10.0;
  y = 10.0;
  z = 10.0;
  */  
  TVector3 aVertex(x,y,z);
  return aVertex;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D EventSourceMC::createSegment(const TVector3 vertexPos, pid_type ion_id, double energy) const{
  
  double theta = 0, phi = 0.0;
  double minCosTheta = -1, maxCosTheta = 1;
  double minPhi = 0, maxPhi = 2*M_PI;
  double length = myRangeCalculator.getIonRangeMM(ion_id, energy);

  /// fixed test configurations for first 3 events 
  if(myCurrentEntry==10){ 
    theta = 0.0;
    phi = 0.0;
  }
  else if(myCurrentEntry==11){ 
    theta = 0.0;
    phi = M_PI/2.0;
  }
  else if(myCurrentEntry==12){
    theta = M_PI/2.0;
    phi = M_PI/2.0;
  }
  else if(myCurrentEntry==13){
    theta = M_PI/2.0;
    phi = M_PI/4.0;
  }
  else{
    theta = TMath::ACos(myRndm.Uniform(minCosTheta, maxCosTheta));
    phi = myRndm.Uniform(minPhi, maxPhi);
  }
 
 //two prong event - take care of momentum conservation in CM
 if (ion_id==pid_type::CARBON_12){
    const TVector3 &aTangent = -myTracks3D.front().getSegments().front().getTangent();
    phi = aTangent.Phi();
    theta = aTangent.Theta();
  }

  std::cout<<KBLU<<"Phi: "<<RST<<phi<<" deg: "<<phi*180/M_PI<<std::endl;
   std::cout<<KBLU<<"Theta: "<<RST<<theta<<" deg: "<<theta*180/M_PI<<std::endl;
 
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
  
  double dEdx_max_energy =0.0, graph_range = 0.0;
  TGraph *dEdx_graph=0;
  
  if(ion_id==pid_type::ALPHA){
    dEdx_max_energy = braggGraph_alpha_energy;
    dEdx_graph=braggGraph_alpha;
    graph_range = 299;
  }
    else if(ion_id==pid_type::CARBON_12){
      dEdx_max_energy = braggGraph_12C_energy;
      dEdx_graph = braggGraph_12C;
      graph_range = 23.5;
    }
  
  double max_ion_range = myRangeCalculator.getIonRangeMM(ion_id, dEdx_max_energy);
  double delta = max_ion_range - ion_range;

  double x = 0.0;
  double value = 0.0;
  TH1F aChargeProfile{"hChargeProfile",";x [mm];dE/dx [keV/bin width]", 1024, -0.2*ion_range, 1.2*ion_range};
  for(int iBinX=1;iBinX<=aChargeProfile.GetNbinsX();++iBinX){
    x = aChargeProfile.GetBinCenter(iBinX);
    if(x<0 || (x+delta>graph_range)) continue;
    value = dEdx_graph->Eval(x+delta)*aChargeProfile.GetBinWidth(iBinX);
    aChargeProfile.SetBinContent(iBinX, value);
  }
  
  std::cout<<KBLU<<"Charge profile sum [keV]: \t"<<RST<<aChargeProfile.Integral()<<std::endl;
  std::cout<<KBLU<<"Ion energy from range calculator [keV]: \t"<<RST<<myRangeCalculator.getIonEnergyMeV(ion_id, ion_range)*1E3<<std::endl;
  return aChargeProfile;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D EventSourceMC::createTrack(const TVector3 & aVtx, pid_type ion_id, double energy) const{

  Track3D aTrack;
  TrackSegment3D aSegment = createSegment(aVtx, ion_id, energy);
  TH1F hChargeProfile = createChargeProfile(aSegment.getLength(), ion_id);
  aTrack.addSegment(aSegment);
  aTrack.setChargeProfile(hChargeProfile);
  return aTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::fill3DChargeCloud(const Track3D & aTrack){

  my3DChargeCloud.Reset();  
  TVector3 depositPosition, smearedPosition;
  TH1F hChargeProfile = aTrack.getChargeProfile();
  double lambda = 0.0, value = 0.0;
  double sigma = 2.0;
  int nTries = 50;
  
  for(int iBin=0;iBin<hChargeProfile.GetNbinsX();++iBin){
    value = hChargeProfile.GetBinContent(iBin);
    if(!value) continue;
    lambda = hChargeProfile.GetBinCenter(iBin);
    depositPosition = aTrack.getSegments().front().getStart() + lambda*aTrack.getSegments().front().getTangent();

    for(int iTry=0;iTry<nTries;++iTry){
      smearedPosition = TVector3(myRndm.Gaus(depositPosition.X(), sigma),
				 myRndm.Gaus(depositPosition.Y(), sigma),
				 myRndm.Gaus(depositPosition.Z(), sigma));
      my3DChargeCloud.Fill(smearedPosition.X(), smearedPosition.Y(), smearedPosition.Z(), value/nTries*keVToChargeScale);
    }
  }
  std::cout<<KBLU<<"Total charge cloud energy [100*keV]: "<<RST<<my3DChargeCloud.Integral()<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::fillPEventTPC(const TH3D & h3DChargeCloud, const Track3D & aTrack){

  int iPolyBin = 0;
  double value = 0.0, totalCharge = 0.0;
  bool err_flag = false;  
  double sigma = 2.0;
  int nTries = 50;
  
  double lambda = 0.0;

  int iCell = 0;
  TVector3 depositPosition, smearedPosition;
  TH1F hChargeProfile = aTrack.getChargeProfile();

  for(int iBin=0;iBin<hChargeProfile.GetNbinsX();++iBin){
    value = hChargeProfile.GetBinContent(iBin);
    if(!value) continue;
    lambda = hChargeProfile.GetBinCenter(iBin);
    depositPosition = aTrack.getSegments().front().getStart() + lambda*aTrack.getSegments().front().getTangent();
    for(int iTry=0;iTry<nTries;++iTry){
      smearedPosition = TVector3(myRndm.Gaus(depositPosition.X(), sigma),
				                         myRndm.Gaus(depositPosition.Y(), sigma),
				                         myRndm.Gaus(depositPosition.Z(), sigma));
      iPolyBin = myGeometryPtr->GetTH2Poly()->FindBin(smearedPosition.X(), smearedPosition.Y());
      iCell = myGeometryPtr->Pos2timecell(smearedPosition.Z(), err_flag);
      std::shared_ptr<StripTPC> aStrip = myGeometryPtr->GetTH2PolyStrip(iPolyBin);
      if(aStrip && !err_flag){
        myCurrentPEvent->AddValByStrip(aStrip, iCell, value/nTries*keVToChargeScale);
        totalCharge+=value/nTries*keVToChargeScale;
      }
    }
  }
  std::cout<<KBLU<<"Total charge generated [100*keV]: "<<RST<<totalCharge<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateSingleProng(pid_type ion_id){

  myGenEventType = ion_id;

  ///Assume energy conservation in CM frame for two prong events
  ///alpha takes 3/4 energy, C12 takes 1/4 energy
  ///assume energy in CMS from 1 to 7 MeV
  double min_E_CM = 4*3/4.0, max_E_CM = 7.0*3/4.0;
  double energy_CM = myRndm.Uniform(min_E_CM, max_E_CM);

  TVector3 aVtx = createVertex();
  myTracks3D.push_back(createTrack(aVtx, ion_id, energy_CM));
  std::cout<<KBLU<<"Generated track: "<<RST<<std::endl;
  std::cout<<myTracks3D.back()<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateTwoProng(){

  ///Assume energy conservation in CM frame for two prong events
  ///alpha takes 3/4 energy, C12 takes 1/4 energy
  double min_E_CM = 4.0, max_E_CM = 7.0;
  double energy_CM = myRndm.Uniform(min_E_CM, max_E_CM);

  TVector3 aVtx = createVertex();

  double energy_first = energy_CM*3/4.0;
  pid_type ion_id_first = pid_type::ALPHA;
  myTracks3D.push_back(createTrack(aVtx, ion_id_first, energy_first));
  std::cout<<KBLU<<"First generated track: "<<RST<<std::endl;
  std::cout<<myTracks3D.back()<<std::endl;

  double energy_second = energy_CM*1/4.0;
  pid_type ion_id_second = pid_type::CARBON_12;
  myTracks3D.push_back(createTrack(aVtx, ion_id_second, energy_second));
  std::cout<<KBLU<<"Second generated track: "<<RST<<std::endl;
  std::cout<<myTracks3D.back()<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateThreeProng(){

  myGenEventType = pid_type::THREE_ALPHA;

  pid_type ion_id = pid_type::ALPHA;
  TVector3 aVtx = createVertex();

  double min_E_CM = 1, max_E_CM = 7.0;
  double energy_CM = myRndm.Uniform(min_E_CM, max_E_CM);

  double fractions[3];
  fractions[0] = myRndm.Uniform(0,1);
  fractions[1] = myRndm.Uniform(0,1-fractions[0]);
  fractions[2] = 1-fractions[0]-fractions[1];

  int nParts = 3;
  for(int iPart=0;iPart<nParts;++iPart){
    myTracks3D.push_back(createTrack(aVtx, ion_id, energy_CM*fractions[iPart]));
    std::cout<<KBLU<<"Generated track number: "<<std::to_string(iPart)<<RST<<std::endl;
    std::cout<<myTracks3D.back()<<std::endl;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMC::generateEvent(){

  myCurrentEventInfo.SetEventId(myCurrentEntry);
  myCurrentEventInfo.SetRunId(0);
  myCurrentEventInfo.SetEventTimestamp(0);
  myCurrentEventInfo.SetPedestalSubtracted(true);
  
  myCurrentPEvent->Clear();
  myCurrentPEvent->SetEventInfo(myCurrentEventInfo);

  myTracks3D.clear();
  
  /*
  double aRndm = myRndm.Uniform(0,1);
  if(aRndm<0.33) generateSingleProng();
  else if (aRndm<2*0.33) generateTwoProng();
  else generateThreeProng();
  */
  //generateSingleProng();
  generateTwoProng();
  
  for(const auto & aTrack: myTracks3D) fillPEventTPC(my3DChargeCloud, aTrack);
  fillEventTPC();
  ++myCurrentEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

