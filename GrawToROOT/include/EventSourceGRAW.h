#ifndef _EventSourceGRAW_H_
#define _EventSourceGRAW_H_

#include "get/GDataFrame.h"
#include "get/TGrawFile.h"

#include "EventSourceBase.h"
#include "PedestalCalculator.h"

class EventSourceGRAW: public EventSourceBase {
public:

  EventSourceGRAW(){};
  
  EventSourceGRAW(const std::string & geometryFileName);
  
  ~EventSourceGRAW();

  void loadDataFile(const std::string & fileName);

  void loadFileEntry(unsigned long int iEntry);

private:

  void fillEventFromFrame(GET::GDataFrame & aGrawFrame);

  PedestalCalculator myPedestalCalculator;
  GET::GDataFrame myDataFrame;
  std::shared_ptr<TGrawFile> myFile;

  int minSignalCell;
  int maxSignalCell;

};
#endif

