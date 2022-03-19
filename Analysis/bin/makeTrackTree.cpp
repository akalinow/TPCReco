#include <cstdlib>
#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TString.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include "dEdxFitter.h"
#include "TrackBuilder.h"
#include "HistoManager.h"
#include "EventSourceROOT.h"
#ifdef WITH_GET
#include "EventSourceGRAW.h"
#endif
#include "SigClusterTPC.h"
#include "EventTPC.h"
#include "colorText.h"

int makeTrackTree(const  std::string & geometryFileName,
		  const  std::string & dataFileName);

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to the geometry file.")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to data file.");
  
  boost::program_options::variables_map varMap;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
  boost::program_options::notify(varMap); 

  if (varMap.count("help")) {
    std::cout<<cmdLineOptDesc<<std::endl;
    exit(1);
  }
  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  std::string geometryFileName, dataFileName;
  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<3){
    char text[] = "--help";
    char *argvTmp[] = {text, text};
    parseCmdLineArgs(2,argvTmp);
    return 1;
  }
  if (varMap.count("geometryFile")) {
    geometryFileName = varMap["geometryFile"].as<std::string>();
  }
  if (varMap.count("dataFile")) {
    dataFileName = varMap["dataFile"].as<std::string>();
  }

  if(dataFileName.size() && geometryFileName.size()){
    makeTrackTree(geometryFileName, dataFileName);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" dataFile: "<<dataFileName
	     <<std::endl;
  }
  return 0;
}
/////////////////////////////
////////////////////////////
// Define some simple structures
typedef struct {Float_t eventId, frameId,
    eventType,
    length,
    horizontalLostLength, verticalLostLength,
    energy, charge, cosTheta, phi, chi2,
    x0, y0, z0, x1, y1, z1;} TrackData;
