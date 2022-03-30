#ifndef _Comp_analysis_H_
#define _Comp_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1D.h"
#include "TH2D.h"

class TH1F;
class TH2F;
class Track3D;
namespace eventraw{
  class EventInfo;
}
class GeometryTPC;

class Comp_analysis{

public:

  Comp_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr);
  
  ~Comp_analysis();

  void fillHistos(Track3D *aRefTrack, eventraw::EventInfo *aRefEventInfo,
		  Track3D *aTestTrack, eventraw::EventInfo *aTestEventInfo);
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr); 

  TFile *outputFile;
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::shared_ptr<GeometryTPC> myGeometryPtr; 
};

#endif
