//////////////////////////
//
// root
// root [0] .L runTimeSec.cxx
// root [1] runTimeSec("Reco_combined.root", 1000);
//
//
//////////////////////////
//////////////////////////
//
// Sorts events from list of Reco ROOT files in ascending
// order using {Run ID, Event ID}.
// Intergrates time intervals between reconstructed events.
// Ignores intervals above user specified threshold in seconds
// to account for run transitions, not yet analyzed files, etc.
//

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <TROOT.h>
#include <TSystem.h>
#include <TTree.h>
#include <TFile.h>
#include <TTreeIndex.h>
#include <TString.h>

#include "Track3D.h"
#include "EventInfo.h"
#include "UtilsEventInfo.h"

void runTimeSec(const char *filename, const double max_interval_sec=1000) {

  if (!gROOT->GetClass("Track3D")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("RunIdParser")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }

  auto f=TFile::Open(filename, "OLD");
  if(!f) {
    std::cerr << "ERROR: Cannot open file: " << filename << std::endl;
    return;
  }
  auto *t = (TTree*)(f->Get("TPCRecoData"));
  if(!t) {
    std::cerr << "ERROR: TTree pointer is empty!" << std::endl;
    return;
  }

  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = t->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr << "ERROR: Cannot find 'RecoEvent' branch!" << std::endl;
    return;
  }
  aBranch->SetAddress(&aTrack);
  
  eventraw::EventInfo *aEventInfo = 0;
  TBranch *aBranchInfo = t->GetBranch("EventInfo");
  if(!aBranchInfo) {
    std::cerr << "ERROR: Cannot find 'EventInfo' branch!" << std::endl;
    return;
  }
  aEventInfo = new eventraw::EventInfo();
  aBranchInfo->SetAddress(&aEventInfo);
  
  unsigned int nEntries = t->GetEntries();
  
  t->BuildIndex("runId", "eventId");
  TTreeIndex *I=(TTreeIndex*)t->GetTreeIndex(); // get the tree index
  Long64_t *index=NULL;
  if(I) {
    std::cout << "I=" << I << std::endl;
    index=I->GetIndex();
    std::cout << "index=" << index << std::endl << std::flush;
  }
  if(!index || !I) {
    std::cerr << "ERROR: TTree index returned a NULL pointer!" << std::endl;
    return;
  }

  double integratedRunTimeSec=0.0;
  bool isFirst=true;
  
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    aBranch->GetEntry(index[iEntry]);  // sorted tree
    aBranchInfo->GetEntry(index[iEntry]); // sorted tree
    
    auto runId = aEventInfo->GetRunId();
    auto eventId = aEventInfo->GetEventId();
    auto eventTime10ns = tpcreco::eventRelativeTime(*aEventInfo).count();
    double unixTimeSec= std::chrono::duration_cast<std::chrono::duration<long double>>(tpcreco::eventAbsoluteTime(*aEventInfo).time_since_epoch()).count();
    static double last_timestamp = 0;
    if(isFirst) {
      last_timestamp=unixTimeSec;
      isFirst=false;
    }
    auto deltaTimeSec=(unixTimeSec - last_timestamp); // [s] time difference for rate measurements
    last_timestamp=unixTimeSec;

    std::cout << "Sorted tree index=" << index[iEntry] << ": run=" << runId << ", evt=" << eventId << ", deltaSec=" << Form("%.20le", deltaTimeSec)
	      << ", unixSec=" << Form("%.20le", unixTimeSec) << std::endl;

    if(deltaTimeSec<max_interval_sec) integratedRunTimeSec += deltaTimeSec;
    else {
      std::cout << "Time gap " << deltaTimeSec << " sec is greater than " << max_interval_sec << " sec" << std::endl;
    }
  }

  std::cout << std::endl << "Total integrated time: " << integratedRunTimeSec << " sec" << std::endl << std::endl;

  f->Close();
}
