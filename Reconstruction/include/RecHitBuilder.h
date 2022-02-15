#ifndef _RecHitBuilder_H_
#define _RecHitBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include "TF1.h"

class TH2D;
class GeometryTPC;

class RecHitBuilder {
public:
  
  RecHitBuilder();

  ~RecHitBuilder();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  const TH2D & makeRecHits(const TH2D & hProjection);

private:

  std::shared_ptr<GeometryTPC> myGeometryPtr;
  TF1 noiseShape, signalShape, emptyShape;
  TH2D hRecHits;
  double maxValueThr{20};
  double windowIntegralThr{40};
  int projection1DHalfSize{10};

  const TH2D & makeTimeProjectionRecHits(const TH2D & hProjection);

  const TH2D & makeStripProjectionRecHits(const TH2D & hProjection);

  const TF1 & fit1DProjection(TH1D* hProj, double initialSigma);

  double getMSE(const TH1D &hProj, const TF1 & aFunc) const;

  const TF1 & fitNoise(TH1D* hProj, double minX, double maxX);
  
  const TF1 & fitSingleHit(TH1D* hProj,
			   double minX, double maxX,
			   double initialMax,
			   double initialSigma);

  void cleanRecHits();

  std::string adaptHistoTitle(const std::string title) const;
  
   
};
#endif

