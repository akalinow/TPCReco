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

#include "TROOT.h"
#include "TSystem.h"
#include "TTree.h"
//#include "TChain.h"
#include "TTreeIndex.h"
#include "TString.h"

#include "Track3D.h"
#include "EventInfo.h"
#include "UtilsMath.h"

void runTimeSec(const char *filename, const double max_interval_sec=1000) {

  if (!gROOT->GetClass("Track3D")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("getUnixTimestamp")){
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
    //    std::cout << index[iEntry] << std::endl;
    //    aBranch->GetEntry(iEntry);
    //    aBranchInfo->GetEntry(iEntry);
    aBranch->GetEntry(index[iEntry]);  // sorted tree
    aBranchInfo->GetEntry(index[iEntry]); // sorted tree
    
    auto runId = aEventInfo->GetRunId();
    auto eventId = aEventInfo->GetEventId();
    auto eventTime10ns = aEventInfo->GetEventTimestamp(); // 10ns units
    double unixTimeSec=Utils::getUnixTimestamp( runId, eventTime10ns ); // absolute Unix time [s]
    static double last_timestamp = 0;
    if(isFirst) {
      last_timestamp=unixTimeSec;
      isFirst=false;
    }
    auto deltaTimeSec=(double)(unixTimeSec - last_timestamp); // [s] time difference for rate measurements
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

  /*
  // Initialize vector with ROOT file names
  std::vector<std::string> filelist;
  std::ifstream file(listname);
  if(file) {
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string filename;
    while(getline(buffer, filename)) {
      if(filename.size()==0) continue; // ignore empty lines
      std::cout << filename << std::endl;
      filelist.push_back(filename);
    };
  }
  if(filelist.size()==0) {
    std::cerr << "ERROR: Inout list of ROOT files is empty!" << std::endl;
    return;
  }
    
  // Initialize TChain
  auto chain = new TChain("t");
  for(auto & it : filelist) {
    chain->AddFile( it.c_str(), 0, "TPCRecoData");
  }
  
  // Sort TChain according to {Run ID, Event ID}
  //
  // NOTE: * branch RecoEvent - must always be present
  //       * branch EventInfo - is optional (eg. for Monte Carlo)
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = chain->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr << "ERROR: Cannot find 'RecoEvent' branch!" << std::endl;
    return;
  }
  aBranch->SetAddress(&aTrack);
  
  eventraw::EventInfo *aEventInfo = 0;
  TBranch *aBranchInfo = chain->GetBranch("EventInfo");
  if(!aBranchInfo) {
    std::cerr << "ERROR: Cannot find 'EventInfo' branch!" << std::endl;
    return;
  }
  aEventInfo = new eventraw::EventInfo();
  aBranchInfo->SetAddress(&aEventInfo);

  unsigned int nEntries = chain->GetEntries();
  
  chain->BuildIndex("runId", "eventId");
  TTreeIndex *I=(TTreeIndex*)chain->GetTreeIndex(); // get the tree index
  Long64_t *index=NULL;
  if(I) {
    std::cout << "I=" << I << std::endl;
    index=I->GetIndex();
    std::cout << "index=" << index << std::endl << std::flush;
  }
  if(!index || !I) {
    std::cerr << "ERROR: TChain index returned a NULL pointer!" << std::endl;
    return;
  }
  
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    std::cout << index[iEntry] << std::endl;
    aBranch->GetEntry(iEntry);
    aBranchInfo->GetEntry(iEntry);
    //    aBranch->GetEntry(index[iEntry]);
    //    aBranchInfo->GetEntry(index[iEntry]);
  }
  */
}
