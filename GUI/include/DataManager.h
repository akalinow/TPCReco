#ifndef _DataManager_H_
#define _DataManager_H_

#include <string>
#include <vector>
#include <memory>

#include "TFile.h"
#include "TTree.h"

class GeometryTPC;
class EventTPC;

class DataManager {
public:
  
  DataManager() = default;
  
  ~DataManager() = default;

  bool loadGeometry(const std::string & fileName);

  void loadDataFile(const std::string & fileName);

  void loadTreeEntry(unsigned int iEntry);

  void loadEventId(unsigned int iEvent);

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  unsigned int numberOfEvents() const;

  unsigned int currentEventNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
private:
  
  std::shared_ptr<TFile> myFile;
  TTree *myTree = nullptr;

  unsigned int nEvents = 0;
  unsigned int myCurrentEntry;

  std::shared_ptr<EventTPC> currentEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

