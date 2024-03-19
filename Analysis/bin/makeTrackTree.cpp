#include <cstdlib>
#include <iostream>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TString.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include <boost/program_options.hpp>

#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/dEdxFitter.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/EventSourceROOT.h"
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/RecoOutput.h"
#include "TPCReco/RunIdParser.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/MakeUniqueName.h"
#include "TPCReco/colorText.h"
#include "TPCReco/HistoManager.h"

#include "TPCReco/EventTPC.h"
/////////////////////////////////////
/////////////////////////////////////
std::string createROOTFileName(const  std::string & grawFileName){

  std::string rootFileName = grawFileName;
  std::string::size_type index = rootFileName.find(",");
  if(index!=std::string::npos){
    rootFileName = grawFileName.substr(0,index);
  }
  index = rootFileName.rfind("/");
  if(index!=std::string::npos){
    rootFileName = rootFileName.substr(index+1,-1);
  }
  if(rootFileName.find("CoBo_ALL_AsAd_ALL")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo_ALL_AsAd_ALL").size(),"TrackTree");
  }
  else if(rootFileName.find("CoBo0_AsAd")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo0_AsAd").size()+1,"TrackTree");
  }
  else if(rootFileName.find("EventTPC")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("EventTPC").size(),"TrackTree");
  }
  else{
    std::cout<<KRED<<"File format unknown: "<<RST<<rootFileName<<std::endl;
    exit(1);
  }
  index = rootFileName.rfind("graw");
  if(index!=std::string::npos){
    rootFileName = rootFileName.replace(index,-1,"root");
  }
  
  return rootFileName;
}
/////////////////////////////////////
/////////////////////////////////////
int makeTrackTree(const  std::string & geometryFileName,
		  const  std::string & dataFileName,
		  unsigned int maxNevents,
		  bool debugMode);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>()->required(), "string - path to the geometry file.")
    ("dataFile",  boost::program_options::value<std::string>()->required(), "string - path to data file.")
    ("maxNevents", boost::program_options::value<unsigned int>()->default_value(0), "int - number of events to process")
    ("debugMode", boost::program_options::bool_switch()->default_value(false), "bool - flag to enable debug plots (default=FALSE)");
  
  boost::program_options::variables_map varMap;        
try {     
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << "makeTrackTree" << "\n\n";
      std::cout << cmdLineOptDesc << std::endl;
      exit(1);
    }
    boost::program_options::notify(varMap);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::cout << cmdLineOptDesc << std::endl;
    exit(1);
  }

  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  std::string geometryFileName, dataFileName;
  bool debugMode{false};
  unsigned int maxNevents{0U};
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
  if (varMap.count("maxNevents")) {
    maxNevents = varMap["maxNevents"].as<unsigned int>();
  }
  if (varMap.count("debugMode")) {
    debugMode = varMap["debugMode"].as<bool>();
  }

  int nEntriesProcessed = 0;
  if(dataFileName.size() && geometryFileName.size()){
    nEntriesProcessed = makeTrackTree(geometryFileName, dataFileName, maxNevents, debugMode);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" dataFile: "<<dataFileName<<"\n"
	     <<" maxNevents: "<<maxNevents<<"\n"
	     <<" debugMode: "<<debug
	     <<std::endl;
  }

  aStopwatch.Stop();
  std::cout<<KBLU<<"Real time:       "<<RST<<aStopwatch.RealTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"CPU time:        "<<RST<<aStopwatch.CpuTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"Processing rate: "<<RST<<nEntriesProcessed/aStopwatch.RealTime()<< " ev/s"<<std::endl;

  return 0;
}
/////////////////////////////
////////////////////////////
// Define some simple structures
typedef struct {Float_t eventId, frameId,
    eventType,
    length,
    horizontalLostLength, verticalLostLength,
    alphaEnergy, carbonEnergy,
    alphaRange, carbonRange,
    cosPhiSegments,
    charge, cosTheta, phi, chi2,
    hypothesisChi2,
    xVtx, yVtx, zVtx,
    xAlphaEnd, yAlphaEnd, zAlphaEnd,
    xCarbonEnd, yCarbonEnd, zCarbonEnd,
    total_mom_x,  total_mom_y,  total_mom_z,
    lineFitChi2, dEdxFitChi2, dEdxFitSigma;
    } TrackData;
