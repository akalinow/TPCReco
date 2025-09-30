#ifndef _EventSourceBase_H_
#define _EventSourceBase_H_

#include <string>
#include <vector>
#include <memory>
#include <boost/property_tree/ptree.hpp>

#include "TPCReco/EventFilter.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"

#include "TPCReco/EventInfo.h"
#include "TPCReco/PEventTPC.h"

#include "TPCReco/TrackBuilder.h"

enum class EventType {raw, tpc};

class EventSourceBase {

public:

  using EventFilterType = EventFilter<std::function<bool(EventSourceBase&)>>;
  
  EventSourceBase();
  
  virtual ~EventSourceBase();

  bool isFileLoaded() const { return nEvents>0;}

  void loadGeometry(const std::string & fileName);

  virtual void loadDataFile(const std::string & fileName);

  void setNEventsToRead(unsigned long int nEvents) { nEventsToRead = nEvents; }

  void setTrackBuilder(std::shared_ptr<TrackBuilder> aTkBuilderPtr); 

  std::shared_ptr<TrackBuilder> getTrackBuilder() const;

  virtual void loadFileEntry(unsigned long int iEntry) = 0;

  virtual void loadEventId(unsigned long int iEvent) = 0;

  std::string getCurrentPath() const;

  std::shared_ptr<PEventTPC> getCurrentPEvent() const;

  std::shared_ptr<EventTPC> getCurrentEvent() const;
  
  std::shared_ptr<EventTPC> getNextEventLoop();

  std::shared_ptr<EventTPC> getPreviousEventLoop();

  std::shared_ptr<EventTPC> getLastEvent();

  const Track3D & getRecoEvent() const;

  unsigned long int numberOfEvents() const;

  unsigned long int currentEventNumber() const;

  unsigned long int currentEntryNumber() const;

  std::shared_ptr<GeometryTPC> getGeometry() const;
    
  inline EventFilterType& getEventFilter() {return eventFilter;}
  
protected:

  virtual std::shared_ptr<EventTPC> getNextEvent() = 0;
  virtual std::shared_ptr<EventTPC> getPreviousEvent() = 0;

  void fillEventTPC();

  std::string currentFilePath;

  bool recoEventUpdated{false};
  
  unsigned long int nEventsToRead{0};
  unsigned long int nEvents{0};
  unsigned long int myCurrentEntry{0};

  EventFilterType eventFilter;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  eventraw::EventInfo myCurrentEventInfo;
  std::shared_ptr<PEventTPC> myCurrentPEvent;
  std::shared_ptr<EventTPC> myCurrentEvent;
  std::shared_ptr<TrackBuilder> myTkBuilderPtr;
  
};
#endif

