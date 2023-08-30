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
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
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
    lineFitChi2, dEdxFitChi2;
    } TrackData;
/////////////////////////
int makeTrackTree(boost::property_tree::ptree & aConfig) {
		  
  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(aConfig);
  
  std::string dataFileName = aConfig.get("dataFileName","");
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
  leafNames += "lineFitChi2:dEdxFitChi2";
  tree->Branch("track",&track_data,leafNames.c_str());
  
  std::string geometryFileName = aConfig.get("geometryFileName","");
  int index = geometryFileName.find("mbar");
  double pressure = stof(geometryFileName.substr(index-3, 3));
  //double pressure = aConfig.get<bool>("conditions.pressure"); this needs python scripts reorganisation
  double temperature = aConfig.get<bool>("conditions.temperature");
    
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
  //nEntries = 5; //TEST
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);    
    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();    
    myTkBuilder.setEvent(myEventSource->getCurrentEvent());
    myTkBuilder.reconstruct();

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
    
    tree->Fill();    
  }
  outputROOTFile.Write();
  return nEntries;
}
/////////////////////////////
////////////////////////////