/////////////////////////
int makeTrackTree(const  std::string & geometryFileName,
		  const  std::string & dataFileName) {

  int index = -1;
  std::string timestamp;

  std::shared_ptr<EventSourceBase> myEventSource;
  if(dataFileName.find(".graw")!=std::string::npos){
    #ifdef WITH_GET
    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(160);
    index = dataFileName.find("AsAd_ALL_")+9;
    timestamp = dataFileName.substr(index, 26);
    #endif
  }
  else if(dataFileName.find(".root")!=std::string::npos){
    myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
    index = dataFileName.find("EventTPC_")+9;
    timestamp = dataFileName.substr(index, 23);
  }
  else{
    std::cout<<KRED<<"Wrong input file: "<<RST<<dataFileName<<std::endl;
    return -1;
  }

  int npos = dataFileName.size()-index-5;
  std::string rootFileName = "TrackTree_"+dataFileName.substr(index, npos)+".root";
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  tree->Branch("track",&track_data,"eventId:frameId:eventType:length:horizontalLostLength:verticalLostLength:energy:charge:cosTheta:phi:chi2:x0:y0:z0:x1:y1:z1");
  
  TrackBuilder myTkBuilder;
  myTkBuilder.setGeometry(myEventSource->getGeometry());
  dEdxFitter mydEdxFitter;

  HistoManager myHistoManager;
  myHistoManager.setGeometry(myEventSource->getGeometry());
  myHistoManager.toggleAutozoom();
  TCanvas *aCanvas = new TCanvas("aCanvas","Histograms",1000,1000);
  aCanvas->Divide(2,2);
  TH1F *hFrame = new TH1F("hFrame","",1,0,1);
  hFrame->SetMaximum(1.0);
  hFrame->SetStats(kFALSE);
  TLatex *aLatex = new TLatex(0,0,"");
  TString  aString;
 
  myEventSource->loadDataFile(dataFileName);
  std::cout<<KBLU<<"File with "<<RST<<myEventSource->numberOfEntries()<<" frames loaded."<<std::endl;

  //Event loop
  unsigned int nEntries = myEventSource->numberOfEntries();
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    myEventSource->loadFileEntry(iEntry);
    myEventSource->getCurrentEvent()->MakeOneCluster(35, 0, 0);
    if(myEventSource->getCurrentEvent()->GetOneCluster().GetNhits()>20000){
      std::cout<<KRED<<"Noisy event - skipping."<<RST<<std::endl;
      continue;
    }			      
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

    int eventId = myEventSource->getCurrentEvent()->GetEventId();
    const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);      
    double length = aTrack3D.getLength();
    double charge = aTrack3D.getIntegratedCharge(length);
    double cosTheta = cos(aTrack3D.getSegments().front().getTangent().Theta());
    double phi = aTrack3D.getSegments().front().getTangent().Phi();
    double chi2 = aTrack3D.getChi2();
    const TVector3 & start = aTrack3D.getSegments().front().getStart();
    const TVector3 & end = aTrack3D.getSegments().front().getEnd();
    const TVector3 & tangent = aTrack3D.getSegments().front().getTangent();
    TVector3 horizontal(0,-1,0);
    double horizontalTrackLostPart = 73.4/std::abs(horizontal.Dot(tangent));

    TVector3 vertical(0,0,-1);
    double verticalTrackLostPart = 6.0/std::abs(vertical.Dot(tangent));

    TH1F hChargeProfile = aTrack3D.getSegments().front().getChargeProfile();
    int eventType = 0;
    double alphaEnergy = 0.0;
    double carbonEnergy = 0.0;
    if(charge>100 && length>50){
      mydEdxFitter.fitHisto(hChargeProfile);
      eventType = mydEdxFitter.getBestFitEventType();
      alphaEnergy = mydEdxFitter.getAlphaEnergy();
      carbonEnergy = mydEdxFitter.getCarbonEnergy();
    }
      
    track_data.frameId = iEntry;
    track_data.eventId = eventId;
    track_data.eventType = eventType;
    track_data.length = length;    
    track_data.horizontalLostLength = horizontalTrackLostPart;
    track_data.verticalLostLength = verticalTrackLostPart;
    track_data.charge = charge;
    track_data.cosTheta = cosTheta;
    track_data.phi = phi;
    track_data.chi2 = chi2;
    track_data.x0 = start.X();
    track_data.y0 = start.Y();
    track_data.z0 = start.Z();
    track_data.x1 = end.X();
    track_data.y1 = end.Y();
    track_data.z1 = end.Z();
    track_data.energy = alphaEnergy + carbonEnergy;
    tree->Fill();

    ///Draw anomaly events
    if(chi2>10 && charge>100){
      myHistoManager.setEvent(myEventSource->getCurrentEvent());
      for(int strip_dir=0;strip_dir<3;++strip_dir){
	aCanvas->cd(strip_dir+1);
	std::shared_ptr<TH2D> hProjection = myHistoManager.getClusterStripVsTimeInMM(strip_dir);
	hProjection->SetStats(kFALSE);
	hProjection->DrawCopy("colz");
	//myHistoManager.getRecHitStripVsTime(strip_dir)->Draw("box same");
      }
      aCanvas->cd(4);
      hFrame->Draw();
      aString.Form("fit loss: %3.2f",chi2);
      aLatex->DrawLatex(0.1,0.85, aString.Data());

      aString.Form("track charge: %3.2f",charge);
      aLatex->DrawLatex(0.1,0.75, aString.Data());

      aString.Form("track length: %3.2f",length);
      aLatex->DrawLatex(0.1,0.65, aString.Data());

      aString.Form("frame id: %d",iEntry);
      aLatex->DrawLatex(0.1,0.55, aString.Data());

      std::string plotFileName = "Clusters_"+std::to_string(eventId)+"_"+dataFileName.substr(index)+".png";
      aCanvas->Print(plotFileName.c_str());
    }
  }
  outputROOTFile.Write();
  return 0;
}
/////////////////////////////
////////////////////////////

