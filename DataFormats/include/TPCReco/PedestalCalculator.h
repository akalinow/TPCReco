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

#include <TH1D.h>
#include <TH2D.h>
#include <TH3F.h>
#include <TProfile.h>
#include <TString.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventRaw.h"

class PedestalCalculatorGRAW;

class PedestalCalculator {
  
 public:

  PedestalCalculator();

  ~PedestalCalculator();

  void SetGeometryAndInitialize(std::shared_ptr<GeometryTPC> aPtr);

  //  double GetPedestalCorrection(int iChannelGlobal, int agentId, int iCell);
  double GetPedestalCorrection(int coboId, int asadId, int agetId, int chanId, int iCell);
  void CalculateEventPedestals(const std::shared_ptr<eventraw::EventRaw> eRaw);

  int GetMinSignalCell() const {return minSignalCell;}
  int GetMaxSignalCell() const {return maxSignalCell;}
  int GetMinPedestalCell() const {return minPedestalCell;}
  int GetMaxPedestalCell() const {return maxPedestalCell;}
  std::shared_ptr<TProfile> GetPedestalProfilePerAsad(int coboId, int asadId);
  std::shared_ptr<TH1D> GetFpnProfilePerAget(int coboId, int asadId, int agetId);

  void SetMinSignalCell(int minSignalCell ) {this->minSignalCell=minSignalCell;}
  void SetMaxSignalCell(int maxSignalCell) {this->maxSignalCell=maxSignalCell;}
  void SetMinPedestalCell(int minPedestalCell) {this->minPedestalCell=minPedestalCell;}
  void SetMaxPedestalCell(int maxPedestalCell) {this->maxPedestalCell=maxPedestalCell;}

 private:

  friend class PedestalCalculatorGRAW;
  
  void InitializeTables();

  void InitializeMonitoringHistos();

  void ResetTables();

  void ProcessEventRaw(const std::shared_ptr<eventraw::EventRaw> eRaw, bool calculateMean);

  int nchan, maxval, nbin_spectrum;
  int minSignalCell, maxSignalCell;
  int minPedestalCell, maxPedestalCell;

  std::shared_ptr<GeometryTPC> myGeometryPtr;

  //  std::vector<double> pedestals;
  // array index: [cobo(>=0)][asad(0-3)[channel(0-255)]
  std::vector< std::vector< std::vector<double> > > pedestals;

  // array index: [aget(0-3)][cell(0-511)]
  //  std::vector< std::vector<uint> > FPN_entries_pedestal;
  //  std::vector< std::vector<double> > FPN_ave_pedestal;
  //  std::vector< std::vector<uint> > FPN_entries_signal;
  //  std::vector< std::vector<double> > FPN_ave_signal;
  // array index: [cobo(>=0)][asad(0-3)][aget(0-3)][cell(0-511)]
  std::vector< std::vector< std::vector< std::vector<uint32_t> > > > FPN_entries_pedestal;
  std::vector< std::vector< std::vector< std::vector<double> > > > FPN_ave_pedestal;
  std::vector< std::vector< std::vector< std::vector<uint32_t> > > > FPN_entries_signal;
  std::vector< std::vector< std::vector< std::vector<double> > > > FPN_ave_signal;
  
  // GLOBAL - PEDESTAL CONTROL HISTOGRAMS  
  // Up to 1024*(NCobos) channels with pedestal (offset)
  // wrt. average of 4 FPN channels from corresponding AGET chip,
  // where error=spread (rms)
  //  TProfile* prof_pedestal;
  std::map< MultiKey2, TProfile*> prof_pedestal_map; // key=[coboId[>=0], asadId[0-3]]
    
};


#endif
