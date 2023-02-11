#ifndef _StripResponseCalculator_H_
#define _StripResponseCalculator_H_

#include <vector>
#include <map>
#include <tuple>
#include <utility>
#include <cmath>

#include "MultiKey.h"

class GeometryTPC;
class PEventTPC;
class TH1D;
class TH2D;
class TF1;
class TF2;
class TF1;
class TVector2;
class TVector3;

class StripResponseCalculator{
  
 public:
  
  StripResponseCalculator() { ; }
  StripResponseCalculator(std::shared_ptr<GeometryTPC> aGeometryPtr,
			  int delta_strips,// consider +/- neighbour strips
			  int delta_timecells, // consider +/- neighbour time cells
			  int delta_pads, // consider +/- neighbour pads for the nearest strip section boundary
			  double sigma_xy, double sigma_z, // horizontal and vertical gaussian spread [mm]
			  double peaking_time=0, // [ns] AGET peaking time, 0=none
			  const char *fname=NULL, // optional file with pre-computed response histograms
			  bool debug_flag=false); // optional debug flag

  // declare objects to be filled (TH2 histograms and/or EventTPC)
  bool setUVWprojectionsRaw(std::vector<TH2D*> aUVWprojectionsRaw); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsInMM(std::vector<TH2D*> aUVWprojectionsInMM); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsRaw(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsRaw); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsInMM(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsInMM); // pair={STRIP_DIR, TH2D pointer}
  //  bool setEventTPC(EventTPC *aEventTPC);
  //  bool setEventTPC(std::shared_ptr<EventTPC> aEventTPC);

  // fill all declared objects (TH2 histograms and/or EventTPC) with smeared point-like charge
  void addCharge(TVector3 position3d, double charge, std::shared_ptr<PEventTPC> aEventPtr=std::shared_ptr<PEventTPC>(nullptr));
  void addCharge(double x, double y, double z, double charge, std::shared_ptr<PEventTPC> aEventPtr=std::shared_ptr<PEventTPC>(nullptr));

  void setDebug(bool enable) { debug_flag=enable; }
  int getDeltaStrips() const { return Nstrips; }
  int getDeltaPads() const { return Npads; }
  int getDeltaTimecells() const { return Ntimecells; }
  double getSigmaXY() const { return sigma_xy; } // [mm]
  double getSigmaZ() const { return sigma_z; } // [mm]
  double getPeakingTime() const { return peaking_time; } // [ns]
  std::shared_ptr<GeometryTPC> getGeometryPtr() const { return myGeometryPtr; }

  // returns clone of underlying response histogram
  std::shared_ptr<TH2D> getStripResponseHistogram(int dir, int delta_strip);
  std::shared_ptr<TH2D> getStripSectionStartResponseHistogram(int dir, int delta_strip, int delta_pad);
  std::shared_ptr<TH1D> getTimeResponseHistogram(int delta_timecell);

  // returns array with {u0, v0, w0} triplet corresponding to the nearest node in XY plane
  std::vector<int> getReferenceStripNode(TVector3 position3d, TVector2 *refNodePosInMM=NULL) const;
  std::vector<int> getReferenceStripNode(TVector2 position2d, TVector2 *refNodePosInMM=NULL) const;
  std::vector<int> getReferenceStripNode(double x, double y, TVector2 *refNodePosInMM=NULL) const;

  // returns t0 time cell index corresponding to Z position
  int getReferenceTimecell(TVector3 position3d) const;
  int getReferenceTimecell(double z) const;

  // save underlying response histograms to new TFile
  bool saveHistograms(const char *fname);
  // load underlying response histograms from existing TFile
  bool loadHistograms(const char *fname);
  // re-generate underlying response histograms
  void initializeStripResponse(unsigned long NpointsXY=StripResponseCalculator::default_NpointsXY,
			       int NbinsXY=StripResponseCalculator::default_NbinsXY);
  void initializeTimeResponse(unsigned long NpointsPeakingTime=StripResponseCalculator::default_NpointsPeakingTime,
			      int NbinsZ=StripResponseCalculator::default_NbinsZ);

 private:

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient member
  int Nstrips{0};
  int Ntimecells{0};
  int Npads{0};
  static const int default_NbinsZ{20};  // {5}; // granularity of strip response histograms in Z domain (sub-time cell resolution)
  static const int default_NbinsXY{10}; // {7}; // granularity of strip response histograms in X/Y domain (sub-strip resolution)
  static const unsigned long default_NpointsXY{10000}; // # of sampling points for initialization of strip response histograms
  //// TEST
  static const unsigned long default_NpointsPeakingTime{10000}; // # of sampling points for GET electronics smearing
  //// TEST
  double sigma_xy{0}; // [mm]
  double sigma_z{0}; // [mm]
  //// TEST
  double peaking_time{0}; // AGET peaking time [ns], 0=none, valid range: 70-1014 ns
  //// TEST

  bool has_UVWprojectionsRaw{false};
  bool has_UVWprojectionsInMM{false};
  bool debug_flag{false};
  std::vector<TH2D*> fillUVWprojectionsRaw;
  std::vector<TH2D*> fillUVWprojectionsInMM;
  
  std::map<MultiKey2, TH2D*> responseMapPerMergedStrip; // key={strip_dir, relative (merged) strip index} 
  std::map<MultiKey3, TH2D*> responseMapPerStripSectionStart; // key={strip_dir, relative strip index, relative section start in pad units}
  std::map<int, TH1D*> responseMapPerTimecell; // key=relative time cell index

  // returns name of underlying response histogram
  const char* getStripResponseHistogramName(int dir, int delta_strip);
  const char* getStripSectionStartResponseHistogramName(int dir, int delta_strip, int delta_pad);
  const char* getTimeResponseHistogramName(int delta_timecell);
};

#endif
