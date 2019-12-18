#ifndef __PEDESTAL_CALCULATOR_H__
#define __PEDESTAL_CALCULATOR_H__

// Pedestal Calculator
// Author: Artur Kalinowski
// Mon Jun 17 13:31:58 CEST 2019

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <memory>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"
#include "TProfile.h"

#include "GeometryTPC.h"

#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"

class PedestalCalculator {
  
 public:

  PedestalCalculator();

  ~PedestalCalculator();

  void SetGeometryAndInitialize(std::shared_ptr<GeometryTPC> aPtr);

  double GetPedestalCorrection(int iChannelGlobal, int agentId, int iCell);

  void CalculateEventPedestals(const GET::GDataFrame & dataFrame);

 private:

  void InitializeTables();

  void InitializeMonitoringHistos();

  void ResetTables();

  void ProcessDataFrame(const GET::GDataFrame & dataFrame, bool calculateMean);

  int nchan, maxval, nbin_spectrum;
  int minSignalCell, maxSignalCell;
  int minPedestalCell, maxPedestalCell;

  std::shared_ptr<GeometryTPC> myGeometryPtr;

  std::vector<double> pedestals;

  std::vector< std::vector<uint32_t> > FPN_entries_pedestal;
  std::vector< std::vector<double> > FPN_ave_pedestal;

  std::vector< std::vector<uint32_t> > FPN_entries_signal;
  std::vector< std::vector<double> > FPN_ave_signal;
  
  // GLOBAL - PEDESTAL CONTROL HISTOGRAMS  
  // 256 channels with pedestal (offset)
  // wrt. average of 4 FPN channels, where error=spread (rms)
  TProfile* prof_pedestal;
    
};


#endif
