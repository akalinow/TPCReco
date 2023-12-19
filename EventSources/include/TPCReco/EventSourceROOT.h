#ifndef _EventSourceROOT_H_
#define _EventSourceROOT_H_

#include "TPCReco/EventSourceBase.h"
#include "TPCReco/EventRaw.h"
#include "TPCReco/PedestalCalculator.h"
#include <boost/property_tree/json_parser.hpp>

class TFile;
class TTree;

class EventSourceROOT: public EventSourceBase {
  
public:

  EventSourceROOT(){};

  EventSourceROOT(const std::string & geometryFileName);

  ~EventSourceROOT();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<EventTPC> getLastEvent();

  unsigned long int numberOfEvents() const;

  void setRemovePedestal(bool aFlag);

  void configurePedestal(const boost::property_tree::ptree &config);

  void loadGeometry(const std::string & fileName);
  
 private:

  PEventTPC *aPtr; // for TBranch
  eventraw::EventInfo *aPtrEventInfo; // for TBranch
  eventraw::EventData *aPtrEventData; // for TBranch
  std::shared_ptr<eventraw::EventRaw> myCurrentEventRaw{std::make_shared<eventraw::EventRaw>()};

  std::string treeName;
  std::shared_ptr<TFile> myFile;
  std::shared_ptr<TTree> myTree;
  bool removePedestal{true};

  PedestalCalculator myPedestalCalculator;  
  void setTreePointers();

};
#endif

