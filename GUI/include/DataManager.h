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
  
  DataManager();
  
  ~DataManager();

  void loadGeometry(const std::string & fileName);

  void loadDataFile(const std::string & fileName);

  void loadEvent(unsigned int iEvent);

  EventTPC* getCurrentEvent() const;
  
  EventTPC* getNextEvent();

  EventTPC* getPreviousEvent();

  unsigned int numberOfEvents() const;

  unsigned int currentEventNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
private:
  
  std::shared_ptr<TFile> myFile;
  TTree *myTree;

  unsigned int nEvents;
  unsigned int myCurrentEntry;

  EventTPC *currentEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

