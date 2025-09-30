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
#include "TPCReco/RunIdParser.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/MakeUniqueName.h"
#include "TPCReco/colorText.h"

#include "TPCReco/EventTPC.h"
#include "TPCReco/CommonDefinitions.h"

float reactionTypeToFloat(reaction_type type) {
    return static_cast<float>(static_cast<std::underlying_type<reaction_type>::type>(type));
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
    eventReactionType,
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
  std::string rootFileName = InputFileHelper::makeOutputFileName(dataFileName,"MCTrackTree");

  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  std::string leafNames = "";
  leafNames += "eventId:";
  leafNames += "eventReactionType:";
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
  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,pressure, temperature);

  std::shared_ptr<eventraw::EventInfo> myEventInfo = std::make_shared<eventraw::EventInfo>();
  std::cout<<KBLU<<"File with "<<RST<<myEventSource->numberOfEvents()<<" frames loaded."<<std::endl;

  //Event loop
  while(myEventSource->getNextEventLoop()){

    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();    
    int eventId = myEventSource->getCurrentEvent()->GetEventInfo().GetEventId();
    const Track3D & genTrack = myEventSource->getGeneratedTrack();

    TrackSegment3D aTrack3DGenAlpha, aTrack3DGenCarbon;
    for(auto iSegment : genTrack.getSegments()) {
      if(iSegment.getPID() == pid_type::ALPHA) {
        aTrack3DGenAlpha = iSegment;
      } else if(iSegment.getPID() == pid_type::CARBON_12) {
        aTrack3DGenCarbon = iSegment;
      }
    }

    const Track3D & aTrack3D = myEventSource->getRecoEvent();

    track_data.eventId = eventId;
    track_data.eventReactionType = reactionTypeToFloat(myEventSource->GetGeneratedReactionType());

    track_data.alphaRangeGen =  aTrack3DGenAlpha.getLength();    
    track_data.alphaEnergyGen = track_data.alphaRangeGen>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeGen):0.0;

    track_data.carbonRangeGen =  aTrack3DGenCarbon.getLength();
    track_data.carbonEnergyGen = track_data.carbonRangeGen>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, track_data.carbonRangeGen):0.0;

    track_data.chargeGen = (track_data.alphaEnergyGen + track_data.carbonEnergyGen)*1E5;
    const TVector3 & tangentGen = aTrack3DGenAlpha.getTangent();
    track_data.cosThetaGen = -tangentGen.X();
    track_data.phiGen = atan2(-tangentGen.Z(), tangentGen.Y());

    ///Use the BEAM coordinates: X - along the beam, Y - horizontal, Z - vertical
    track_data.cosThetaGen = tangentGen.Z();
    track_data.phiGen = tangentGen.Phi();

    const TVector3 & vtxGen = aTrack3DGenAlpha.getStart();
    track_data.vtxGenX = vtxGen.X();
    track_data.vtxGenY = vtxGen.Y();
    track_data.vtxGenZ = vtxGen.Z();

    track_data.eventTypeReco = aTrack3D.getSegments().front().getPID() + aTrack3D.getSegments().back().getPID();    
    track_data.alphaRangeReco =  aTrack3D.getSegments().front().getLength();    
    track_data.alphaEnergyReco = track_data.alphaRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, track_data.alphaRangeReco):0.0;

    track_data.carbonRangeReco =  aTrack3D.getSegments().size()==2 ? aTrack3D.getSegments().back().getLength(): 0.0;    
    track_data.carbonEnergyReco = track_data.carbonRangeReco>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, track_data.carbonRangeReco):0.0;

    track_data.chargeReco = aTrack3D.getIntegratedCharge(aTrack3D.getLength());

    const TVector3 & vtxReco = aTrack3D.getSegments().front().getStart();
    track_data.vtxRecoX = vtxReco.X();
    track_data.vtxRecoY = vtxReco.Y();
    track_data.vtxRecoZ = vtxReco.Z();

    const TVector3 & tangentReco = aTrack3D.getSegments().front().getTangent();
    track_data.cosThetaReco = -tangentReco.X();
    track_data.phiReco = atan2(-tangentReco.Z(), tangentReco.Y());

    ///Use the BEAM coordinates: X - along the beam, Y - horizontal, Z - vertical
    track_data.cosThetaReco = cos(tangentReco.Theta());
    track_data.phiReco = tangentReco.Phi();


    track_data.lineFitLoss = aTrack3D.getLoss();
    track_data.dEdxFitLoss = aTrack3D.getHypothesisFitLoss();
    track_data.dEdxFitSigma = aTrack3D.getSegments().front().getDiffusion();
    
    tree->Fill();    
  }
  return 0;
}
/////////////////////////////
////////////////////////////

