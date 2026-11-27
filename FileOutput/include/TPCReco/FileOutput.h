
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

  // Latest TPCData entry already written for the event (run id, event id) of aKey.
  // Fills aInfo and aRecoEvent and returns true when found. Reads only the EventInfo and
  // RecoEvent branches, so it can be used while the file is being written.
  bool findRecoEvent(const eventraw::EventInfo & aKey,
                     eventraw::EventInfo & aInfo, Track3D & aRecoEvent);

  const std::string & getFileName() const { return myFileName; }

  // Rewrites the closed file fileName so that TPCData keeps only the last entry of each event
  // (run id, event id), in the order of these entries: an event saved again (e.g. from the GUI)
  // replaces the earlier one. Other objects are copied unchanged. Returns the number of removed entries.
  static long keepLastEntryPerEvent(const std::string & fileName);

private:
  
  void close();
    
  std::string myFileName;
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

