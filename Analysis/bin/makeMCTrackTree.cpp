#include <cstdlib>
#include <iostream>
#include <vector>

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
int makeTrackTree(boost::property_tree::ptree & aConfig);
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  std::string geometryFileName, dataFileName;
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
    alphaRangeGen,
    alphaEnergyGen,
    chargeGen,
    cosThetaGen, phiGen,
    ///
    eventTypeReco,
    alphaRangeReco,
    alphaEnergyReco,
    carbonRangeReco,
    carbonEnergyReco,
    chargeReco,
    cosThetaReco, phiReco,
    lineFitChi2, dEdxFitChi2;
    } TrackData;
/////////////////////////
int makeTrackTree(boost::property_tree::ptree & aConfig) {
		  
	std::shared_ptr<EventSourceBase> eventSource = EventSourceFactory::makeEventSourceObject(aConfig);
	auto myEventSource = std::dynamic_pointer_cast<EventSourceMC>(eventSource);

  std::string dataFileName = aConfig.get("dataFileName","");
  std::string rootFileName = createROOTFileName(dataFileName);
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  std::string leafNames = "";
  leafNames += "eventId:frameId:";
  leafNames += "eventTypeGen:alphaRangeGen:alphaEnergyGen:chargeGen:cosThetaGen:phiGen:";  
  leafNames += "eventTypeReco:alphaRangeReco:alphaEnergyReco:carbonRangeReco:carbonEnergyReco:";
  leafNames += "chargeReco:cosThetaReco:phiReco:";
  leafNames += "lineFitChi2:dEdxFitChi2";
  tree->Branch("track",&track_data,leafNames.c_str());

  std::string geometryFileName = aConfig.get("dataFileName","");
  int index = geometryFileName.find("mbar");
  double pressure = stof(geometryFileName.substr(index-3, 3));
  double temperature = 293.15;
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
  nEntries = 10000; //TEST
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);    
    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();    
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

    int eventId = myEventSource->getCurrentEvent()->GetEventInfo().GetEventId();
    const Track3D & aTrack3DGen = myEventSource->getGeneratedTrack();
    const Track3D & aTrack3DReco = myTkBuilder.getTrack3D(0);

    track_data.frameId = iEntry;
    track_data.eventId = eventId;

    track_data.eventTypeGen = myEventSource->getGeneratedEventType(); 
    track_data.alphaRangeGen =  aTrack3DGen.getSegments().front().getLength();    
    track_data.alphaEnergyGen = track_data.alphaRangeGen>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeGen):0.0;
    track_data.chargeGen = track_data.alphaEnergyGen;//aTrack3DGen.getIntegratedCharge(track_data.alphaRangeGen);
    const TVector3 & tangentGen = aTrack3DGen.getSegments().front().getTangent();
    track_data.cosThetaGen = -tangentGen.X();
    track_data.phiGen = atan2(-tangentGen.Z(), tangentGen.Y());

    track_data.eventTypeReco = aTrack3DReco.getSegments().front().getPID() + aTrack3DReco.getSegments().back().getPID();    
    track_data.alphaRangeReco =  aTrack3DReco.getSegments().front().getLength();    
    track_data.alphaEnergyReco = track_data.alphaRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeReco):0.0;

    track_data.carbonRangeReco =  aTrack3DReco.getSegments().size()==2 ? aTrack3DReco.getSegments().back().getLength(): 0.0;    
    track_data.carbonEnergyReco = track_data.carbonRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, track_data.carbonRangeReco):0.0;

    track_data.chargeReco = aTrack3DReco.getIntegratedCharge(aTrack3DReco.getLength());
    
    const TVector3 & tangentReco = aTrack3DReco.getSegments().front().getTangent();
    track_data.cosThetaReco = -tangentReco.X();
    track_data.phiReco = atan2(-tangentReco.Z(), tangentReco.Y());
    track_data.lineFitChi2 = aTrack3DReco.getChi2();
    track_data.dEdxFitChi2 = aTrack3DReco.getHypothesisFitChi2();
    
    tree->Fill();    
  }
  outputROOTFile.Write();
  return nEntries;
}
/////////////////////////////
////////////////////////////

