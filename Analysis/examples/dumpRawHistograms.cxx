/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// Example ROOT macro for dumping raw data into three TH2D projections (U-Z, V-Z, W-Z) in DET coordinates [mm].
// Raw hit clustering and/or pedestal subtraction can be turned on/off.
// Compatible with: GRAW, multi-GRAW and ROOT input files.
//
// root[0] .L dumpRawHistograms.cxx
//
// # TPC working point: CO2 130 mbar, 25MHz sampling, 1764V drift / Gamma beam energy: 9.845 MeV: 
// root[1] loop_events("/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd0_2022-08-22T22:03:11.776_0000.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd1_2022-08-22T22:03:11.793_0000.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd2_2022-08-22T22:03:11.796_0000.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd3_2022-08-22T22:03:11.798_0000.graw", 0, 100+0*3852, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", 50000.0, 500., false, true);
//
// # TPC working point: CO2 190 mbar, 25MHz sampling, 3332V drift / Gamma beam energy: 11.5 MeV: 
// root[2] loop_events("/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd0_2022-04-12T06:47:52.487_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd1_2022-04-12T06:47:52.489_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd2_2022-04-12T06:47:52.492_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd3_2022-04-12T06:47:52.494_0000.graw", 3852-20, 3852, "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", 50000.0, 500., false, true);
//
// # TPC working point: CO2 190 mbar, 25MHz sampling, 3920V drift / Gamma beam energy: 13.1 MeV: 
// root[2] loop_events("/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd0_2022-04-12T20:05:29.462_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd1_2022-04-12T20:05:29.464_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd2_2022-04-12T20:05:29.467_0000.graw,/mnt/NAS_STORAGE/20220412_extTrg_CO2_190mbar_DT1470ET/CoBo0_AsAd3_2022-04-12T20:05:29.470_0000.graw", 3852-20, 3852, "geometry_ELITPC_190mbar_3920Vdrift_25MHz.dat", 50000.0, 500., false, true);
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#define WITH_GET // temporary HACK - TO BE REPLACED WITH PROPER CMAKE FLAG

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../GrawToROOT/include)
R__ADD_INCLUDE_PATH(../../EventSources/include)
R__ADD_INCLUDE_PATH($GET_DIR/GetSoftware_bin/$GET_RELEASE/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <cstdlib>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include <TMath.h>
#include <TFile.h>
#include <TString.h>
#include <TH2D.h>
#include <TVector3.h>
#include <TTree.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TMarker.h>
#include <TLine.h>
#include <TLatex.h>

#include "TPCReco/colorText.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/EventSourceBase.h"
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/EventSourceROOT.h"
#include "TPCReco/UtilsMath.h"
#include "TPCReco/RunIdParser.h"

#define DRAW_SAME_SCALE false // plot UVW projection using the same scale of dE/dx for all strip directions
#define DRAW_SAME_SCALE_MARGIN 0.05 // add +/- 5% margin for common dE/dx scale
#define DRAW_PAD_FILL_COLOR kAzure-6 // matches default kBird palette
#define DRAW_LOG_SCALE  false // use LOG scale for dE/dx axis (NOTE: differential plots will still use LIN scale)

using namespace ROOT::Math;
class GeometryTPC;

// _______________________________________
//
// Adds arbitrary comment using NDC coordinates of the pad.
//
void DrawLatexNDC(TPad *tpad, const double xNDC, const double yNDC, const char *comment="") {
  if(!tpad) return;
  const auto text_color=kBlue;
  const auto text_font=42; // Arial normal, precision=2 (scalable, size in NDC)
  const auto text_size=0.025; // size in NDC
  tpad->cd();
  auto *t=new TLatex(xNDC, yNDC, comment);
  t->SetTextAlign(11); // horizontal=left, vertical=bottom
  t->SetNDC(true);
  t->SetTextFont(text_font);
  t->SetTextColor(text_color);
  t->SetTextSize(text_size);
  t->Draw();
  t->ResetBit(kCannotPick); // prevents editing
  tpad->Modified();
}
// _______________________________________
//
// Divide existing TCanvas into multiple TPads to visualize raw data separately for each U/V/W projection.
//
void DrawHistos(TCanvas *tcanvas, // input TCanvas
		std::vector<std::shared_ptr<TH2D> > *refHistogramsInMM) { // vector of RAW histograms
if(!tcanvas || !refHistogramsInMM || refHistogramsInMM->size()>3) return;
  const auto pad_width=500;
  const auto pad_right_margin=0.15;
  const auto pad_left_margin=0.15;
  const auto npadx=refHistogramsInMM->size();
  const auto npady=1U;
  tcanvas->Clear();
  tcanvas->SetWindowSize(pad_width*npadx, pad_width*npady);
  tcanvas->Divide(npadx,npady,0.01,0.04);

  int idir;

#if(DRAW_SAME_SCALE)
  // first pass to set common Z rangef
  idir=definitions::projection_type::DIR_U;
  auto ref_min=1E30;
  auto ref_max=-1E30;
  for(auto &it: *refHistogramsInMM) {
    ref_min=std::min(ref_min, it->GetBinContent(it->GetMinimumBin())); // true MIN of histogram
    ref_max=std::max(ref_max, it->GetBinContent(it->GetMaximumBin())); // true MAX of histogram
    idir++;
  }
  for(auto &it: *refHistogramsInMM) {
    it->SetMinimum( ref_min-DRAW_SAME_SCALE_MARGIN*(ref_max-ref_min) );
    it->SetMaximum( ref_max+DRAW_SAME_SCALE_MARGIN*(ref_max-ref_min) );
  }
#endif
  // draw reference histograms
  idir=definitions::projection_type::DIR_U;
  auto ipad=1;
  for(auto &it: *refHistogramsInMM) {
    tcanvas->cd(ipad);
    if(gPad) {
      gPad->SetFrameFillColor(DRAW_PAD_FILL_COLOR); // show empty bins in ligh blue (matches standard kBird palette)
      gPad->SetLogx(false);
      gPad->SetLogy(false);
      gPad->SetLogz(DRAW_LOG_SCALE);
      gPad->SetLeftMargin(pad_left_margin);
      gPad->SetRightMargin(pad_right_margin);
    }
    auto h=(TH2D*)(it->DrawClone("COLZ"));
    h->SetDirectory(0);
    h->SetStats(false);
    h->SetName(Form("rawdata_%s",h->GetName()));
    h->SetTitle(Form("%s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
    h->GetYaxis()->SetTitleOffset(1.4);
    h->GetZaxis()->SetTitleOffset(1.6);
    ipad++;
    idir++;
  }
  tcanvas->Update();
  tcanvas->Modified();
}

// _______________________________________
//
// Dumps PEventTPC raw-data deposits from GRAW file.
// Pedestal calculation and clustering settings have to be adjusted for the real-data and the Monte Carlo cases.
//
int loop_events(const char *rawInputFile, // single ROOT file, or single GRAW, or comma-separated list of GRAW files
		unsigned long firstEvent=0,        // first eventId to process
		unsigned long lastEvent=0,         // last eventId to process (0 = up to the end)
		const char *geometryFile="geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat",
		double totalCharge_cut=1000.0, // event pre-filtering according to total integrated charge [ADC counts]
		double maxCharge_cut=100.0, // event pre-filtering according to maximal charge per U/V/W projection [ADC counts]
		bool flag_clustering=false,  // enable displaying clustered RAW data
		bool flag_subtract_pedestals=true  // enable pedestal subtraction in case of GRAW files
		) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("EventSourceGRAW")){
    R__LOAD_LIBRARY(libTPCGrawToROOT.so);
  }
  if (!gROOT->GetClass("EventSourceROOT")){
    R__LOAD_LIBRARY(libTPCEventSources.so);
  }

  /////////// set parameters of EventTPC clustering
  //
  filter_type filterType = (flag_clustering ? filter_type::threshold : filter_type::none);
  //
  // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
  //       Below is a workaround employing simple node.
  boost::property_tree::ptree pedestalConfig, hitFilterConfig;
  hitFilterConfig.put("hitFilter.recoClusterEnable", flag_clustering);
  hitFilterConfig.put("hitFilter.recoClusterThreshold", 35.0); // [ADC units] - seed hits
  hitFilterConfig.put("hitFilter.recoClusterConstantFractionThreshold", 0.1); // 10%
  hitFilterConfig.put("hitFilter.recoClusterDeltaStrips", 2); // band around seed hit in strip units
  hitFilterConfig.put("hitFilter.recoClusterDeltaTimeCells", 5); // band around seed hit in time cells
  pedestalConfig.put("remove", flag_subtract_pedestals);
  pedestalConfig.put("minPedestalCell", 5);
  pedestalConfig.put("maxPedestalCell", 25);
  pedestalConfig.put("minSignalCell", 5);
  pedestalConfig.put("maxSignalCell", 506);

  /////////// initialize TPC geometry, electronic parameters and gas conditions
  //
  auto aGeometry = std::make_shared<GeometryTPC>(geometryFile, false);
  //  aGeometry->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  ////////// initialize EventSource
  //
  std::shared_ptr<EventSourceBase> myEventSource;
  if(std::string(rawInputFile).find(".graw")!=std::string::npos){
#ifdef WITH_GET
   const char del = ','; // delimiter character
    std::set<std::string> fileNameList; // list of unique strings
    std::stringstream sstream(rawInputFile);
    std::string fileName;
    while (std::getline(sstream, fileName, del)) {
      if(fileName.size()>0) fileNameList.insert(fileName);
    };
    const int frameLoadRange=150; // important for single GRAW file mode
    const unsigned int AsadNboards=aGeometry->GetAsadNboards();
    if(fileNameList.size()==AsadNboards) {
      myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFile);
    } else if (fileNameList.size()==1) {
      myEventSource = std::make_shared<EventSourceGRAW>(geometryFile);
      dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(frameLoadRange);
    } else {
      std::cerr << __FUNCTION__ << KRED << ": Invalid number of GRAW files!" << RST << std::endl << std::flush;
      return 1;
    }

    // initialize pedestal removal parameters for EventSource
    // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
    //       Below is a workaround employing simple node.
    // if(aConfig.find("pedestal")!=aConfig.not_found()) {
    //   dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(false);
    //   dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(pedestalConfig);
    //   dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(aConfig.find("pedestal")->second);
    // }
    // else {
    //   std::cerr << __FUNCTION__ << KRED << ": Some pedestal configuration options are missing!" << RST << std::endl << std::flush;
    //   return 1;
    // }
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(pedestalConfig);
#else
    std::cerr << __FUNCTION__ << KRED << ": Program compiled without GET libraries!" << RST << std::endl << std::flush;
    return 1;
#endif
  } else if(std::string(rawInputFile).find(".root")!=std::string::npos){
    myEventSource = std::make_shared<EventSourceROOT>(geometryFile);
  } else {
    std::cerr << __FUNCTION__ << KRED << ": Invalid raw-data input file: " << RST << rawInputFile << std::endl << std::flush;
    return 1;
  }
  myEventSource->loadDataFile(rawInputFile);
  auto nEntries = myEventSource->numberOfEntries();
  std::cout << "File with " << nEntries << " frames loaded."
	    << std::endl;
  myEventSource->loadFileEntry(0); // load 1st frame (NOTE: otherwise LoadEventId does not work)
  
  ////////// initialize output ROOT file with TCanvas
  //
  std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
  const std::string rootFileNameCanvas = "raw_uvw_histos.root";
  TFile *outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
  outputCanvasROOTFile->cd();
  TCanvas *outputCanvas=new TCanvas("c_result", "c_result", 500, 500);

  long lastEventId = firstEvent-1;
  for(long eventId=firstEvent; eventId<=lastEvent; eventId++) {

    // load EventTPC to be displayed from the RAW file    
    myEventSource->loadEventId(eventId);
    auto aEventTPC = myEventSource->getCurrentEvent();
    long currentEventId = aEventTPC->GetEventInfo().GetEventId();
    long currentRunId = aEventTPC->GetEventInfo().GetRunId();
    if(currentEventId>lastEvent && currentEventId==lastEvent) break;
    auto totalCharge = aEventTPC->GetTotalCharge(); // ADC counts
    auto maxCharge = aEventTPC->GetMaxCharge(); // ADC counts
    std::cout << "#####################" << std::endl
	      << "#### EVENT PLAYER:  RUN=" << currentRunId << ", EVENT=" << currentEventId << ", Max.charge=" << maxCharge << ", Tot.charge=" << totalCharge << std::endl
	      << "#####################" << std::endl;
    std::cout << aEventTPC->GetEventInfo() << std::endl;

    // prepare UVW projections to be displayed
    aEventTPC->setHitFilterConfig(filterType, hitFilterConfig); // hit clustering (if any)
    if(totalCharge<totalCharge_cut) {
      std::cout << "EVENT SKIPPED: Total charge " << totalCharge << " is below limit (" << totalCharge_cut << ")" <<  std::endl;
      continue;
    }
    if(maxCharge<maxCharge_cut) {
      std::cout << "EVENT SKIPPED: Maximal charge " << maxCharge << " is below limit (" << maxCharge_cut << ")" <<  std::endl;
      continue;
    }
    //    std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir] = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
      referenceHistosInMM[strip_dir]->SetTitle(Form("hrawMM_run%ld_evt%ld_%sstrip", currentRunId, currentEventId, aGeometry->GetDirName(strip_dir)));
    }

    //////// display UVW histograms
    //
    outputCanvasROOTFile->cd();
    DrawHistos(outputCanvas, &referenceHistosInMM);
    outputCanvas->SetName(Form("c_run%ld_evt%ld", currentRunId, currentEventId));
    outputCanvas->SetTitle(outputCanvas->GetName());
    DrawLatexNDC(outputCanvas, 0.01, 0.01, Form("Max. charge: %4.0lf, Tot. charge: %.4le", maxCharge, totalCharge));
    outputCanvas->Write();

    //////// discard UVW projectons that are not needed anymore
    //
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir].reset();
    }
  } // end of main processing loop

  outputCanvasROOTFile->Close();

  return 0;
}
