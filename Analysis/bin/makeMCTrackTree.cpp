#include <cstdlib>
#include <iostream>
#include <vector>
#include <ctime>

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TString.h>
#include <TStopwatch.h>

#include <boost/program_options.hpp>

#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/dEdxFitter.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/EventSourceFactory.h"

#include "TPCReco/ConfigManager.h"
#include "TPCReco/RecoOutput.h"
#include "TPCReco/RunIdParser.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/MakeUniqueName.h"
#include "TPCReco/colorText.h"

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
  else if(rootFileName.find("MC")!=std::string::npos){
    rootFileName = "TrackTree_"+rootFileName;
  }
  else{
    std::cout<<KRED<<"File format unknown: "<<RST<<rootFileName<<std::endl;
    exit(1);
  }
  index = rootFileName.rfind("graw");
  if(index!=std::string::npos){
    rootFileName = rootFileName.replace(index,-1,"root");
  }

  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  std::stringstream ss;
  ss<<std::put_time(&tm, "%d_%m_%Y_%H_%M");
  std::string timestamp = ss.str();

  rootFileName = rootFileName.replace(rootFileName.find(".root"),-1,"_"+timestamp+".root");
  
  return rootFileName;
}
/////////////////////////////////////
/////////////////////////////////////
int makeTrackTree(boost::property_tree::ptree & aConfig);
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
 
  int nEntriesProcessed = makeTrackTree(myConfig);
 
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
    eventTypeGen,
    alphaRangeGen, alphaEnergyGen,
    carbonRangeGen, carbonEnergyGen,
    chargeGen, cosThetaGen, phiGen,
    vtxGenX, vtxGenY, vtxGenZ,
    ///
    eventTypeReco,
    alphaRangeReco,alphaEnergyReco,
    carbonRangeReco, carbonEnergyReco,
    chargeReco, cosThetaReco, phiReco,
    vtxRecoX, vtxRecoY, vtxRecoZ,
    lineFitLoss, dEdxFitLoss, dEdxFitSigma;
    } TrackData;
/////////////////////////
int makeTrackTree(boost::property_tree::ptree & aConfig) {
		  
	std::shared_ptr<EventSourceBase> eventSource = EventSourceFactory::makeEventSourceObject(aConfig);
	auto myEventSource = std::dynamic_pointer_cast<EventSourceMC>(eventSource);
  if(!myEventSource){
    std::cout<<KRED<<"Wrong event source type!"<<RST<<std::endl;
    exit(1);
  }

  std::string dataFileName = aConfig.get("input.dataFile","");
  std::string rootFileName = createROOTFileName(dataFileName);
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  std::string leafNames = "";
  leafNames += "eventId:frameId:";
  leafNames += "eventTypeGen:";
  leafNames += "alphaRangeGen:alphaEnergyGen:";
  leafNames += "carbonRangeGen:carbonEnergyGen:";
  leafNames += "chargeGen:cosThetaGen:phiGen:";
  leafNames += "vtxGenX:vtxGenY:vtxGenZ:";
  leafNames += "eventTypeReco:";
  leafNames += "alphaRangeReco:alphaEnergyReco:";
  leafNames += "carbonRangeReco:carbonEnergyReco:";
  leafNames += "chargeReco:cosThetaReco:phiReco:";
  leafNames += "vtxRecoX:vtxRecoY:vtxRecoZ:";
  leafNames += "lineFitLoss:dEdxFitLoss:dEdxFitSigma";
  tree->Branch("track",&track_data,leafNames.c_str());
  
  std::string geometryFileName = aConfig.get("input.geometryFile","");
  double pressure = aConfig.get<double>("conditions.pressure"); 
  double temperature = aConfig.get<double>("conditions.temperature");
  
  TrackBuilder myTkBuilder;
  myTkBuilder.setGeometry(myEventSource->getGeometry());
  myTkBuilder.setPressure(pressure);
  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,pressure, temperature);

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
  nEntries = 40000; 
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);    
    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();    
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

    int eventId = myEventSource->getCurrentEvent()->GetEventInfo().GetEventId();
    const Track3D & aTrack3DGenAlpha = myEventSource->getGeneratedTrack(0);
    const Track3D & aTrack3DGenCarbon = myEventSource->getGeneratedTrack(1);
    const Track3D & aTrack3DReco = myTkBuilder.getTrack3D(0);

    track_data.frameId = iEntry;
    track_data.eventId = eventId;

    track_data.eventTypeGen = myEventSource->getGeneratedEventType(); 
    track_data.alphaRangeGen =  aTrack3DGenAlpha.getSegments().front().getLength();    
    track_data.alphaEnergyGen = track_data.alphaRangeGen>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeGen):0.0;

    track_data.carbonRangeGen =  aTrack3DGenCarbon.getSegments().front().getLength();
    track_data.carbonEnergyGen = track_data.carbonRangeGen>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, track_data.carbonRangeGen):0.0;

    track_data.chargeGen = (track_data.alphaEnergyGen + track_data.carbonEnergyGen)*1E5;
    const TVector3 & tangentGen = aTrack3DGenAlpha.getSegments().front().getTangent();
    track_data.cosThetaGen = -tangentGen.X();
    track_data.phiGen = atan2(-tangentGen.Z(), tangentGen.Y());

    track_data.cosThetaGen = tangentGen.Z();//TEST
    track_data.phiGen = tangentGen.Phi();//TEST

    const TVector3 & vtxGen = aTrack3DGenAlpha.getSegments().front().getStart();
    track_data.vtxGenX = vtxGen.X();
    track_data.vtxGenY = vtxGen.Y();
    track_data.vtxGenZ = vtxGen.Z();

    track_data.eventTypeReco = aTrack3DReco.getSegments().front().getPID() + aTrack3DReco.getSegments().back().getPID();    
    track_data.alphaRangeReco =  aTrack3DReco.getSegments().front().getLength();    
    track_data.alphaEnergyReco = track_data.alphaRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeReco):0.0;

    track_data.carbonRangeReco =  aTrack3DReco.getSegments().size()==2 ? aTrack3DReco.getSegments().back().getLength(): 0.0;    
    track_data.carbonEnergyReco = track_data.carbonRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, track_data.carbonRangeReco):0.0;

    track_data.chargeReco = aTrack3DReco.getIntegratedCharge(aTrack3DReco.getLength());

    const TVector3 & vtxReco = aTrack3DReco.getSegments().front().getStart();
    track_data.vtxRecoX = vtxReco.X();
    track_data.vtxRecoY = vtxReco.Y();
    track_data.vtxRecoZ = vtxReco.Z();
    
    const TVector3 & tangentReco = aTrack3DReco.getSegments().front().getTangent();
    track_data.cosThetaReco = -tangentReco.X();
    track_data.phiReco = atan2(-tangentReco.Z(), tangentReco.Y());

    track_data.cosThetaReco = cos(tangentReco.Theta());//TEST
    track_data.phiReco = tangentReco.Phi();//TEST


    track_data.lineFitLoss = aTrack3DReco.getLoss();
    track_data.dEdxFitLoss = aTrack3DReco.getHypothesisFitLoss();
    track_data.dEdxFitSigma = aTrack3DReco.getSegments().front().getDiffusion();
    
    tree->Fill();    
  }
  outputROOTFile.Write();
  return nEntries;
}
/////////////////////////////
////////////////////////////

