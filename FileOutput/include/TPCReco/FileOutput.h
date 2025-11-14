
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
    
private:
  
  void close();
    
  std::shared_ptr<TFile> myOutputFilePtr;
  std::map<std::string, std::shared_ptr<TTree>> myOutputTrees;

  std::shared_ptr<Track3D> mySimEventPtr;
  std::shared_ptr<Track3D> myRecoEventPtr;
  std::shared_ptr<PEventTPC> myPEventPtr;
  std::shared_ptr<eventraw::EventInfo> myEventInfoPtr;

  std::vector<std::string> offBranches;
  std::vector<std::string> onBranches;
  
};
#endif

