#ifndef _EventSourceBase_H_
#define _EventSourceBase_H_

#include <string>
#include <vector>
#include <memory>

#include "EventFilter.h"
#include "EventTPC.h"
#include "GeometryTPC.h"

class EventSourceBase {
public:
  
  EventSourceBase();
  
  virtual ~EventSourceBase();

  bool isFileLoaded() const { return nEntries>0;}

  void loadGeometry(const std::string & fileName);

  virtual void loadDataFile(const std::string & fileName);

  virtual void loadFileEntry(unsigned long int iEntry) = 0;

  virtual void loadEventId(unsigned long int iEvent) = 0;

  std::string getCurrentPath() const;

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  std::shared_ptr<EventTPC> getNextEventLoop();

  std::shared_ptr<EventTPC> getPreviousEventLoop();

  std::shared_ptr<EventTPC> getLastEvent();

  virtual unsigned long int numberOfEvents() const = 0;

  unsigned long int numberOfEntries() const;

  unsigned long int currentEventNumber() const;

  unsigned long int currentEntryNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
  inline EventFilter & getEventFilter() {return eventFilter;}
protected:

  virtual std::shared_ptr<EventTPC> getNextEvent() = 0;
  virtual std::shared_ptr<EventTPC> getPreviousEvent() = 0;
  std::string currentFilePath;
  
  unsigned long int nEntries;
  unsigned long int myCurrentEntry;
  EventFilter eventFilter;

  std::shared_ptr<EventTPC> myCurrentEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

