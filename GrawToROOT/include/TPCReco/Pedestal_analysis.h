#ifndef _Peedestal_analysis_H_
#define _Peedestal_analysis_H_

#include <map>

#include <TFile.h>
#include <TProfile.h>

#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/MultiKey.h"

class EventSourceGRAW;
class TProfile;

class Pedestal_analysis{

public:

  Pedestal_analysis(EventSourceGRAW *aSource, std::string aOutputFileName="PedestalTree.root");
  ~Pedestal_analysis();
  
  void fillHistos();

 private:

  std::string myOutputFileName;
  EventSourceGRAW *myEventSource{0};
  std::shared_ptr<TFile> myOutputFilePtr;
  std::map<MultiKey2, TProfile*> myAbsPedestalPerRun;  // pedestal correction values (averaged over all time cells) per channel / ASAD / COBO
  std::map<MultiKey2, TProfile*> myRelPedestalPerRun;  // relative pedestal values w.r.t. FPN shape (averaged over 4 AGET channels) per channel / ASAD / COBO
  std::map<MultiKey2, TProfile*> myNoisePerRun;        // noise RMS w.r.t. FPN shape (averaged over 4 AGET channels) per channel / ASAD / COBO
  std::map<MultiKey3, TProfile*> myFpnShapePerRun;     // FPN shape (averaged over 4 AGET channels) per AGET / ASAD / COBO
                                                       // NOTE: this average shape from all events makes sense only for pedestal runs taken with internal
                                                       //       ASAD trigger, when event-to-event phases of the SCA circular buffers should be in sync
  void initialize();
  void finalize();
};

#endif
