#ifndef _EventSourceROOT_H_
#define _EventSourceROOT_H_

#include "EventSourceBase.h"

class TFile;
class TTree;

class EventSourceROOT: public EventSourceBase {
public:
  
  EventSourceROOT();
  
  ~EventSourceROOT();

  void loadDataFile(const std::string & fileName);

  void loadFileEntry(unsigned long int iEntry);

private:

  EventTPC *aPtr;
  std::string treeName;
  std::shared_ptr<TFile> myFile;
  std::shared_ptr<TTree> myTree;
  
};
#endif

