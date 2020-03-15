#ifndef _EventSourceROOT_H_
#define _EventSourceROOT_H_

#include "EventSourceBase.h"

class TFile;
class TTree;

class EventSourceROOT: public EventSourceBase {
public:
  
  EventSourceROOT();
  
  ~EventSourceROOT();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<EventTPC> getLastEvent();

  unsigned long int numberOfEvents() const;

private:

  EventTPC *aPtr;
  std::string treeName;
  std::shared_ptr<TFile> myFile;
  std::shared_ptr<TTree> myTree;
  
};
#endif

