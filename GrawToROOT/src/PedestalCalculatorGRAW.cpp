// Pedestal Calculator GRAW
// Author: Artur Kalinowski
// Mon Jun 17 13:31:58 CEST 2019

#include <iostream>

#include "TPCReco/PedestalCalculatorGRAW.h"

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculatorGRAW::ResetTables(int coboId, int asadId){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START, Cobo=" << coboId
  //	    << ", Asad=" << asadId
  //	    << std::endl << std::flush;
  /////// DEBUG
  
    // reset pedestal TProfile histograms
  MultiKey2 mkey(coboId, asadId);
  auto it=prof_pedestal_map.find(mkey);
  if(it==prof_pedestal_map.end()) {
    std::cerr << __FUNCTION__
	      << " ERROR: wrong pair [Cobo=" << coboId
	      << ", Asad=" << asadId << "]!!!" << std::endl;
    return;
  }

  ////// DEBUG
  //  std::cout << __FUNCTION__ << " Resetting TProfile=" << it->second << std::endl;
  ////// DEBUG
  (it->second)->Reset();

  // reset averages and entries
  for (int agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId) {
    for(int cellId=minPedestalCell; cellId<=maxPedestalCell; ++cellId) {
	  
      /////// DEBUG
      //      std::cout << __FUNCTION__ << " Resetting FPN for: Cobo=" << coboId
      //		<< ", Asad=" << asadId
      //		<< ", Aget=" << agetId
      //		<< ", TimeCell=" << cellId
      //		<< std::endl << std::flush;
      /////// DEBUG
      
      FPN_entries_pedestal[coboId][asadId][agetId][cellId]=0; 
      FPN_ave_pedestal[coboId][asadId][agetId][cellId]=0.0;
    }
    for(int cellId=minSignalCell; cellId<=maxSignalCell; ++cellId) { 
      FPN_entries_signal[coboId][asadId][agetId][cellId]=0; 
      FPN_ave_signal[coboId][asadId][agetId][cellId]=0.0; 
    }
  }
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculatorGRAW::CalculateEventPedestals(const GET::GDataFrame & dataFrame){

  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START"
  //	    << std::endl << std::flush;
  /////// DEBUG

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
      for(Int_t ibin=1; ibin<=(it->second)->GetNbinsX(); ibin++) {
	pedestals[coboId][asadId].push_back( (it->second)->GetBinContent(ibin) ); //mean
      }
    }
  }
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
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculatorGRAW::ProcessDataFrame(const GET::GDataFrame &dataFrame, bool calculateMean){
  
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - START, calculateMean=" << calculateMean
  //	    << std::endl << std::flush;
  /////// DEBUG

  double rawVal = 0, corrVal = 0;
  int  COBO_idx = dataFrame.fHeader.fCoboIdx;
  int  ASAD_idx = dataFrame.fHeader.fAsadIdx;

  MultiKey2 mkey(COBO_idx, ASAD_idx);
  auto it=prof_pedestal_map.find(mkey);
  if(it==prof_pedestal_map.end()) {
    std::cerr << __FUNCTION__
	      << " ERROR: wrong pair [Cobo=" << COBO_idx
	      << ", Asad=" << ASAD_idx << "]!!!" << std::endl;
    return;
  }
  if(calculateMean) ResetTables(COBO_idx, ASAD_idx);
  
  // loop over AGET chips
  for (int agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    // FPN channels loop
    for (int channelId=0; calculateMean && channelId<myGeometryPtr->GetAgetNchannels_fpn(); ++channelId) {

      /////// DEBUG
      //      std::cout << __FUNCTION__ << " Calculating average FPN["
      //		<< " Cobo=" << COBO_idx
      //		<< ", Asad=" << ASAD_idx
      //		<< ", Aget=" << agetId
      //		<< ", FPN_index=" << channelId << "]"
      //		<< std::endl << std::flush;
      /////// DEBUG

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

		/////// DEBUG
		//		std::cout << __FUNCTION__ << " Filling TProfile: "
		//			  << "Cobo=" << COBO_idx
		//			  << ", Asad=" << ASAD_idx
		//			  << ", chan=" << asadChannelId
		//			  << ", corr_val=" << corrVal
		//			  << std::endl << std::flush;
		/////// DEBUG
		
	       
		(it->second)->Fill(asadChannelId, corrVal);
		/*
          // Beware HACK!!!
          //TProfile (prof_pedestal) with pedestals is only 256 (max chans in frame) long, pedestals are calculated for each frame and reset
          //to fit into TProfile the global number of first chan in COBO/ASAD has to be substracted from global channel
          int minChannelGlobal = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, 0, 0);
	        prof_pedestal->Fill(globalChannelId-minChannelGlobal, corrVal);
		*/
		
	      }
      }
    }// end of FPN loop    
  }// end of AGET loop  
  /////// DEBUG
  //  std::cout << __FUNCTION__ << " - END"
  //	    << std::endl << std::flush;
  /////// DEBUG
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

