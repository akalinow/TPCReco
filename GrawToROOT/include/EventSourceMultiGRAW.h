#ifndef _EventSourceMultiGRAW_H_
#define _EventSourceMultiGRAW_H_

#include <map>
#include <set>

#include "get/TGrawFile.h"

#include "Graw2DataFrame.h"

#include "EventSourceBase.h"
#include "EventSourceGRAW.h"
#include "EventRaw.h"
#include "PedestalCalculatorGRAW.h"
#include <boost/property_tree/json_parser.hpp>

class EventSourceMultiGRAW: public EventSourceGRAW {

public:

  EventSourceMultiGRAW(){};
  
  EventSourceMultiGRAW(const std::string & geometryFileName);
  
  ~EventSourceMultiGRAW();

  //  void setRemovePedestal(bool aFlag);

  //  void configurePedestal(const boost::property_tree::ptree &config);

  std::shared_ptr<EventTPC> getNextEvent(); // OVERLOADED
  
  std::shared_ptr<EventTPC> getPreviousEvent(); // OVERLOADED

  //  std::shared_ptr<eventraw::EventRaw> getCurrentEventRaw() { return myCurrentEventRaw; }

  //  virtual unsigned long int numberOfEvents() const { return nEntries/GRAW_EVENT_FRAGMENTS;} /// BEWARE: THIS METHOD IS WRONG !!!

  void loadDataFile(const std::string & commaSeparatedFileNames); // OVERLOADED to accept list of comma separated files (one file per ASAD)

  void loadDataFileList(const std::set<std::string> & fileNameList); // NEW

  void loadFileEntry(unsigned long int iEntry); // OVERLOADED

  void loadEventId(unsigned long int eventIdx); // OVERLOADED
  
  //  inline void setFrameLoadRange(int range) {frameLoadRange=range;}

  //  inline void setFillEventType(EventType type) {fillEventType=type;}

  unsigned int getMaxNumberOfStreams() { return GRAW_EVENT_FRAGMENTS; }
  
private:

  bool loadGrawFrame(unsigned int iEntry, bool readFullEvent, unsigned int streamIndex); // OVERLOADED
  //  void findEventFragments(unsigned long int eventIdx, unsigned int iInitialEntry);
  void collectEventFragments(unsigned int eventIdx); // OVERLOADED
  //  void fillEventFromFrame(GET::GDataFrame & aGrawFrame);
  //  void fillEventRawFromFrame(GET::GDataFrame & aGrawFrame);
  void checkEntryForFragments(unsigned int iEntry, unsigned int streamIndex); // OVERLOADED
  //  void findStartingIndex(unsigned long int size);
  std::string getNextFilePath(unsigned int streamIndex); // OVERLOADED

  //  unsigned int GRAW_EVENT_FRAGMENTS;
  //  PedestalCalculatorGRAW myPedestalCalculator;
  //  Graw2DataFrame> myFrameLoader;
  //  std::vector<GET::GDataFrame> myDataFrameList; // NEW [streamIndex] // HOTFIX!!!!! => do not use at all
  //  std::shared_ptr<eventraw::EventRaw> myCurrentEventRaw{std::make_shared<eventraw::EventRaw>()};
  std::vector<std::shared_ptr<TGrawFile> > myFileList; // NEW [streamIndex]
  std::vector<std::string> myFilePathList, myNextFilePathList; // NEW [streamIndex]
  std::vector<std::map<unsigned int, unsigned int> > myFramesMapList; // NEW [streamIndex, eventId, iEntry]
  std::vector<std::map<unsigned int, unsigned int> > myAsadMapList; // NEW [streamIndex, eventId, AsadId]
  std::vector<std::map<unsigned int, unsigned int> > myCoboMapList; // NEW [streamIndex, eventId, CoboId]
  // std::map<unsigned int, std::set<unsigned int> > myASADMap;
  // unsigned int myEntries;
  std::vector<std::set<unsigned int> > myReadEntriesSetList; // NEW [streamIndex, {iEntry, iEntry, ...}]
  // bool isFullFileScannedList;

 // int minSignalCell;
 // int maxSignalCell;
  //  bool removePedestal{true};
  //  unsigned long int startingEventIndex{0};
  unsigned int frameLoadRange{1}; // OVERLOADED
  //  EventType fillEventType{tpc};

};
#endif

