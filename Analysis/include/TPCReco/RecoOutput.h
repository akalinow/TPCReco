#ifndef _RecoOutput_H_
#define _RecoOutput_H_

#include <string>
#include <vector>
#include <memory>

#include "TPCReco/EventInfo.h"

class TTree;
class TFile;
class Track3D;

class RecoOutput {
public:
  
  RecoOutput();
  
  ~RecoOutput();

  void setRecTrack(const Track3D & aRecTrack);

  void setEventInfo(const eventraw::EventInfo & aEventInfo);

  void open(const std::string & fileName);
    
  void update();

private:
  
  void close();

  std::shared_ptr<Track3D> myTrackPtr;
  
  std::shared_ptr<TFile> myOutputFilePtr;
  std::shared_ptr<TTree> myOutputTreePtr;
  std::shared_ptr<eventraw::EventInfo> myEventInfoPtr;
  
};
#endif

