#ifndef _TrackDiffusion_analysis_H_
#define _TrackDiffusion_analysis_H_

#include <map>
#include <TFile.h>
#include <TTree.h>

#include "TPCReco/TrackDiffusion_tree_dataFormat.h"
#include "TPCReco/TrackSegment3D.h"

class GeometryTPC;
class EventTPC;
class Track3D;
class TGraph;
class TH2D;

typedef std::vector<TGraph> TrackDiffusionCropList; // ROI around each track from Track3D collection for a given strip direction

class TrackDiffusion_tree_analysis{

 public:
  // helper class with track diffusion config parameters
  struct TrackDiffusionConfig {
    bool clusterEnable{true};
    float clusterThreshold{35};
    int clusterDeltaStrips{10};
    int clusterDeltaTimeCells{50};
    float trackFractionStart{0.2}; // [0-1] fraction of track length to be skipped near track vertex
    float trackFractionEnd{0.1}; // [0-1] fraction of track length to be skipped near track end
    float trackDistanceMM{10.0}; // [mm] defines band around track axis for hit projection
    float hitThreshold{0.0}; // [ADC] only hits with charge above this value enter MEAN and RMS (excludes negative noise)
    bool dumpHits{false}; // write the "Hits" tree: every hit near the leading segment, with its position along and across it
  };

  // helper class with 1D transversal hit position wrt track line
  class Hit1D {
  public:
    Hit1D(double positionMM, double charge) : x(positionMM), q(charge) { }
    const double x; // [mm]
    const double q; // [ADC units]
  };

  TrackDiffusion_tree_analysis(const std::shared_ptr<GeometryTPC> aGeometryPtr,
			       const TrackDiffusionConfig &aConfig,
			       const std::string aOutputFileName="RawTrackDiffusionTree.root"); // definition of LAB detector coordinates
  ~TrackDiffusion_tree_analysis();
  
  //  void fillTree(const std::shared_ptr<EventTPC> aEventTPC, const std::shared_ptr<Track3D> aTrack);
  void fillTree(const std::shared_ptr<EventTPC> aEventTPC, Track3D *aTrack);

 private:

  TrackDiffusionConfig myConfig;
  Event_rawdiffusion event_rawdiffusion;
  std::string myOutputFileName;
  std::shared_ptr<TFile> myOutputFilePtr;
  std::shared_ptr<TTree> myOutputTreePtr;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  std::map<int, std::vector<Hit1D> > myHitMap;// key = strip dir [0-3]
  // Transverse hit profiles (owned by the output file): charge-weighted distribution of the signed hit
  // distance from the track axis vs |cos(theta_DET)| of the leading track, each event normalised to unit
  // charge; per strip direction (U, V, W) and all directions, for all events and for 2-prong events.
  // Key: "<all|2prong>_<U|V|W|All>".
  std::map<std::string, TH2D*> myProfiles;

  // "Hits" tree (config dumpHits): for each strip direction, the hits of the clustered strip-time projection within
  // trackDistanceMM of the projected leading segment and with lambda in [-0.1, 1.1], independent of the ROI cuts.
  // lambda: position along the projected segment, 0 = vertex, 1 = end (3D distance from the vertex = lambda*length);
  // across: signed distance [mm] from the projected axis. dz2D/ds2D: projected segment (drift, strip) per direction.
  struct HitDump {
    int runId{-1}; long eventId{-1}; int ntracks{0};
    float length{0}, cosThetaDET{0}, phiDET{0};
    float vtx[3]{0,0,0}, end[3]{0,0,0}, carbonEnd[3]{0,0,0};
    float dz2D[3]{0,0,0}, ds2D[3]{0,0,0};
    std::vector<int> dir; std::vector<float> lambda, across, q;
  } myHitDump;
  std::shared_ptr<TTree> myHitTreePtr;
  void fillHitDump(const std::vector<std::shared_ptr<TH2D> > &aHistos, TrackSegment3DCollection &aColl);
  
  void initialize();
  void finalize();
  void clear();
  void updateMeanRms();
  void fillProfiles(); // fills myProfiles from myHitMap (after updateMeanRms)

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
  TrackDiffusionCropList getCropList(TrackSegment3DCollection &aColl, int dir); // returns list of TGraphs with crop areas around each track segment (for a given strip dir)
  bool verifyCropList(TrackDiffusionCropList &aList); // returns TRUE when ROIs do not overlap (for a given strip dir)
  bool processCropList(TrackDiffusionCropList &aList, TrackSegment3DCollection &aColl, std::shared_ptr<TH2D>, int dir); // fills event_rawdiffusion structure
  bool edgesInside(TGraph &g1, TGraph &g2);
  bool edgesIntersect(TGraph &g1, TGraph &g2);
};

#endif
