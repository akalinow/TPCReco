#ifndef _Comp_analysis_H_
#define _Comp_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1D.h"
#include "TH2D.h"
//// TEST
#include "TProfile.h"
#include "TProfile2D.h"
//// TEST

class TH1F;
class TH2F;
//// TEST
class TProfile;
class TProfile2D;
//// TEST
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
  //// TEST
  std::map<std::string, TProfile*> profiles1D;
  std::map<std::string, TProfile2D*> profiles2D;
  //// TEST
  std::shared_ptr<GeometryTPC> myGeometryPtr; 
};

#endif
