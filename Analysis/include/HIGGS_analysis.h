#ifndef _HIGGS_analysis_H_
#define _HIGGS_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1D.h"
#include "TH2D.h"

class TH1F;
class TH2F;
class Track3D;

class HIGGS_analysis{

public:

  HIGGS_analysis();

  ~HIGGS_analysis();

  void fillHistos(Track3D *aTrack);
  
private:

  void bookHistos();

  void finalize();

  TFile *outputFile;
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;


};

#endif
