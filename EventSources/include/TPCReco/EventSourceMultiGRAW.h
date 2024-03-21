#ifndef _EventSourceMultiGRAW_H_
#define _EventSourceMultiGRAW_H_

#ifdef WITH_GET

#include <map>
#include <set>

#include <get/TGrawFile.h>

#include "TPCReco/Graw2DataFrame.h"

#include "TPCReco/EventSourceBase.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventRaw.h"
#include "TPCReco/PedestalCalculatorGRAW.h"
#include <boost/property_tree/json_parser.hpp>

class EventSourceMultiGRAW: public EventSourceGRAW {

public:

  EventSourceMultiGRAW(){};
  
  EventSourceMultiGRAW(const std::string & geometryFileName);
  
  ~EventSourceMultiGRAW();

  std::shared_ptr<EventTPC> getNextEvent(); // OVERLOADED
  
  std::shared_ptr<EventTPC> getPreviousEvent(); // OVERLOADED

  unsigned long int numberOfEvents() const { return nEntries; } // the lowest number of frames among all GRAW_EVENT_FRAGMENTS files

  void loadDataFile(const std::string & commaSeparatedFileNames); // OVERLOADED to accept list of comma separated files (one file per ASAD)

  void loadDataFileList(const std::set<std::string> & fileNameList); // NEW

  void loadFileEntry(unsigned long int iEntry); // OVERLOADED

  void loadEventId(unsigned long int eventIdx); // OVERLOADED
  
  unsigned int getMaxNumberOfStreams() { return GRAW_EVENT_FRAGMENTS; }
  
private:

  bool loadGrawFrame(unsigned int iEntry, bool readFullEvent, unsigned int streamIndex); // OVERLOADED
  void collectEventFragments(unsigned int eventIdx); // OVERLOADED
  void checkEntryForFragments(unsigned int iEntry, unsigned int streamIndex); // OVERLOADED
  std::string getNextFilePath(unsigned int streamIndex); // OVERLOADED

  std::vector<std::shared_ptr<TGrawFile> > myFileList; // NEW [streamIndex]
  std::vector<std::string> myFilePathList, myNextFilePathList; // NEW [streamIndex]
  std::vector<std::map<unsigned int, unsigned int> > myFramesMapList; // NEW [streamIndex, eventId, iEntry]
  std::vector<std::map<unsigned int, unsigned int> > myAsadMapList; // NEW [streamIndex, eventId, AsadId]
  std::vector<std::map<unsigned int, unsigned int> > myCoboMapList; // NEW [streamIndex, eventId, CoboId]
  std::vector<std::set<unsigned int> > myReadEntriesSetList; // NEW [streamIndex, {iEntry, iEntry, ...}]
  
  unsigned int frameLoadRange{1}; // OVERLOADED

};
#endif
#endif

