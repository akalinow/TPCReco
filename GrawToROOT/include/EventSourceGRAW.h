#ifndef _EventSourceGRAW_H_
#define _EventSourceGRAW_H_

#include <map>
#include <set>

#include "get/GDataFrame.h"
#include "get/TGrawFile.h"

#include "EventSourceBase.h"
#include "PedestalCalculator.h"

class EventSourceGRAW: public EventSourceBase {

public:

  EventSourceGRAW(){};
  
  EventSourceGRAW(const std::string & geometryFileName);
  
  ~EventSourceGRAW();

  void setRemovePedestal(bool aFlag);

  std::shared_ptr<EventTPC> getNextEvent();
  
  std::shared_ptr<EventTPC> getPreviousEvent();

  virtual unsigned long int numberOfEvents() const { return nEntries/GRAW_EVENT_FRAGMENTS;}

  void loadDataFile(const std::string & fileName);

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int eventIdx);

private:

  bool loadGrawFrame(unsigned int iEntry);
  void findEventFragments(unsigned long int eventIdx, unsigned int iInitialEntry);
  void collectEventFragments(unsigned int eventIdx);
  void fillEventFromFrame(GET::GDataFrame & aGrawFrame);

  unsigned int GRAW_EVENT_FRAGMENTS;
  PedestalCalculator myPedestalCalculator;
  GET::GDataFrame myDataFrame;
  std::shared_ptr<TGrawFile> myFile;
  std::string myFilePath;
  std::map<unsigned int, std::set<unsigned int> > myFramesMap;

  int minSignalCell;
  int maxSignalCell;
  bool removePedestal{true};

};
#endif