/////////////////////////
int makeTrackTree(const  std::string & geometryFileName,
		  const  std::string & dataFileName,
		  unsigned int maxNevents,
		  bool debugMode) {

  std::shared_ptr<EventSourceBase> myEventSource;
  if(dataFileName.find(".graw")!=std::string::npos){

    boost::property_tree::ptree property_tree;
    property_tree.put("pedestal.minPedestalCell", 5.0);
    property_tree.put("pedestal.maxPedestalCell",25);
    property_tree.put("pedestal.minSignalCell",5);
    property_tree.put("pedestal.maxSignalCell",506);
    
    #ifdef WITH_GET
    if(dataFileName.find(",")!=std::string::npos){
      myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
    }
    else{
      myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
      dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(160);
    }
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(property_tree.find("pedestal")->second);    
    #endif
  }
  else if(dataFileName.find(".root")!=std::string::npos){
    myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
  }
  else{
    std::cout<<KRED<<"Wrong input file: "<<RST<<dataFileName<<std::endl;
    return -1;
  }

  std::string rootFileName = createROOTFileName(dataFileName);
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  std::string leafNames = "";
  leafNames += "eventId:frameId:eventType:";
  leafNames += "length:horizontalLostLength:verticalLostLength:";
  leafNames += "alphaEnergy:carbonEnergy:alphaRange:carbonRange:";
  leafNames += "cosPhiSegments:";
  leafNames += "charge:cosTheta:phi:chi2:hypothesisChi2:";
  leafNames += "xVtx:yVtx:zVtx:";
  leafNames += "xAlphaEnd:yAlphaEnd:zAlphaEnd:";
  leafNames += "xCarbonEnd:yCarbonEnd:zCarbonEnd:";
  leafNames += "total_mom_x:total_mom_y:total_mom_z:";
  leafNames += "lineFitChi2:dEdxFitChi2:dEdxFitSigma";
  tree->Branch("track",&track_data,leafNames.c_str());

  int index = geometryFileName.find("mbar");
  double pressure = stof(geometryFileName.substr(index-3, 3));
  TrackBuilder myTkBuilder;
  myTkBuilder.setGeometry(myEventSource->getGeometry());
  myTkBuilder.setPressure(pressure);
  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,pressure,293.15);

  ////////////////////////////////////////////
  //
  // extra initialization for fit DEBUG plots
  HistoManager myHistoManager;
  const std::string rootFileNameCanvas = "makeTrackTree_debug_histos.root";
  TFile *outputCanvasROOTFile = nullptr;
  TCanvas *outputCanvas = nullptr;
  if(debugMode) {
    outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
    if(!outputCanvasROOTFile) {
      std::cout<<KRED<<"Cannot create output ROOT file with debug plots!"<<RST<<std::endl;
      return -1;
    }
    outputCanvasROOTFile->cd();
    const int nx=4, ny=3;
    assert((nx*ny)>=3+1+3+3); // 3 for UVW rawdata, , 1 for dE/dx fit, 3 for UVW RecHits, 3 for UVW Hough accumulators
    outputCanvas=new TCanvas("c_result", "c_result", 1.1*400*nx, 400*ny);
    if(!outputCanvas) {
      std::cout<<KRED<<"Cannot create TCanvas with debug plots!"<<RST<<std::endl;
      return -1;
    }
    outputCanvas->Divide(nx,ny);
    myHistoManager.clearCanvas(outputCanvas, false);
    myHistoManager.setGeometry(myEventSource->getGeometry());
    myHistoManager.setPressure(pressure);
    const boost::property_tree::ptree aConfig; // dummy config tree
    myHistoManager.setConfig(aConfig); // apply default hit clustering parameters
    myHistoManager.toggleAutozoom(); // start auto zoom feature
  }
  //
  ////////////////////////////////////////////

  RecoOutput myRecoOutput;
  std::string fileName = InputFileHelper::tokenize(dataFileName)[0];
  std::size_t last_dot_position = fileName.find_last_of(".");
  std::size_t last_slash_position = fileName.find_last_of("//");
  std::string recoFileName = MakeUniqueName("Reco_"+fileName.substr(last_slash_position+1,
						     last_dot_position-last_slash_position-1)+".root");
  std::shared_ptr<eventraw::EventInfo> myEventInfo = std::make_shared<eventraw::EventInfo>();
  myRecoOutput.open(recoFileName);
 
  myEventSource->loadDataFile(dataFileName);
  std::cout<<KBLU<<"File with "<<RST<<myEventSource->numberOfEntries()<<" frames loaded."<<std::endl;

  //Event loop
  unsigned int nEntries = myEventSource->numberOfEntries();
  if(maxNevents>0) nEntries = std::min( (unsigned int)nEntries, (unsigned int)maxNevents );
  //nEntries = 5; //TEST
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);    
    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();    
    myTkBuilder.setGeometry(myEventSource->getGeometry());
    myTkBuilder.setPressure(pressure); ////// HACK - workaround for static variable dEdxFitter::currentPressure
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

    ////////////////////////////////////////////
    //
    // make fit DEBUG plots
    if(debugMode) {
      myHistoManager.setEvent(myEventSource->getCurrentEvent());
      gStyle->SetOptStat(0);

      // plot clustered data and dE/dx fit
      myHistoManager.drawDevelHistos(outputCanvas); // pads 1-4

      // plot RecHits per UVW direction
      int ipad=5;
      for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
	TVirtualPad *aPad=outputCanvas->cd(ipad+iDir); // pads 5-7
	if(aPad) {
	  aPad->SetFrameFillColor(kAzure-6);
	  myHistoManager.getRecHitStripVsTime(iDir)->DrawCopy("colz");
	}
      }

      // plot Hough accumulators per UVW direction
      ipad=9;
      for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
	TVirtualPad *aPad=outputCanvas->cd(ipad+iDir); // pads 9-11
	if(aPad) {
	  aPad->SetFrameFillColor(kAzure-6);
	  myHistoManager.getHoughAccumulator(iDir,0).DrawCopy("colz");
	}
      }

      outputCanvas->SetName(Form("c_run%ld_evt%ld", (unsigned long)myEventInfo->GetRunId(), (unsigned long)myEventInfo->GetEventId()));
      outputCanvas->SetTitle(outputCanvas->GetName());
      const std::vector<std::pair<double, double> > marginsLeftRight{ {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15}, {0.15, 0.15},
								      {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15}, {0.5, 0.5},
								      {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15} };
      ipad=0;
      for(auto &it : marginsLeftRight) {
	ipad++;
	TVirtualPad *aPad=outputCanvas->cd(ipad);
	if(aPad) {
	  aPad->SetLeftMargin(it.first);
	  aPad->SetRightMargin(it.second);
	  aPad->Modified();
	  aPad->Update();
	}
      }
      outputCanvas->Modified();
      outputCanvas->Update();
      outputCanvasROOTFile->cd();
      outputCanvas->Write();
    }
    //
    ////////////////////////////////////////////

    const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

    myRecoOutput.setRecTrack(aTrack3D);
    myRecoOutput.setEventInfo(myEventInfo);				   
    myRecoOutput.update(); 

    double length = aTrack3D.getLength();
    double charge = aTrack3D.getIntegratedCharge(length);
    double chi2 = aTrack3D.getChi2();
    double hypothesisChi2 = aTrack3D.getHypothesisFitChi2();
    const TVector3 & vertex = aTrack3D.getSegments().front().getStart();
    const TVector3 & alphaEnd = aTrack3D.getSegments().front().getEnd();
    const TVector3 & carbonEnd = aTrack3D.getSegments().back().getEnd();

    double cosPhiSegments = (alphaEnd-vertex).Unit().Dot((carbonEnd-vertex).Unit());
    
    const TVector3 & tangent = aTrack3D.getSegments().front().getTangent();
    double phi = atan2(-tangent.Z(), tangent.Y());
    double cosTheta = -tangent.X();

    TVector3 horizontal(0,-1,0);
    double horizontalTrackLostPart = 73.4/std::abs(horizontal.Dot(tangent));

    TVector3 vertical(0,0,-1);
    double verticalTrackLostPart = 6.0/std::abs(vertical.Dot(tangent));

    int eventType = aTrack3D.getSegments().front().getPID()+aTrack3D.getSegments().back().getPID();
    double alphaRange =  aTrack3D.getSegments().front().getLength();
    double carbonRange =  aTrack3D.getSegments().back().getPID()== pid_type::CARBON_12 ? aTrack3D.getSegments().back().getLength(): 0.0;
    double alphaEnergy = alphaRange>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA,alphaRange):0.0;
    double carbonEnergy = carbonRange>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, carbonRange):0.0;
    double m_Alpha = myRangeCalculator.getIonMassMeV(pid_type::ALPHA);
    double m_12C = myRangeCalculator.getIonMassMeV(pid_type::CARBON_12);
    
    double p_alpha = sqrt(2*m_Alpha*alphaEnergy);
    double p_12C = sqrt(2*m_12C*carbonEnergy);
    TVector3 total_p = p_alpha*(alphaEnd-vertex).Unit() + p_12C*(carbonEnd-vertex).Unit();
    
    track_data.frameId = iEntry;
    track_data.eventId = myEventInfo->GetEventId();
    track_data.eventType = eventType;
    track_data.length = length;    
    track_data.horizontalLostLength = horizontalTrackLostPart;
    track_data.verticalLostLength = verticalTrackLostPart;
    track_data.charge = charge;
    track_data.cosTheta = cosTheta;
    track_data.phi = phi;
    track_data.chi2 = chi2;
    track_data.hypothesisChi2 = hypothesisChi2;
    
    track_data.xVtx = vertex.X();
    track_data.yVtx = vertex.Y();
    track_data.zVtx = vertex.Z();
    
    track_data.xAlphaEnd = alphaEnd.X();
    track_data.yAlphaEnd = alphaEnd.Y();
    track_data.zAlphaEnd = alphaEnd.Z();
    
    track_data.xCarbonEnd = carbonEnd.X();
    track_data.yCarbonEnd = carbonEnd.Y();
    track_data.zCarbonEnd = carbonEnd.Z();

    track_data.alphaEnergy = alphaEnergy;
    track_data.carbonEnergy = carbonEnergy;
    track_data.alphaRange = alphaRange;
    track_data.carbonRange = carbonRange;
    track_data.cosPhiSegments = cosPhiSegments;

    track_data.total_mom_x = total_p.x();
    track_data.total_mom_y = total_p.y();
    track_data.total_mom_z = total_p.z();

    track_data.lineFitChi2 = aTrack3D.getChi2();
    track_data.dEdxFitChi2 = aTrack3D.getHypothesisFitChi2();
    track_data.dEdxFitSigma = myTkBuilder.getdEdxFitSigmaSmearing();
    
    tree->Fill();    
  }
  outputROOTFile.Write();

  ////////////////////////////////////////////
  //
  // close file with fit DEBUG plots
  if(debugMode) {
    outputCanvasROOTFile->Close();
    delete outputCanvasROOTFile;
    delete outputCanvas;
  }
  //
  ////////////////////////////////////////////

  return nEntries;
}
/////////////////////////////
////////////////////////////

