#include <cstdlib>
#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TString.h"
#include "TStopwatch.h"

#include <boost/program_options.hpp>

#include "IonRangeCalculator.h"
#include "dEdxFitter.h"
#include "TrackBuilder.h"
#include "EventSourceROOT.h"
#ifdef WITH_GET
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#endif
#include "SigClusterTPC.h"
#include "EventTPC.h"
#include "RecoOutput.h"
#include "RunIdParser.h"
#include "InputFileHelper.h"
#include "MakeUniqueName.h"
#include "colorText.h"


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
		  const  std::string & dataFileName);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>()->required(), "string - path to the geometry file.")
    ("dataFile",  boost::program_options::value<std::string>()->required(), "string - path to data file.");
  
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

  int nEntriesProcessed = 0;
  if(dataFileName.size() && geometryFileName.size()){
    nEntriesProcessed = makeTrackTree(geometryFileName, dataFileName);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" dataFile: "<<dataFileName
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
    charge, cosTheta, phi, chi2,
    hypothesisChi2,
    xVtx, yVtx, zVtx,
    xAlphaEnd, yAlphaEnd, zAlphaEnd,
    xCarbonEnd, yCarbonEnd, zCarbonEnd;
    } TrackData;
/////////////////////////
int makeTrackTree(const  std::string & geometryFileName,
		  const  std::string & dataFileName) {

  std::shared_ptr<EventSourceBase> myEventSource;
  if(dataFileName.find(".graw")!=std::string::npos){
    #ifdef WITH_GET
    if(dataFileName.find(",")!=std::string::npos){
      myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
    }
    else{
      myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
      dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(160);
    }
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
  leafNames += "charge:cosTheta:phi:chi2:hypothesisChi2:";
  leafNames += "xVtx:yVtx:zVtx:";
  leafNames += "xAlphaEnd:yAlphaEnd:zAlphaEnd:";
  leafNames += "xCarbonEnd:yCarbonEnd:zCarbonEnd";
  tree->Branch("track",&track_data,leafNames.c_str());

  int index = geometryFileName.find("mbar");
  double pressure = stof(geometryFileName.substr(index-3, 3));
  TrackBuilder myTkBuilder;
  myTkBuilder.setGeometry(myEventSource->getGeometry());
  myTkBuilder.setPressure(pressure);
  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,pressure,293.15);

  RecoOutput myRecoOutput;
  std::string fileName = InputFileHelper::tokenize(dataFileName)[0];
  std::size_t last_dot_position = fileName.find_last_of(".");
  std::size_t last_slash_position = fileName.find_last_of("//");
  std::string recoFileName = MakeUniqueName("Reco_"+fileName.substr(last_slash_position+1,
						     last_dot_position-last_slash_position-1)+".root");
  std::shared_ptr<eventraw::EventInfo> myEventInfo = std::make_shared<eventraw::EventInfo>();
  RunIdParser runParser(fileName);
  myEventInfo->SetRunId(runParser.runId());
  myRecoOutput.setEventInfo(myEventInfo);
  myRecoOutput.open(recoFileName);
  
  myEventSource->loadDataFile(dataFileName);
  std::cout<<KBLU<<"File with "<<RST<<myEventSource->numberOfEntries()<<" frames loaded."<<std::endl;

  //Event loop
  unsigned int nEntries = myEventSource->numberOfEntries();
  //nEntries = 500;
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);
    /*
    myEventSource->getCurrentEvent()->MakeOneCluster(35, 0, 0);
    if(myEventSource->getCurrentEvent()->GetOneCluster().GetNhits()>20000){
      std::cout<<KRED<<"Noisy event - skipping."<<RST<<std::endl;
      continue;
      }
    */
    
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

    int eventId = myEventSource->getCurrentEvent()->GetEventInfo().GetEventId();
    const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

    myRecoOutput.setRecTrack(aTrack3D);
    myEventInfo->SetEventId(eventId);
    myRecoOutput.setEventInfo(myEventInfo);				   
    myRecoOutput.update(); 
    
    double length = aTrack3D.getLength();
    double charge = aTrack3D.getIntegratedCharge(length);
    double chi2 = aTrack3D.getChi2();
    double hypothesisChi2 = aTrack3D.getHypothesisFitChi2();
    const TVector3 & vertex = aTrack3D.getSegments().front().getStart();
    const TVector3 & alphaEnd = aTrack3D.getSegments().front().getEnd();
    const TVector3 & carbonEnd = aTrack3D.getSegments().back().getEnd();
    
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
    tree->Fill();    
  }
  outputROOTFile.Write();
  myRecoOutput.close();
  return nEntries;
}
/////////////////////////////
////////////////////////////

