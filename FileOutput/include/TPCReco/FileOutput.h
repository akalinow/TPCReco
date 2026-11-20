
#ifndef _FileOutput_H_
#define _FileOutput_H_

#include <string>
#include <vector>
#include <memory>

#include "boost/property_tree/ptree.hpp"
#include "TPCReco/EventInfo.h"

class TTree;
class TFile;
class SimEvent;
class EventTPC;
class PEventTPC;
class Track3D;
class EventSourceBase;
class MLTree;

class FileOutput {
public:
  
  FileOutput(boost::property_tree::ptree & aConfig);

  FileOutput();
  
  ~FileOutput();

  void init(boost::property_tree::ptree aConfig);

  void setRecoEvent(const Track3D & aRecTrack);

  void update(std::shared_ptr<EventSourceBase> aEventSource);

  void update(const eventraw::EventInfo & aEventInfoPtr,
              const Track3D & aRecoEventPtr);

  // Fill TPCData with the current event of aEventSource, but with the given
  // EventInfo (e.g. carrying GUI event type bits) and reconstructed track
  // (e.g. from manual GUI reconstruction). No event selection is applied.
  void update(std::shared_ptr<EventSourceBase> aEventSource,
              const eventraw::EventInfo & aEventInfo,
              const Track3D & aRecoEvent);

private:
  
  void close();
    
  std::shared_ptr<TFile> myOutputFilePtr;
  std::map<std::string, std::shared_ptr<TTree>> myOutputTrees;

  std::shared_ptr<Track3D> mySimEventPtr;
  std::shared_ptr<Track3D> myRecoEventPtr;
  std::shared_ptr<PEventTPC> myPEventPtr;
  std::shared_ptr<eventraw::EventInfo> myEventInfoPtr;

  // flat tree for ML (output.mlTree), filled together with TPCData
  std::shared_ptr<MLTree> myMLTree;

  std::vector<std::string> offBranches;
  std::vector<std::string> onBranches;
  
};
#endif

