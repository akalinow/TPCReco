#ifndef _DataManager_H_
#define _DataManager_H_

#include <string>
#include <vector>
#include <memory>

#include "TFile.h"
#include "TTree.h"
#include "GeometryTPC.h"

class GeometryTPC;
class EventTPC;

class DataManager {
public:
  
    DataManager() = default;
  
  ~DataManager() = default;

  void loadDataFile(const std::string & fileName);

  void loadTreeEntry(unsigned int iEntry);

  void loadEventId(unsigned int iEvent);

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  void loadNextEvent();

  void loadPreviousEvent();

  unsigned int numberOfEvents() const;

  unsigned int currentEventNumber() const;
    
private:
  
  TFile myFile;
  TTree *myTree = nullptr;

  unsigned int nEvents = 0;
  unsigned int myCurrentEntry;

  std::shared_ptr<EventTPC> currentEvent_external_copy;
  EventTPC* currentEvent_internal = nullptr;
};
#endif

