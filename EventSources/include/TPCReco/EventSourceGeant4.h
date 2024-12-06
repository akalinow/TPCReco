#ifndef _EventSourceGeant4_H_
#define _EventSourceGeant4_H_

#include <memory>
#include <boost/property_tree/json_parser.hpp>

#include <TH1F.h>
#include <TRandom3.h>

#include "TPCReco/EventSourceBase.h"
#include "TPCReco/UVWprojector.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/RunController.h"

class EventSourceGeant4: public EventSourceBase {
  
public:

  EventSourceGeant4(){};

  EventSourceGeant4(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents);

  ~EventSourceGeant4();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<EventTPC> getLastEvent();

  unsigned long int numberOfEvents() const;

  void loadGeometry(const std::string & fileName);
  
 private:

  std::shared_ptr<fwk::RunController> myRunController;
  unsigned long int nEvents;
  void fillEvent(ModuleExchangeSpace &event);
  std::shared_ptr<ModuleExchangeSpace> current_event;
  void generateNextEvent();
};
#endif

