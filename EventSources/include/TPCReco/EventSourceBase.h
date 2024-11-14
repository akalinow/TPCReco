#ifndef _EventSourceBase_H_
#define _EventSourceBase_H_

#include <string>
#include <vector>
#include <memory>
#include <boost/property_tree/ptree.hpp>

#include "TPCReco/EventFilter.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/GeometryTPC.h"

#include "TPCReco/EventInfo.h"
#include "TPCReco/PEventTPC.h"

enum class EventType {raw, tpc};

class EventSourceBase {

public:
  using EventFilterType = EventFilter<std::function<bool(EventTPC&)>>;
  
  EventSourceBase();
  
  virtual ~EventSourceBase();

  bool isFileLoaded() const { return nEntries>0;}

  void loadGeometry(const std::string & fileName);

  virtual void loadDataFile(const std::string & fileName);

  virtual void loadFileEntry(unsigned long int iEntry) = 0;

  virtual void loadEventId(unsigned long int iEvent) = 0;

  std::string getCurrentPath() const;

  std::shared_ptr<PEventTPC> getCurrentPEvent() const;

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  std::shared_ptr<EventTPC> getNextEventLoop();

  std::shared_ptr<EventTPC> getPreviousEventLoop();

  std::shared_ptr<EventTPC> getLastEvent();

  virtual unsigned long int numberOfEvents() const = 0;

  unsigned long int numberOfEntries() const;

  unsigned long int currentEventNumber() const;

  unsigned long int currentEntryNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
  inline EventFilterType& getEventFilter() {return eventFilter;}

  virtual std::shared_ptr<EventTPC> getNextEvent() = 0;
  
  virtual std::shared_ptr<EventTPC> getPreviousEvent() = 0;
  
protected:

  void fillEventTPC();

  std::string currentFilePath;
  
  unsigned long int nEntries{0};
  unsigned long int myCurrentEntry{0};
  EventFilterType eventFilter;

  std::shared_ptr<GeometryTPC> myGeometryPtr;
  eventraw::EventInfo myCurrentEventInfo;
  std::shared_ptr<PEventTPC> myCurrentPEvent;
  std::shared_ptr<EventTPC> myCurrentEvent;
  
};
#endif

