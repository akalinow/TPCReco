#ifndef _DotFinder_H_
#define _DotFinder_H_

#define DOTFINDER_DEFAULT_HIT_THR          50  // hit threshold
#define DOTFINDER_DEFAULT_CHARGE_THR       200  // total charge threshold (all hits)
//#define DOTFINDER_DEFAULT_NSTRIPS     5   // max number of fired strips per direction
//#define DOTFINDER_DEFAULT_NTIMECELLS  10  // max number of fired time cells per direction
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
class TVector2;
class TVector3;
//class TF1;
//class TTree;
class TFile;

class GeometryTPC;
class EventTPC;

class DotFinder {
public:
  
  DotFinder();
  
  ~DotFinder();

  void openOutputStream(const std::string & fileName);
  void closeOutputStream();
  void fillOutputStream();

  void setEvent(EventTPC* aEvent);
  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setCuts(unsigned int aHitThr,
	       //	       unsigned int aMaxStripsPerDir,
	       //	       unsigned int aMaxTimeCellsPerDir,
	       unsigned int aMinTotalCharge,
	       double aMatchRadiusInMM);

  //  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void reconstruct();
  bool checkCuts();
  void initializeHistograms();
  void resetHistograms();

  //  const SigClusterTPC & getCluster() const { return myCluster;}

  //  const TH2D & getRecHits2D(int iDir) const;

  //  const TH2D & getHoughtTransform(int iDir) const;
  
  //  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;

  //  void getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY);
  
  //  const TrackSegment3D & getSegment3DSeed() const;

  //  const Track3D & getTrack3D(unsigned int iSegment) const;

private:

  //  void makeRecHits(int iDir);

  //  TF1 fitTimeWindow(TH1D* hProj);
 
  //  void fillHoughAccumulator(int iDir);

  //  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  //  TrackSegment2D findSegment2D(int iDir, int iPeak) const;
  
  //  TrackSegment3D buildSegment3D(int iTrackSeed=0) const;
  
  //  Track3D fitTrack3D(const TrackSegment3D & aTrackSeedSegment) const;

  //  Track3D fitTrackNodes(const Track3D & aTrack) const;

  //  double fitTrackSplitPoint(const Track3D& aTrackCandidate) const;
   
  EventTPC *myEvent;
  //  SigClusterTPC myCluster;
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
  //  std::shared_ptr<GeometryTPC> myGeometryPtr;
  //  std::vector<double> phiPitchDirection;

  unsigned int myHitThr; // ADC units
  //  unsigned int myMaxStripsPerDir; // strip pitch units
  //  unsigned int myMaxTimeCellsPerDir; // time cells units
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
  
  //  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  //  TVector3 aHoughOffest;
  //  std::vector<TH2D> myAccumulators;
  //  std::vector<TH2D> myRecHits;
  //  std::vector<TrackSegment2DCollection> my2DSeeds;

  //  TrackSegment2D dummySegment2D;
  //  TrackSegment3D myTrack3DSeed, dummySegment3D;
  //  Track3D myFittedTrack;
  //  Track3D *myFittedTrackPtr;

  std::shared_ptr<TFile> myOutputFilePtr;
  //  std::shared_ptr<TTree> myOutputTreePtr;

  //  mutable ROOT::Fit::Fitter fitter;
  
};
#endif

