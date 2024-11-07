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

  /// Calculate track segment tangent in 3D
  /// There are requirements for minimal length of 2D projection to 
  /// provide reliable estimate of the bias
  TVector3 getTangent(int iTrack2DSeed) const;

  /// Calculate track segment bias in 2D. Bias is defined as 
  /// position of the maximum charge in time-strip projection.
  TVector2 get2DBias(definitions::projection_type iProj) const;

  /// Calculate track segment bias in 3D
  /// if acceptDot==true, then bias is calculated even for very short tracks, i.e dots
  /// otherwise there are requirements for minimal length of 2D projection to 
  /// provide reliable estimate of the bias
  TVector3 getBias(int iTrack2DSeed,  bool acceptDot=false) const;

  /// Calculate length in XY plane from two projections using formula for length 
  /// in covariant coordinates: l = sqrt(g_ij * dx^i * dx^j)
  double getXYLength(definitions::projection_type dir1, 
                     definitions::projection_type dir2,
                     double l1, double l2) const;

  //Normalise tangents along the common axis - the time axis
  void normaliseTangents(TVector3 & tangent_U, TVector3 & tangent_V, TVector3 & tangent_W) const;

  /// Calculate length of track projection on strip direction.
  /// Sign of the length is determined by the position of maximum:
  /// -1 -> maximum closer to the beginning
  /// +1 -> maximum closer  the end
  /// this sets tangent along the alpha track from C+alpha events
  /// Maximum is found from give 2D projection - auxProj
  /// the auxProj should be a projection with longest track
  double getSignedLengthProjection(definitions::projection_type iProj,
                                   definitions::projection_type auxProj=definitions::projection_type::DIR_U) const;

  /// Solve equation for cos(phi) and sin(phi) for given two projection tangents
  double getXYTangentPhiFromProjsTangents(definitions::projection_type dir1, definitions::projection_type dir2,
                                          double l1, double l2) const;
  
  TrackSegment3D buildSegment3D(int iTrackSeed=0) const;


  /// Fit a dot - a very short cluster, 
  /// failing minimal length requirements for bias and tangent estimation
  Track3D fitDot(Track3D & aFittedTrack) const;

  /// Fit track restricted parameter set
  /// TANGENT - only tangent is fitted
  /// BIAS - only bias is fitted
  /// BIAS_TANGENT - both bias and tangent are fitted
  void fitTrack3D(Track3D & aTrackCandidate, definitions::fit_type fitType);

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
  
};
#endif

