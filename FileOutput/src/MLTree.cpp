#include <stdexcept>

#include <TTree.h>

#include "TPCReco/EventTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/MLTree.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MLTree::MLTree(const boost::property_tree::ptree &aConfig) : myConfig(aConfig) {

  const std::string filterName = aConfig.get<std::string>("output.mlHitFilter", "threshold");
  if(filterName=="none") myHitFilter = filter_type::none;
  else if(filterName=="threshold") myHitFilter = filter_type::threshold;
  else if(filterName=="fraction") myHitFilter = filter_type::fraction;
  else throw std::invalid_argument("output.mlHitFilter: unknown hit filter \""+filterName+
                                   "\". Allowed: none, threshold, fraction.");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MLTree::Segments::clear(){

  pid.clear();
  startX.clear(); startY.clear(); startZ.clear();
  endX.clear(); endY.clear(); endZ.clear();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MLTree::Segments::set(const Track3D &aTrack){

  clear();
  for(const auto &aSegment: aTrack.getSegments()){
    pid.push_back(aSegment.getPID());
    startX.push_back(aSegment.getStart().X());
    startY.push_back(aSegment.getStart().Y());
    startZ.push_back(aSegment.getStart().Z());
    endX.push_back(aSegment.getEnd().X());
    endY.push_back(aSegment.getEnd().Y());
    endZ.push_back(aSegment.getEnd().Z());
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MLTree::Segments::book(TTree *aTree, const std::string &prefix){

  aTree->Branch((prefix+"pid").c_str(), &pid);
  aTree->Branch((prefix+"startX").c_str(), &startX);
  aTree->Branch((prefix+"startY").c_str(), &startY);
  aTree->Branch((prefix+"startZ").c_str(), &startZ);
  aTree->Branch((prefix+"endX").c_str(), &endX);
  aTree->Branch((prefix+"endY").c_str(), &endY);
  aTree->Branch((prefix+"endZ").c_str(), &endZ);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MLTree::book(TTree *aTree){

  aTree->Branch("runId", &runId);
  aTree->Branch("eventId", &eventId);
  aTree->Branch("timestamp", &timestamp);
  aTree->Branch("hitDir", &hitDir);
  aTree->Branch("hitStrip", &hitStrip);
  aTree->Branch("hitTime", &hitTime);
  aTree->Branch("hitCharge", &hitCharge);
  sim.book(aTree, "sim_");
  reco.book(aTree, "reco_");
  aTree->Branch("reco_length", &reco_length);
  aTree->Branch("reco_loss", &reco_loss);
  aTree->Branch("reco_hypothesisLoss", &reco_hypothesisLoss);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MLTree::set(EventTPC &aEvent, const Track3D &aRecoTrack, const Track3D *aSimTrack){

  const auto &info = aEvent.GetEventInfo();
  runId = info.GetRunId();
  eventId = info.GetEventId();
  timestamp = info.GetEventTimestamp();

  // the event may carry a filter configuration set by the reconstruction:
  // use the one from the application configuration
  if(myHitFilter!=filter_type::none) aEvent.setHitFilterConfig(myHitFilter, myConfig);

  mergedHits.clear();
  for(const auto &key: aEvent.GetFilteredKeys(myHitFilter)){
    const int dir = std::get<0>(key), section = std::get<1>(key);
    const int strip = std::get<2>(key), timeCell = std::get<3>(key);
    const double value = aEvent.GetValByStrip(dir, section, strip, timeCell);
    if(value==0.0) continue; // envelope cells without a charge entry
    mergedHits[std::make_tuple(dir, strip, timeCell)] += value;
  }

  hitDir.clear(); hitStrip.clear(); hitTime.clear(); hitCharge.clear();
  hitDir.reserve(mergedHits.size());
  hitStrip.reserve(mergedHits.size());
  hitTime.reserve(mergedHits.size());
  hitCharge.reserve(mergedHits.size());
  for(const auto &item: mergedHits){
    hitDir.push_back(std::get<0>(item.first));
    hitStrip.push_back(std::get<1>(item.first));
    hitTime.push_back(std::get<2>(item.first));
    hitCharge.push_back(item.second);
  }

  reco.set(aRecoTrack);
  reco_length = aRecoTrack.getLength();
  reco_loss = aRecoTrack.getLoss();
  reco_hypothesisLoss = aRecoTrack.getHypothesisFitLoss();

  if(aSimTrack) sim.set(*aSimTrack);
  else sim.clear();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
