#ifndef _EventSourceGRAW_H_
#define _EventSourceGRAW_H_

#include <map>
#include <set>

#include "get/TGrawFile.h"

#include "Graw2DataFrame.h"

#include "EventSourceBase.h"
#include "EventRaw.h"
#include "PedestalCalculatorGRAW.h"
#include <boost/property_tree/json_parser.hpp>

class EventSourceGRAW: public EventSourceBase {

public:

  EventSourceGRAW(){};
  
  EventSourceGRAW(const std::string & geometryFileName);
  
  ~EventSourceGRAW();

  void setRemovePedestal(bool aFlag);

  void configurePedestal(const boost::property_tree::ptree &config);

  std::shared_ptr<EventTPC> getNextEvent();
  
  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<eventraw::EventRaw> getCurrentEventRaw() { return myCurrentEventRaw; }

  virtual unsigned long int numberOfEvents() const { return nEntries/GRAW_EVENT_FRAGMENTS;} /// BEWARE: THIS METHOD IS WRONG !!!

  void loadDataFile(const std::string & fileName);

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int eventIdx);
  
  inline void setFrameLoadRange(int range) {frameLoadRange=range;}

  inline void setFillEventType(int type) {fillEventType=(type==0 ? 0 : 1);}
  
private:

  bool loadGrawFrame(unsigned int iEntry, bool readFullEvent);
  void findEventFragments(unsigned long int eventIdx, unsigned int iInitialEntry);
  void collectEventFragments(unsigned int eventIdx);
  void fillEventFromFrame(GET::GDataFrame & aGrawFrame);
  void fillEventRawFromFrame(GET::GDataFrame & aGrawFrame);
  void checkEntryForFragments(unsigned int iEntry);
  void findStartingIndex(unsigned long int size);
  std::string getNextFilePath();

  unsigned int GRAW_EVENT_FRAGMENTS;
  PedestalCalculatorGRAW myPedestalCalculator;
  Graw2DataFrame myFrameLoader;
  GET::GDataFrame myDataFrame;
  std::shared_ptr<eventraw::EventRaw> myCurrentEventRaw{std::make_shared<eventraw::EventRaw>()};
  std::shared_ptr<TGrawFile> myFile;
  std::string myFilePath, myNextFilePath;
  std::map<unsigned int, std::set<unsigned int> > myFramesMap;
  std::map<unsigned int, std::set<unsigned int> > myASADMap;
  std::set<unsigned int> myReadEntriesSet;
  bool isFullFileScanned{false};

 // int minSignalCell;
 // int maxSignalCell;
  bool removePedestal{true};
  unsigned long int startingEventIndex{0};
  unsigned int frameLoadRange{100};
  unsigned int fillEventType{0}; // EventTPC=0(default), EventRaw=1

};
#endif

