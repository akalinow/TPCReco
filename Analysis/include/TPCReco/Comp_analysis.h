#ifndef _Comp_analysis_H_
#define _Comp_analysis_H_

#include <string>
#include <vector>
#include <map>

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/IonRangeCalculator.h"

class TH1F;
class TH2F;
class TProfile;
class TProfile2D;
class Track3D;
namespace eventraw{
  class EventInfo;
}

class Comp_analysis{

public:

  Comp_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,
		double pressure,    // CO2 pressure [mbar]
		double temperature);// CO2 temperature [K]
  
  ~Comp_analysis();

  void fillHistos(Track3D *aRefTrack, eventraw::EventInfo *aRefEventInfo,
		  Track3D *aTestTrack, eventraw::EventInfo *aTestEventInfo);
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr); 

  void setIonRangeCalculator(double pressure, double temperature); // CO2 pressure [mbar] and temperature [K]

  TFile *outputFile;
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::map<std::string, TProfile*> profiles1D;
  std::map<std::string, TProfile2D*> profiles2D;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  IonRangeCalculator myRangeCalculator;
};

#endif
