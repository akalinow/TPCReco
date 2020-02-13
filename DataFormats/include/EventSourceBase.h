#ifndef _EventSourceBase_H_
#define _EventSourceBase_H_

#include <string>
#include <vector>
#include <memory>

#include "EventTPC.h"
#include "GeometryTPC.h"

class EventSourceBase {
public:
  
  EventSourceBase();
  
  virtual ~EventSourceBase();

  void loadGeometry(const std::string & fileName);

  virtual void loadDataFile(const std::string & fileName) = 0;

  virtual void loadFileEntry(unsigned long int iEntry) = 0;

  void loadEventId(unsigned long int iEvent);

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  unsigned long int numberOfEvents() const;

  unsigned long int currentEventNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
protected:
  
  unsigned long int nEvents;
  unsigned long int myCurrentEntry;

  std::shared_ptr<EventTPC> myCurrentEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

