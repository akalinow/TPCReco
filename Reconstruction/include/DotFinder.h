#ifndef _DotFinder_H_
#define _DotFinder_H_

#define DOTFINDER_DEFAULT_HIT_THR     50  // hit threshold
#define DOTFINDER_DEFAULT_CHARGE_THR  200  // total charge threshold (all hits)
#define DOTFINDER_DEFAULT_RADIUS      25.0 // max allowed mismatch [mm] between XY postion from UVW coordinate pairs

#include <string>
#include <map>
//#include <vector>
//#include <memory>
//#include <tuple>

class TH1D;
class TH2D;
class TProfile;
class TProfile2D;
class TVector3;
//class TTree;
class TFile;

class EventTPC;

class DotFinder {
 public:
  
  DotFinder();
  
  ~DotFinder();

 private:
  void openOutputStream(const std::string & fileName);
  void closeOutputStream();
  void fillOutputStream();
  void setEvent(EventTPC* aEvent);
  void setEvent(std::shared_ptr<EventTPC> aEvent);
  void setCuts(unsigned int aHitThr, unsigned int aMinTotalCharge, float aMatchRadiusInMM);
  void reconstruct();
  bool checkCuts(SigClusterTPC &aCluster);
  void initializeHistograms();
  void resetHistograms();

 public:
  void initializeDotFinder(unsigned int hitThr, unsigned int totalChargeThr, float matchRadiusInMM, const std::string & filePath);
  void runDotFinder(std::shared_ptr<EventTPC> aEvent);
  void finalizeDotFinder();

 private:
  EventTPC *myEvent;
  TVector3 myDot3D;
  double myDotDeltaZ;
  /*
  struct RateMetadata {
    double timediff; // [s]
    double event;    // event id    
  }
  struct DotMetadata {
    double x;        // [mm]
    double y;        // [mm]
    double z;        // [mm]
    double timediff; // [s]
    double event;    // event id
  } myDotMetadata;
  */

  unsigned int myHitThr; // ADC units
  unsigned int myTotalChargeThr; // integrated charge per cluster
  double myMatchRadiusInMM; // [mm]
  bool myHistogramsInitialized;
  bool isFirstEvent_All;
  bool isFirstEvent_Dot;
  bool isDotEvent;
  Long64_t myEventCounter_All; // # of total analyzed events
  Long64_t myEventCounter_Dot; // # of point-like events
  Long64_t previousEventTime_All; // from GET electronics: in 10ns units
  Long64_t previousEventTime_Dot; // from GET electronics: in 10ns units
  std::map< std::string, TH1D* > myHistograms; // map of histograms indexed by TH1D name
  std::map< std::string, TH2D* > myHistograms2D; // map of histograms indexed by TH2D name
  std::map< std::string, TProfile* > myHistogramsProf; // map of histograms indexed by TProfile name
  std::map< std::string, TProfile2D* > myHistogramsProf2D; // map of histograms indexed by TProfile2D name
  
  std::shared_ptr<TFile> myOutputFilePtr;
  //  std::shared_ptr<TTree> myOutputTreePtr;
};
#endif

