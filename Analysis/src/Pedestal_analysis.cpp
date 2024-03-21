#ifdef WITH_GET

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>

#include <TFile.h>
#include <TProfile.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/Pedestal_analysis.h"
#include "TPCReco/MultiKey.h"

#include "TPCReco/colorText.h"

///////////////////////////////
///////////////////////////////
Pedestal_analysis::Pedestal_analysis(EventSourceGRAW *aSource, std::string aOutputFileName)
{
  if(!aSource) {
    std::cout << __FUNCTION__ << ": Wrong EventSourceGRAW pointer!" << std::endl;
    exit(-1);
  }
  myEventSource = aSource;
  myOutputFileName = aOutputFileName;
  initialize();
}
///////////////////////////////
///////////////////////////////
Pedestal_analysis::~Pedestal_analysis(){
  finalize();
}
///////////////////////////////
///////////////////////////////
void Pedestal_analysis::initialize(){

  myOutputFilePtr = std::make_shared<TFile>(myOutputFileName.c_str(),"RECREATE");
  if(!myOutputFilePtr) {
    std::cout<<KRED<<"Pedestal_analysis::open: Cannot create new ROOT file: "<<RST<<myOutputFileName
	     <<std::endl;
    return;
  }
  myOutputFilePtr->cd();
  
  for(auto &it: myAbsPedestalPerRun) {
    if(it.second) delete it.second;
  }
  for(auto &it: myRelPedestalPerRun) {
    if(it.second) delete it.second;
  }
  for(auto &it: myNoisePerRun) {
    if(it.second) delete it.second;
  }
  for(auto &it: myFpnShapePerRun) {
    if(it.second) delete it.second;
  }
  myAbsPedestalPerRun.clear();
  myRelPedestalPerRun.clear();
  myNoisePerRun.clear();
  myFpnShapePerRun.clear();

  auto aGeometry = myEventSource->getGeometry();
  const auto nchan = aGeometry->GetAgetNchips()*aGeometry->GetAgetNchannels(); // 64 normal channels
  const auto ncell = aGeometry->GetAgetNtimecells(); // 512 time cells
  const auto maxval = 600; // typical pedestals are within 250-450 range for 12-bit AGETs
  for(int coboId = 0; coboId < aGeometry->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < aGeometry->GetAsadNboards(coboId); asadId++) {
      MultiKey2 mkey(coboId, asadId);
      if(myAbsPedestalPerRun.find(mkey)==myAbsPedestalPerRun.end()) {
	myAbsPedestalPerRun[mkey] = new TProfile(Form("prof_abs_pedestal_cobo%d_asad%d",
						      coboId, asadId), "Average pedestal from all time cells;Channel;ADC counts;Event count",
						 nchan, 0.0-0.5, 1.*nchan-0.5, 0.0, 1.*maxval);
      }
      if(myRelPedestalPerRun.find(mkey)==myRelPedestalPerRun.end()) {
	myRelPedestalPerRun[mkey] = new TProfile(Form("prof_rel_pedestal_cobo%d_asad%d",
						      coboId, asadId), "Pedestal relative to average FPN shape;Channel;Relative ADC counts;Event count",
						 nchan, 0.0-0.5, 1.*nchan-0.5, -1.*maxval, 1.*maxval);
      }
      if(myNoisePerRun.find(mkey)==myNoisePerRun.end()) {
	myNoisePerRun[mkey] = new TProfile(Form("prof_noise_cobo%d_asad%d",
						coboId, asadId), "RMS of noise relative to average FPN;Channel;ADC counts;Event count",
					   nchan, 0.0-0.5, 1.*nchan-0.5, 0.0, 1.*maxval);
      }
      for(int agetId = 0 ; agetId < aGeometry->GetAgetNchips(); agetId++) {
	MultiKey3 mkey3(coboId, asadId, agetId);
	if(myFpnShapePerRun.find(mkey3)==myFpnShapePerRun.end()) {
	  myFpnShapePerRun[mkey3] = new TProfile(Form("prof_average_FPN_cobo%d_asad%d_aget%d",
						      coboId, asadId, agetId), "Average FPN shape per AGET;Time cell;ADC counts",
						 ncell, 0.0-0.5, 1.*ncell-0.5, 0.0, 1.*maxval);
	}
      }
    }
  }
}
///////////////////////////////
///////////////////////////////
void Pedestal_analysis::finalize(){
  
  if(!myOutputFilePtr){
    std::cout<<KRED<<"Pedestal_analysis::close: "<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }
  myOutputFilePtr->cd();
  for(auto &it: myAbsPedestalPerRun) {
    it.second->Write("", TObject::kOverwrite);
  }
  for(auto &it: myRelPedestalPerRun) {
    it.second->Write("", TObject::kOverwrite);
  }
  for(auto &it: myNoisePerRun) {
    it.second->Write("", TObject::kOverwrite);
  }
  for(auto &it: myFpnShapePerRun) {
    it.second->Write("", TObject::kOverwrite);
  }
  myOutputFilePtr->Close();
}
///////////////////////////////
///////////////////////////////
void Pedestal_analysis::fillHistos(){

  auto aGeometry = myEventSource->getGeometry();
  const auto nchan = aGeometry->GetAgetNchips()*aGeometry->GetAgetNchannels(); // 64 normal channels
  auto aEventTPC=myEventSource->getCurrentEvent();
  for(int coboId = 0; coboId < aGeometry->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < aGeometry->GetAsadNboards(coboId); asadId++) {
      MultiKey2 mkey(coboId, asadId);
      auto pedestals = *(myEventSource->getPedestalProfilePerAsad(coboId, asadId));
      std::vector<double> relativePedestals(nchan, 0.0);
      for(int ibin = 1; ibin <= pedestals.GetXaxis()->GetNbins(); ibin++) {
	if(myRelPedestalPerRun.find(mkey)!=myRelPedestalPerRun.end()) {
	  myRelPedestalPerRun[mkey]->Fill( pedestals.GetBinCenter(ibin), pedestals.GetBinContent(ibin) );
	}
	if(myNoisePerRun.find(mkey)!=myNoisePerRun.end()) {
	  myNoisePerRun[mkey]->Fill( pedestals.GetBinCenter(ibin), pedestals.GetBinError(ibin) );
	}
	relativePedestals[(int)pedestals.GetBinCenter(ibin)] = pedestals.GetBinContent(ibin); // constant offset w.r.t. average FPN shape of given AGET
      }
      std::vector<double> averageFpnPerAget(aGeometry->GetAgetNchips(), 0.0);
      for(int agetId = 0 ; agetId < aGeometry->GetAgetNchips(); agetId++) {
	MultiKey3 mkey3(coboId, asadId, agetId);
	if(myFpnShapePerRun.find(mkey3)!=myFpnShapePerRun.end()) {
	  auto averageFPNshape = *( myEventSource->getFpnProfilePerAget(coboId, asadId, agetId) );
	  double averageFPNvalue=0.0;
	  int ncells_good=0;
	  for(int ibin = 1; ibin <= averageFPNshape.GetXaxis()->GetNbins(); ibin++) {
	    if(averageFPNshape.GetBinError(ibin)==0.0) continue; // skip empty time cells (e.g. when pedestal window is limited)
	    ncells_good++;
	    averageFPNvalue += averageFPNshape.GetBinContent(ibin);
	    myFpnShapePerRun[mkey3]->Fill( averageFPNshape.GetBinCenter(ibin), averageFPNshape.GetBinContent(ibin) );
	  }
	  if(ncells_good) averageFpnPerAget[agetId] = averageFPNvalue/ncells_good; // average value from all time cells of FPN shape of given AGET
	}
      }
      if(myAbsPedestalPerRun.find(mkey)!=myAbsPedestalPerRun.end()) {
	for(int ichan = 0; ichan < nchan; ichan++) {
	  int agetId = ichan / aGeometry->GetAgetNchannels(); // [0-3]
	  myAbsPedestalPerRun[mkey]->Fill(1.0*ichan, averageFpnPerAget[agetId] + relativePedestals[ichan]); // average total pedestal correction per channel
	}
      }
    }
  }
}
#endif