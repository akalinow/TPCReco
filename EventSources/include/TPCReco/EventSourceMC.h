#ifndef _EventSourceMC_H_
#define _EventSourceMC_H_

#include <memory>
#include <boost/property_tree/json_parser.hpp>

#include <TH1F.h>
#include <TRandom3.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/EventSourceBase.h"
#include "TPCReco/UVWprojector.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/RunController.h"

class EventSourceMC: public EventSourceBase {
  
public:

  EventSourceMC(){};

  EventSourceMC(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents);

  ~EventSourceMC();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  reaction_type GetGeneratedReactiontType();

  Track3D getGeneratedTrack();

  std::vector<Track3D> getGeneratedTracks();

  std::shared_ptr<EventTPC> getLastEvent();

  unsigned long int numberOfEvents() const;

  void loadGeometry(const std::string & fileName);

  static inline event_type getSourceType() { return event_type::EventSourceMC; }
  
 private:

  std::shared_ptr<fwk::RunController> myRunController;
  std::shared_ptr<SimEvent> myCurrentSimEvent;
  unsigned long int nEvents;
  void generateNextEvent();
};
#endif

