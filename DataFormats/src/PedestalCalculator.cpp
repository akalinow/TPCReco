// Pedestal Calculator
// Author: Artur Kalinowski
// Mon Jun 17 13:31:58 CEST 2019

#include <iostream>

#include "PedestalCalculator.h"

PedestalCalculator::PedestalCalculator(){
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
PedestalCalculator::~PedestalCalculator(){
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::SetGeometryAndInitialize(std::shared_ptr<GeometryTPC> aPtr){

  myGeometryPtr = aPtr;

  std::cout<<__FUNCTION__<<" myGeometryPtr: "<<myGeometryPtr<<std::endl;

  InitializeTables();
  InitializeMonitoringHistos();
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::InitializeTables(){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START, myGeometryPtr=" << myGeometryPtr
  //	    << std::endl << std::flush;
  /////// DEBUG

  minSignalCell = 2;
  maxSignalCell = 500;

  minPedestalCell=10;
  maxPedestalCell=50;

  nchan = myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels(); // 64 normal channels
  maxval = 4096;          // 12-bit ADC
  nbin_spectrum = 100;    // Energy spectrum histograms

  /*
  FPN_entries_pedestal.assign(myGeometryPtr->GetAgetNchips(), std::vector<uint>(512));
  FPN_ave_pedestal.assign(myGeometryPtr->GetAgetNchips(), std::vector<double>(512));
  */
  std::vector< std::vector<uint> >
    v2_entries(myGeometryPtr->GetAgetNchips(),
	       std::vector<uint>(myGeometryPtr->GetAgetNtimecells(),0));
  std::vector< std::vector<double> >
    v2_average(myGeometryPtr->GetAgetNchips(),
	       std::vector<double>(myGeometryPtr->GetAgetNtimecells(),0.0));
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " Ncobos=" << myGeometryPtr->GetCoboNboards()
  //	    << std::endl << std::flush;
  /////// DEBUG

  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    /////// DEBUG
    //    std::cout << __FUNCTION__ << " Cobo=" << coboId << ": Nasads="
    //	      << myGeometryPtr->GetAsadNboards(coboId)
    //	      << std::endl << std::flush;
    /////// DEBUG
    std::vector< std::vector< std::vector<uint> > >
      v_entries(myGeometryPtr->GetAsadNboards(coboId));
    std::vector< std::vector< std::vector<double> > >
      v_average(myGeometryPtr->GetAsadNboards(coboId));

    FPN_entries_pedestal.push_back(v_entries);
    FPN_ave_pedestal.push_back(v_average);
    for(int asadId = 0; asadId < myGeometryPtr->GetAsadNboards(coboId); asadId++) {
      ////// DEBUG
      //      std::cout << __FUNCTION__ << " BEFORE: Size of FPN[Cobo=" << coboId
      //		<< "]=" << FPN_entries_pedestal[coboId].size()
      //		<< std::endl << std::flush;
      ////// DEBUG

      FPN_entries_pedestal.at(coboId).at(asadId)=v2_entries;
      FPN_ave_pedestal.at(coboId).at(asadId)=v2_average;

      ////// DEBUG
      //      std::cout << __FUNCTION__ << " AFTER: Pointer FPN[Cobo=" << coboId
      //		<< "]=" << &FPN_entries_pedestal.at(coboId)
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Pointer FPN[Cobo=" << coboId
      //		<< ", Asad=" << asadId
      //		<< "]=" << &FPN_entries_pedestal.at(coboId).at(asadId)
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Pointer V2[Aget=3, TimeCell=511]="
      //		<< &v2_entries.at(3).at(511)
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Size of FPN="
      //		<< FPN_entries_pedestal.size()
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Size of FPN[Cobo=" << coboId
      //		<< "]=" << (FPN_entries_pedestal[coboId]).size()
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Pointer FPN[Cobo=" << coboId
      //		<< ", Asad=" << asadId
      //		<< ", Aget=0]=" << &((FPN_entries_pedestal.at(coboId)).at(asadId)).at(0)
      //		<< std::endl << std::flush;
      //      std::cout << __FUNCTION__ << " Ponter FPN[Cobo=" << coboId
      //		<< ", Asad=" << asadId
      //		<< ", Aget=0, TimeCell=0]=" << &FPN_entries_pedestal.at(coboId).at(asadId).at(0).at(0)
      //		<< std::endl << std::flush;
      ////// DEBUG
    }
  }
  FPN_entries_signal =  FPN_entries_pedestal;
  FPN_ave_signal = FPN_ave_pedestal;

  // Initialize pedestals array (per ASAD board)  
  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    std::vector< std::vector<double> >
      v3_average(myGeometryPtr->GetAsadNboards(coboId),
		   std::vector<double>(myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels(), 0.0));
    pedestals.push_back(v3_average);
  }

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::ResetTables(){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START, myGeometryPtr="
  //	    << myGeometryPtr << std::endl << std::flush;
  /////// DEBUG
  
    // reset pedestal TProfile histograms
  for(auto it=prof_pedestal_map.begin(); it!=prof_pedestal_map.end(); it++) {
    ////// DEBUG
    //    std::cout << __FUNCTION__ << " Resetting TProfile=" << it->second << std::endl;
    ////// DEBUG
    (it->second)->Reset();
  }

  // reset averages and entries
  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < myGeometryPtr->GetAsadNboards(coboId); asadId++) {
      for (int agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId) {
	for(int cellId=minPedestalCell; cellId<=maxPedestalCell; ++cellId) {
	  
	  /////// DEBUG
	  //	  std::cout << __FUNCTION__ << " Resetting FPN for: Cobo=" << coboId
	  //		    << ", Asad=" << asadId
	  //		    << ", Aget=" << agetId
	  //		    << ", TimeCell=" << cellId
	  //		    << std::endl << std::flush;
	  /////// DEBUG
	  
	  FPN_entries_pedestal[coboId][asadId][agetId][cellId]=0; 
	  FPN_ave_pedestal[coboId][asadId][agetId][cellId]=0.0;
	}
	for(int cellId=minSignalCell; cellId<=maxSignalCell; ++cellId) { 
	  FPN_entries_signal[coboId][asadId][agetId][cellId]=0; 
	  FPN_ave_signal[coboId][asadId][agetId][cellId]=0.0; 
	}
      }
    }
  }
  
  /*
  prof_pedestal->Reset();

  for (int agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    for(int cellId=minPedestalCell; cellId<=maxPedestalCell; ++cellId) { 
      FPN_entries_pedestal[agetId][cellId]=0; 
      FPN_ave_pedestal[agetId][cellId]=0.0;
    }
    for(int cellId=minSignalCell; cellId<=maxSignalCell; ++cellId) { 
      FPN_entries_signal[agetId][cellId]=0; 
      FPN_ave_signal[agetId][cellId]=0.0; 
    }
  }
  */
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::InitializeMonitoringHistos(){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START"
  //	    << std::endl << std::flush;
  /////// DEBUG
  /*
    prof_pedestal = new TProfile("prof_pedestal", "Pedestal relative to average FPN (err=RMS, all events);Channel;ADC counts",
    nchan, 0.0-0.5, 1.*nchan-0.5, -1.*maxval, 1.*maxval, "S"); // error = RMS of Y value
  */
  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < myGeometryPtr->GetAsadNboards(coboId); asadId++) {
      MultiKey2 mkey(coboId, asadId);
      if(prof_pedestal_map.find(mkey)==prof_pedestal_map.end()) {
	prof_pedestal_map[mkey] = new TProfile(Form("prof_pedestal_cobo%d_asad%d",
						      coboId, asadId), "Pedestal relative to average FPN (err=RMS, all events);Channel;ADC counts",
			       nchan, 0.0-0.5, 1.*nchan-0.5, -1.*maxval, 1.*maxval, "S"); // error = RMS of Y value
      }
    }
  }
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
/*
double PedestalCalculator::GetPedestalCorrection(int iChannelGlobal, int agetId, int iCell){

  //  double pedestal = pedestals.at(iChannelGlobal);
  double pedestal = pedestals[.at(iChannelGlobal);
  double average = FPN_ave_signal[agetId][iCell];
  
  double correction = pedestal + average;
  return correction;
}
*/
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
double PedestalCalculator::GetPedestalCorrection(int coboId, int asadId, int agetId, int chanId, int iCell){

  double pedestal = pedestals[coboId][asadId].at(myGeometryPtr->Asad_normal2normal(agetId, chanId));
  double average = FPN_ave_signal[coboId][asadId][agetId][iCell];
  
  double correction = pedestal + average;
  return correction;
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
/*
void PedestalCalculator::CalculateEventPedestals(const GET::GDataFrame & dataFrame){

  bool calculateMean = true;
  ProcessDataFrame(dataFrame, calculateMean);

  calculateMean = false;
  ProcessDataFrame(dataFrame, calculateMean);

  // update vector with pedestals
  //  pedestals.clear();
  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < myGeometryPtr->GetAsadNboards(coboId); asadId++) {
      MultiKey2 mkey(coboId, asadId);
      auto it=prof_pedestal_map.find(mkey);
      if(it==prof_pedestal_map.end()) continue;
      pedestals[coboId][asadId].clear();
      for(Int_t ibin=1; ibin<=(*it)->GetNbinsX(); ibin++) {
	pedestals[coboId][asadId].push_back( (*it)->GetBinContent(ibin) ); //mean
      }
    }
  }
*/
  /*
  for(Int_t ibin=1; ibin<=prof_pedestal->GetNbinsX(); ibin++) {
    double mean=prof_pedestal->GetBinContent(ibin);
    //double rms=prof_pedestal->GetBinError(ibin);
    pedestals.push_back(mean);
  }
  if ((int)pedestals.size()!=myGeometryPtr->GetAgetNchannels()*myGeometryPtr->GetAgetNchips()) {
    std::cerr << "ERROR: wrong size of pedestal vector!!!" << std::endl;
    //return false;
  }    
  */
/*}*/
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::CalculateEventPedestals(const std::shared_ptr<eventraw::EventRaw> eRaw){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START"
  //	    << std::endl << std::flush;
  /////// DEBUG

  bool calculateMean = true;
  ProcessEventRaw(eRaw, calculateMean);

  calculateMean = false;
  ProcessEventRaw(eRaw, calculateMean);


  // update vector with pedestals
  for(int coboId = 0; coboId < myGeometryPtr->GetCoboNboards(); coboId++) {
    for(int asadId = 0; asadId < myGeometryPtr->GetAsadNboards(coboId); asadId++) {
      MultiKey2 mkey(coboId, asadId);
      auto it=prof_pedestal_map.find(mkey);
      if(it==prof_pedestal_map.end()) continue;
      pedestals[coboId][asadId].clear();
      for(Int_t ibin=1; ibin<=(it->second)->GetNbinsX(); ibin++) {
	pedestals[coboId][asadId].push_back( (it->second)->GetBinContent(ibin) ); //mean
      }
    }
  }
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
/*
void PedestalCalculator::ProcessDataFrame(const GET::GDataFrame &dataFrame, bool calculateMean){
  
  double rawVal = 0, corrVal = 0;
  int  COBO_idx = dataFrame.fHeader.fCoboIdx;
  int  ASAD_idx = dataFrame.fHeader.fAsadIdx;
  if(calculateMean) ResetTables();
  
  MultiKey2 mkey(COBO_idx, ASAD_idx);
  auto it=prof_pedestal_map.find(mkey);
  if(it==prof_pedestal_map.end()) {
    std::cerr << __FUNCTION__
	      << " ERROR: wrong pair [CoboId=" << COBO_idx
	      << ", AsadId=" << ASAD_idx << "]!!!" << std::endl;
    return;
  }

  // loop over AGET chips
  for (int agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    // FPN channels loop
    for (int channelId=0; calculateMean && channelId<myGeometryPtr->GetAgetNchannels_fpn(); ++channelId) {

      GET::GDataChannel* channel = const_cast<GET::GDataFrame &>(dataFrame).SearchChannel(agetId, myGeometryPtr->Aget_fpn2raw(channelId));
      if (!channel) continue;

      for (int aSample = 0; aSample < channel->fNsamples; ++aSample){
	      GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(aSample);	
	      int cellId = sample->fBuckIdx;
	      if(cellId<2 || cellId>509) continue;
	      //Pedestal time-window
	      if(cellId>=minPedestalCell && cellId<=maxPedestalCell){	 
	        FPN_ave_pedestal[COBO_idx][ASAD_idx][agetId][cellId] += sample->fValue;
	        FPN_entries_pedestal[COBO_idx][ASAD_idx][agetId][cellId] ++;
	      }
	      //Signal time window
	      if(cellId>=minSignalCell && cellId<=maxSignalCell){
	        FPN_ave_signal[COBO_idx][ASAD_idx][agetId][cellId] += sample->fValue;
	        FPN_entries_signal[COBO_idx][ASAD_idx][agetId][cellId] ++;
	      }
      }
    }// end of FPN loop

    // calculate average FPN profile from 4 channels (pedestal time-window only)
    for (int cellId=minPedestalCell; calculateMean && cellId<=maxPedestalCell; ++cellId) {
      if(FPN_entries_pedestal[COBO_idx][ASAD_idx][agetId][cellId]>0) FPN_ave_pedestal[COBO_idx][ASAD_idx][agetId][cellId] /= (double)FPN_entries_pedestal[COBO_idx][ASAD_idx][agetId][cellId];
    }
    for (int cellId=minSignalCell; calculateMean && cellId<=maxSignalCell; ++cellId) {
      if(FPN_entries_signal[COBO_idx][ASAD_idx][agetId][cellId]>0) FPN_ave_signal[COBO_idx][ASAD_idx][agetId][cellId] /= (double)FPN_entries_signal[COBO_idx][ASAD_idx][agetId][cellId];    
    }

    ///Loop over normal channels
    for (int channelId=0; !calculateMean && channelId<myGeometryPtr->GetAgetNchannels(); ++channelId) {

      GET::GDataChannel* channel = const_cast<GET::GDataFrame &>(dataFrame).SearchChannel(agetId, myGeometryPtr->Aget_normal2raw(channelId));
      if (!channel) continue;

      for (int aSample = 0; aSample < channel->fNsamples; ++aSample){
	      GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(aSample);	
	      int cellId = sample->fBuckIdx;
	      if(cellId<2 || cellId>509) continue;
	      //Pedestal time-window
	      if(cellId>=minPedestalCell && cellId<=maxPedestalCell){
	        rawVal  = sample->fValue;
	        corrVal = rawVal - FPN_ave_pedestal[COBO_idx][ASAD_idx][agetId][cellId];
	        int asadChannelId = myGeometryPtr->Asad_normal2normal(agetId, channelId);// 0-255 (without FPN)
		prof_pedestal.at(mkey)->Fill(asadChannelId, corrVal);
*/		/*
          // Beware HACK!!!
          //TProfile (prof_pedestal) with pedestals is only 256 (max chans in frame) long, pedestals are calculated for each frame and reset
          //to fit into TProfile the global number of first chan in COBO/ASAD has to be substracted from global channel
          int minChannelGlobal = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, 0, 0);
	        prof_pedestal->Fill(globalChannelId-minChannelGlobal, corrVal);
		*/
/*		
	      }
      }
    }// end of FPN loop    
  }// end of AGET loop  
}
*/
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::ProcessEventRaw(const std::shared_ptr<eventraw::EventRaw> eRaw, bool calculateMean){
  
  //  double rawVal = 0, corrVal = 0;
  //  int  COBO_idx = dataFrame.fHeader.fCoboIdx;
  //  int  ASAD_idx = dataFrame.fHeader.fAsadIdx;
  if(calculateMean) ResetTables();
  
  // loop over AGET chips
  eventraw::AgetRawMap_t::iterator a_it;
  for(a_it=(eRaw->data).begin() ; calculateMean && a_it!=(eRaw->data).end() ; a_it++) {
    // TODO
  }

  // TODO
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
std::shared_ptr<TProfile> PedestalCalculator::GetPedestalProfilePerAsad(int coboId, int asadId){
  MultiKey2 mkey(coboId, asadId);
  auto it=prof_pedestal_map.find(mkey);
  if(it==prof_pedestal_map.end()) return std::shared_ptr<TProfile>();
  std::shared_ptr<TProfile> result( (TProfile*)((it->second)->Clone()) );

  ////// DEBUG
  std::cout << __FUNCTION__
	    << " iteratorPtr=" << it->second
	    << " result=" << result
    	    << " result.get()=" << result.get()
	    << std::endl << std::flush;
  ////// DEBUG
  return result;
}

