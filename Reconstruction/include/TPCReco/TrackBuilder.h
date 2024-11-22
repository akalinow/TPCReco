#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <boost/property_tree/ptree.hpp>

#include <Fit/Fitter.h>

#include "TPCReco/TrackSegment2D.h"
#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/RecHitBuilder.h"
#include "TPCReco/dEdxFitter.h"

#include "TPCReco/EventTPC.h"
#include "TPCReco/EventInfo.h"

class TH2D;
class TF1;
class TTree;
class TFile;

class GeometryTPC;
class SigClusterTPC;

class TrackBuilder {
public:
  
  TrackBuilder();
  
  ~TrackBuilder();

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void setPressure(double aPressure);

  void reconstruct();

  const TH2D & getCluster2D(int iDir) const;

  const TH2D & getRecHits2D(int iDir) const;

  const TH1D & getRecHitsTimeProjection() const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;

  void getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY);
  
  const TrackSegment3D & getSegment3DSeed() const;

  const Track3D & getTrack3D(unsigned int iSegment) const;

  TF1 getdEdx() const {return mydEdxFitter.getFittedModel();};

private:

  void makeRecHits(int iDir);

  void fillHoughAccumulator(int iDir);

  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  TrackSegment2D findSegment2D(int iDir, int iPeak) const;

  /// Calculate lengths of 1D projections and store them in the map
  /// Calculate positions of maximum charge in strip-time projections and store them in the map
  void getSignedLengthsAndMaxPos(int iTrack2DSeed=-1);

  /// Calculate track segment tangent in 3D
  /// There are requirements for minimal length of 2D projection to 
  /// provide reliable estimate of the bias
  TVector3 getTangent(int iTrack2DSeed=-1);

  /// Calculate track segment bias in 2D. Bias is defined as 
  /// position of the maximum charge in time-strip projection.
  /// if iTrack2DSeed is given, then the bias is taken from the seed
  TVector2 get2DBias(definitions::projection_type iProj, int iTrack2DSeed = -1) const;

  /// Calculate track segment bias in 3D
  /// if acceptDot==true, then bias is calculated even for very short tracks, i.e dots
  /// otherwise there are requirements for minimal length of 2D projection to 
  /// provide reliable estimate of the bias
  TVector3 getBias(int iTrack2DSeed=-1,  bool acceptDot=false);

  /// Calculate length in XY plane from two projections using formula for length 
  /// in covariant coordinates: l = sqrt(g_ij * dx^i * dx^j)
  double getXYLength(definitions::projection_type dir1, 
                     definitions::projection_type dir2,
                     double l1, double l2) const;

  /// Calculate the 3D track azimuthal angle using ratios
  /// of the unsigned lengths of projections on strip directions
  double getTangentPhiFromUnsignedLengths(double l_U, double l_V, double l_W) const;

  /// Solve equation for cos(phi) and sin(phi) for given two projection tangents
  /// use signed lengths.
  double getTangentPhiFromSignedLengths(definitions::projection_type dir1, 
                                        definitions::projection_type dir2,
                                        double l1, double l2) const;
  

  /// Calculate length of track projection on strip direction.
  /// Sign of the length is determined by the position of maximum:
  /// -1 -> maximum closer to the beginning
  /// +1 -> maximum closer  the end
  /// this sets tangent along the alpha track from C+alpha events
  /// Maximum is found from give 2D projection - auxProj
  /// the auxProj should be a projection with longest track
  /// if iTrack2DSeed is given, then the length is taken from the seed
  double getSignedLengthProjection(definitions::projection_type iProj,
                                   definitions::projection_type auxProj=definitions::projection_type::DIR_U,
                                   int iTrack2DSeed = -1) const;

  /// Calculate length of track projection on strip direction.
  /// use TProfile. This works only for nearly horizontal or vertical tracks
  double getLengthProjectionFromProfile(definitions::projection_type iProj, 
                                        definitions::projection_type auxProj) const;                             


  TrackSegment3D buildSegment3D(int iTrackSeed=-1);

  /// Fit a dot - a very short cluster, 
  /// failing minimal length requirements for bias and tangent estimation
  Track3D fitDot(Track3D & aFittedTrack);

  /// Fit track restricted parameter set
  /// TANGENT - only tangent is fitted
  /// BIAS - only bias is fitted
  /// BIAS_TANGENT - both bias and tangent are fitted
  void fitTrack3DInSelectedDir(Track3D & aTrackCandidate, definitions::fit_type fitType);

  Track3D fitTrack3D(const Track3D & aTrackCandidate);

  Track3D fitEventHypothesis(const Track3D & aTrackCandidate);
  
  ROOT::Fit::FitResult fitTrackNodesBiasTangent(const Track3D & aTrack, definitions::fit_type fitType) const;

  std::tuple<double, double> getProjectionEdges(const TH1D &hProj, int binMargin=10) const;

  std::shared_ptr<EventTPC> myEventPtr;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  RecHitBuilder myRecHitBuilder;
  dEdxFitter mydEdxFitter;
  double myPressure{190};
  
  std::vector<double> phiPitchDirection;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  //  TVector3 aHoughOffest;
  std::vector<TVector3> myHoughOffset;
  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits, myRawHits;
  TH1D hTimeProjection;
  std::vector<TrackSegment2DCollection> my2DSeeds;
  std::tuple<double, double> myZRange;

  TrackSegment2D dummySegment2D;
  TrackSegment3D myTrack3DSeed, dummySegment3D;  
  Track3D myTmpTrack, myFittedTrack;
  
  mutable ROOT::Fit::Fitter fitter;

  std::map<definitions::projection_type, double> lengths;
  std::map<definitions::projection_type, TVector2> biases2D;
  definitions::projection_type long_proj_type{definitions::projection_type::DIR_U};

  double minStripProjLength{20}; //parameter to be moved to configuration
  double minStripProjLengthForVertTracks{20}; //parameter to be moved to configuration
  double minTimeProjLength{20};  //parameter to be moved to configuration 
  double epsilon{1E-2};          //parameter to be moved to configuration
  double stripDiffusionMargin{2.0}; //parameter to be moved to configuration
  double timeDiffusionMargin{4.0}; //parameter to be moved to configuration

};
#endif

