#ifndef __PEDESTAL_CALCULATOR_GRAW_H__
#define __PEDESTAL_CALCULATOR_GRAW_H__

// Pedestal Calculator (GRAW)
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
#include "PedestalCalculator.h"

#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"

class PedestalCalculatorGRAW : public PedestalCalculator {
  
 public:

  void CalculateEventPedestals(const GET::GDataFrame & dataFrame);

 private:

  void ResetTables(int coboId, int asadId);
  void ProcessDataFrame(const GET::GDataFrame & dataFrame, bool calculateMean);

};


#endif
