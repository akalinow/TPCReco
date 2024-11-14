#ifndef _RecHitBuilder_H_
#define _RecHitBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <TF1.h>

#define EVENTTPC_DEFAULT_RECO_METHOD 1  // 0 = equal charge division along the strip
                                        // 1 = weighted charge division from complementary strip directions
#define EVENTTPC_DEFAULT_STRIP_REBIN 2  // number of strips to rebin [1-1024] 
#define EVENTTPC_DEFAULT_TIME_REBIN  5  // number of time cells to rebin [1-512]

class TH2D;
class GeometryTPC;

class RecHitBuilder {
public:
  
  RecHitBuilder();

  ~RecHitBuilder();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  const TH2D & makeRecHits(const TH2D & hProjection);

  TH2D makeCleanCluster(const TH2D & aHisto);

private:

  std::shared_ptr<GeometryTPC> myGeometryPtr;
  TF1 noiseShape, signalShape, emptyShape;
  TH2D hRecHits;
  double maxValueThr{20};
  double windowIntegralThr{40};
  int projection1DHalfSize{10};
  
  double emptyBinThreshold{1.0};
  double kernelSumThreshold{150};//parameter to moved to configuration

  const TH2D & makeTimeProjectionRecHits(const TH2D & hProjection);

  const TH2D & makeStripProjectionRecHits(const TH2D & hProjection);

  const TF1 & fit1DProjection(TH1D* hProj, double initialSigma);

  double getMSE(const TH1D &hProj, const TF1 & aFunc) const;

  const TF1 & fitNoise(TH1D* hProj, double minX, double maxX);
  
  const TF1 & fitSingleHit(TH1D* hProj,
			   double minX, double maxX,
			   double initialMax,
			   double initialSigma);

  std::vector<int> fillClusterAndGetBonduary(const std::vector<int> & neighboursBinsIndices,
					     const TH2D & aHisto,
					     TH2D & aCluster);

  std::vector<int> getKernelIndices(int iBin, const TH2D & aHisto);

  double getKernelSum(const std::vector<int> & kernelBins, const TH2D & aHisto);

  void cleanRecHits();

  std::string adaptHistoTitle(const std::string title) const;

  Int_t FindFirstBinAbove(TH1* histo=0, Double_t threshold=0,
			  Int_t axis=1, Int_t firstBin=-1, Int_t lastBin=-1) const;
  
  Int_t FindLastBinAbove(TH1* histo=0, Double_t threshold=0,
			 Int_t axis=1, Int_t firstBin=-1, Int_t lastBin=-1) const;
  
};
#endif

