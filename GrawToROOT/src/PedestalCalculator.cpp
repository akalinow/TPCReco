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

  InitializeTables();
  InitializeMonitoringHistos();
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::InitializeTables(){

  minSignalCell = 2;
  maxSignalCell = 500;

  minPedestalCell=10;
  maxPedestalCell=50;

  nchan = myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels(); // 64 normal channels
  maxval = 4096;          // 12-bit ADC
  nbin_spectrum = 100;    // Energy spectrum histograms

  FPN_entries_pedestal.assign(myGeometryPtr->GetAgetNchips(), std::vector<uint>(512));
  FPN_ave_pedestal.assign(myGeometryPtr->GetAgetNchips(), std::vector<double>(512));

  FPN_entries_signal =  FPN_entries_pedestal;
  FPN_ave_signal = FPN_ave_pedestal;
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::ResetTables(){

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
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::InitializeMonitoringHistos(){
  
  prof_pedestal = new TProfile("prof_pedestal", "Pedestal relative to average FPN (err=RMS, all events);Channel;ADC counts",
			       nchan, 0.0-0.5, 1.*nchan-0.5, -1.*maxval, 1.*maxval, "S"); // error = RMS of Y value
                                                              
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
double PedestalCalculator::GetPedestalCorrection(int iChannelGlobal, int agetId, int iCell){

  double pedestal = pedestals.at(iChannelGlobal);
  double average = FPN_ave_signal[agetId][iCell];
  
  double correction = pedestal + average;
  return correction;
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::CalculateEventPedestals(const GET::GDataFrame & dataFrame){

  bool calculateMean = true;
  ProcessDataFrame(dataFrame, calculateMean);

  calculateMean = false;
  ProcessDataFrame(dataFrame, calculateMean);

  // update vector with pedestals
  pedestals.clear();
  for(Int_t ibin=1; ibin<=prof_pedestal->GetNbinsX(); ibin++) {
    double mean=prof_pedestal->GetBinContent(ibin);
    //double rms=prof_pedestal->GetBinError(ibin);
    pedestals.push_back(mean);
  }
  if ((int)pedestals.size()!=myGeometryPtr->GetAgetNchannels()*myGeometryPtr->GetAgetNchips()) {
    std::cerr << "ERROR: wrong size of pedestal vector!!!" << std::endl;
    //return false;
  }    
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
void PedestalCalculator::ProcessDataFrame(const GET::GDataFrame &dataFrame, bool calculateMean){
  
  double rawVal = 0, corrVal = 0;
  int  COBO_idx = dataFrame.fHeader.fCoboIdx;
  int  ASAD_idx = dataFrame.fHeader.fAsadIdx;
  if(calculateMean) ResetTables();
  
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
	        FPN_ave_pedestal[agetId][cellId] += sample->fValue;
	        FPN_entries_pedestal[agetId][cellId] ++;
	      }
	      //Signal time window
	      if(cellId>=minSignalCell && cellId<=maxSignalCell){
	        FPN_ave_signal[agetId][cellId] += sample->fValue;
	        FPN_entries_signal[agetId][cellId] ++;
	      }
      }
    }// end of FPN loop

    // calculate average FPN profile from 4 channels (pedestal time-window only)
    for (int cellId=minPedestalCell; calculateMean && cellId<=maxPedestalCell; ++cellId) {
      if(FPN_entries_pedestal[agetId][cellId]>0) FPN_ave_pedestal[agetId][cellId] /= (double)FPN_entries_pedestal[agetId][cellId];
    }
    for (int cellId=minSignalCell; calculateMean && cellId<=maxSignalCell; ++cellId) {
      if(FPN_entries_signal[agetId][cellId]>0) FPN_ave_signal[agetId][cellId] /= (double)FPN_entries_signal[agetId][cellId];    
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
	        corrVal = rawVal - FPN_ave_pedestal[agetId][cellId];
	        int globalChannelId = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, agetId, channelId);// 0-255 (without FPN)
          // Beware HACK!!!
          //TProfile (prof_pedestal) with pedestals is only 256 (max chans in frame) long, pedestals are calculated for each frame and reset
          //to fit into TProfile the global number of first chan in COBO/ASAD has to be substracted from global channel
          int minChannelGlobal = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, 0, 0);
	        prof_pedestal->Fill(globalChannelId-minChannelGlobal, corrVal);
	      }
      }
    }// end of FPN loop    
  }// end of AGET loop  
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

