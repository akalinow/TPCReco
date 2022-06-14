#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_LIBRARY_PATH(../lib)
#endif
#include "TROOT.h"
#include "Track3D.h"
#include "GeometryTPC.h"
//////////////////////////
//
// root
// root [0] .L analyzeRecoEvent.cxx
// root [1] analyze3Alpha()
//
//
//
//////////////////////////
double getKineticEnergyForRange(double range){

  //range [mm] Energy [MeV]
  std::vector<double> params = {0.0075, 0.00308, 0.00106, -1.60361E-5, 7.63072E-8, 1.9926E-10, -1.95833E-12};

  double result = 0.0;
  for(int iPower=0;iPower<params.size();++iPower){
    result += params.at(iPower)*std::pow(range, iPower);
  }
  return result; 
}
/////////////////////////
/////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
//////////////////////////
//////////////////////////
Track3D *loadRecoEvent(const std::string fileName){

  if (!gROOT->GetClass("Track3D")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  
  TFile *aFile = new TFile(fileName.c_str());
  TTree *aTree = (TTree*)aFile->Get("TPCRecoData");
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  aBranch->SetAddress(&aTrack);
  aBranch->GetEntry(0); 
  return aTrack;
}
//////////////////////////
//////////////////////////
void analyze3Alpha(){

  ///Data and geometry loading.
  std::string fileName = "Reco_EventTPC_2021-06-22T12:01:56.568_0.root";
  Track3D *aTrack = loadRecoEvent(fileName);
  fileName = "/home/user1/scratch/akalinow/ELITPC/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat";
  std::shared_ptr<GeometryTPC> aGeometry = loadGeometry(fileName);
  std::cout<<*aTrack<<std::endl;

  ///Fetching tracks segments from the full track.
  TrackSegment3D aSegment = aTrack->getSegments().front();
  aSegment.setGeometry(aGeometry);

  double atomicMassUnit = 931.49410242; //MeV/c^2
  double alphaMass = 4*atomicMassUnit + 2.4249; //A*u + Delta MeV/c2
  double carbonMass = 12*atomicMassUnit;
  TLorentzVector totalP4;

  std::vector<double> chargeProfile;
  std::vector<double> profilePosition;
  double totalLength = 0;
  double maxCharge = 0;
  double stepSize = 2; //[mm]
  for(auto aSegment: aTrack->getSegments()){
    aSegment.setGeometry(aGeometry);
    const TVector3 & direction = (aSegment.getEnd() - aSegment.getStart());
    double length = aSegment.getLength();
    double kineticEnergy = getKineticEnergyForRange(length);
    double energy = kineticEnergy + alphaMass;
    double momentum = sqrt(energy*energy - alphaMass*alphaMass);
    TLorentzVector aP4(direction.Unit()*momentum, energy);
    totalP4 += aP4;
    std::cout<<"Segment:"
	     <<" direction (phi, theta): "
	     << direction.Phi()<<", "<<direction.Theta()<<std::endl
	     <<"\t length [mm] = "<<aSegment.getLength()<<std::endl
	     <<"\t kin. energy [MeV] = "<<kineticEnergy<<std::endl
	     <<"\t total Charge: "<<aSegment.getIntegratedCharge(length)<<std::endl
	     <<"\t kin. energy/charge: "<<kineticEnergy/aSegment.getIntegratedCharge(length)<<std::endl
	     <<"\t charge along track: "; 
    int nSteps = length/stepSize;
    double sum = 0.0;
    for(int iStep=1;iStep<=nSteps;++iStep){
      double charge = aSegment.getIntegratedCharge(iStep*stepSize) -
	aSegment.getIntegratedCharge((iStep-1)*stepSize);
      std::cout<<charge<<", ";
      sum += charge;
      chargeProfile.push_back(charge);
      profilePosition.push_back(iStep*stepSize+totalLength);
      maxCharge = std::max(charge, maxCharge);
    }
    std::cout<<" sum: "<<sum<<std::endl;
    totalLength += length;
  }
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,500);
  TGraph *chargeProfileGraph = new TGraph(profilePosition.size(), profilePosition.data(), chargeProfile.data());
  TH1F *hFrame = new TH1F("hFrame","Charge profile;length [mm]; charge [arbitrary units]",1,0,totalLength);
  hFrame->SetMinimum(0.0);
  hFrame->SetMaximum(1.2*maxCharge);
  hFrame->Draw();
  chargeProfileGraph->SetMarkerStyle(22);
  chargeProfileGraph->Draw("P");
  
  std::cout<<std::endl;
  std::cout<<"Total energy [MeV]:             "<<totalP4.E()<<std::endl
	   <<"Total kin energy [MeV]:         "<<totalP4.E() - 3*alphaMass<<std::endl
	   <<"      momentum [MeV/c]:         "<<totalP4.P()<<std::endl
    	   <<"      invariant mass [MeV/c^2]: "<<totalP4.M()<<std::endl;
}
//////////////////////////
//////////////////////////
void plotTrackSegment(int strip_dir, const std::string & dataFileName, const std::string & geometryFileName){

  ///Data and geometry loading.
  Track3D *aTrack = loadRecoEvent(dataFileName);
  std::shared_ptr<GeometryTPC> aGeometry = loadGeometry(geometryFileName);
  std::cout<<*aTrack<<std::endl;

  ///Fetching tracks segments from the full track.
  TrackSegment3D aSegment = aTrack->getSegments().front();
  aSegment = aTrack->getSegments().back();
  aSegment.setGeometry(aGeometry);

  ///Choose the U direction and get the 2D segment
  double startLambda = 0;
  double endLambda = aSegment.getLength();
  TrackSegment2D aStripProjection = aSegment.get2DProjection(strip_dir, startLambda, endLambda);
  const TVector3 & start = aStripProjection.getStart();
  const TVector3 & end = aStripProjection.getEnd();
  std::cout<<aStripProjection<<std::endl;

  ///Some simple plots for the 2D segment.
  TH2F *hProjection = new TH2F("hProjection",";x [mm]; y[mm]; charge",100,start.X(), end.X(), 100, start.Y(), end.Y());
  hProjection->SetStats(false);
  TH1D *hDistance = new TH1D("hDistance",";hit distance [mm];charge",10,0,10);
  hDistance->SetStats(false);
  TVector3 aPoint;
  double distance = 0.0;
  for(auto aHit: aSegment.getRecHits().at(strip_dir)){
    double x = aHit.getPosTime();
    double y = aHit.getPosStrip();
    double charge = aHit.getCharge();
    aPoint.SetXYZ(x,y,0);
    distance = aStripProjection.getPointTransverseDistance(aPoint);
    //std::cout<<"x,y: "<<x<<", "<<y<<" charge: "<<charge<<" distance: "<<distance<<std::endl;

    if(distance>0 && distance<10){
      hProjection->Fill(x,y,charge);
    }
    hDistance->Fill(distance,charge);
  }
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,500);
  TLine *aSegment2DLine = new TLine(start.X(), start.Y(),  end.X(),  end.Y());
  aSegment2DLine->SetLineColor(2);
  aSegment2DLine->SetLineWidth(2);
  aCanvas->Divide(2,1);
  aCanvas->cd(1);
  hProjection->Draw("colz");
  aSegment2DLine->Draw();
  aCanvas->cd(2);
  hDistance->Draw("colz");
}
//////////////////////////
//////////////////////////

