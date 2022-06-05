#ifndef _StripResponseCalculator_H_
#define _StripResponseCalculator_H_

#include <vector>
#include <map>
#include <tuple>
#include <utility>

class GeometryTPC;
class MultiKey2;
class TH1D;
class TH2D;
class TF1;
class TF2;
class TVector2;
class TVector3;

class StripResponseCalculator{
  
 public:
  
  StripResponseCalculator() { ; }
  StripResponseCalculator(std::shared_ptr<GeometryTPC> aGeometryPtr,
			  int delta_strips, int delta_timecells, // consider +/- neighbour strips, time cells
			  double sigma_xy, double sigma_z, // horizontal and vertical gaussian spread [mm]
			  bool debug_flag=false, // optional debug flag
			  const char *fname=""); // optional file with pre-computed response histograms

  // declare TH2 histograms to be filled
  bool setUVWprojectionsRaw(std::vector<TH2D*> aUVWprojectionsRaw); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsInMM(std::vector<TH2D*> aUVWprojectionsInMM); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsRaw(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsRaw); // pair={STRIP_DIR, TH2D pointer}
  bool setUVWprojectionsInMM(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsInMM); // pair={STRIP_DIR, TH2D pointer}

  // fill all declared TH2 histograms with smeared point-like charge
  void fillProjections(TVector3 position3d, double charge);
  void fillProjections(double x, double y, double z, double charge);

  void setDebug(bool enable) { debug_flag=enable; }
  int getDeltaStrips() const { return Nstrips; }
  int getDeltaTimecells() const { return Ntimecells; }
  double getSigmaXY() const { return sigma_xy; } // [mm]
  double getSigmaZ() const { return sigma_z; } // [mm]
  std::shared_ptr<GeometryTPC> getGeometryPtr() const { return myGeometryPtr; }

  // returns clone of underlying response histogram
  std::shared_ptr<TH2D> getStripResponseHistogram(int dir, int delta_strip);
  std::shared_ptr<TH1D> getTimeResponseHistogram(int delta_timecell);

  // returns array with {u0, v0, w0} triplet corresponding to the nearest node in XY plane
  std::vector<int> getReferenceStripNode(TVector3 position3d, TVector2 *refNodePosInMM=NULL) const;
  std::vector<int> getReferenceStripNode(TVector2 position2d, TVector2 *refNodePosInMM=NULL) const;
  std::vector<int> getReferenceStripNode(double x, double y, TVector2 *refNodePosInMM=NULL) const;

  // returns t0 time cell index corresponding to Z position
  int getReferenceTimecell(TVector3 position3d) const;
  int getReferenceTimecell(double z) const;

  // save underlyiong response histograms to new TFile
  bool saveResponseHistograms(const char *fname);

 private:

  std::shared_ptr<GeometryTPC> myGeometryPtr;
  int Nstrips{0};
  int Ntimecells{0};
  const int Nbins=30; // granularity of strip response histograms in X/Y/Z domains
  const unsigned long Npoints=100000; // # of sampling points for initialization of strip response histograms
  double sigma_xy{0}; // [mm]
  double sigma_z{0}; // [mm]
  //  static TF2 *gauss_XY; // 2D normal distribution centered at (0,0) with sigma_xy spread, normalized to 1
  //  static TF1 *gauss_Z; // 1D normal distribution centered at 0 with sigma_z spread, normalized to 1

  bool has_UVWprojectionsRaw{false};
  bool has_UVWprojectionsInMM{false};
  bool debug_flag{false};
  std::vector<TH2D*> myUVWprojectionsRaw;
  std::vector<TH2D*> myUVWprojectionsInMM;
  
  std::map<MultiKey2, TH2D*> responseMapPerMergedStrip; // key={strip_dir, relative (merged) strip index} 
  std::map<int, TH1D*> responseMapPerTimecell; // key=relative time cell index

  void initializeStripResponse();
  void initializeTimeResponse();

  // returns name of underlying response histogram
  const char* getStripResponseHistogramName(int dir, int delta_strip);
  const char* getTimeResponseHistogramName(int delta_timecell);

  // load underlying response histograms from existing TFile
  bool loadResponseHistograms(const char *fname);
};

#endif
