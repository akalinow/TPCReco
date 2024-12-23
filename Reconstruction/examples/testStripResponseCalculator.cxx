//////////////////////////
//
// Example ROOT macros that use StripResponseCalculator to:
// * prepare strip response matrix for a given diffusion hypothesis and TPC working point.
// * create PEventTPC collection out of user-supplied Track3D collection (e.g. Toy MC).
//
// Mikolaj Cwiok (UW) - 27 May 2023
//
//
// root
// root [0] .L ../examples/testStripResponseCalculator.cxx
// root [1] testGenerateResponse();
// root [2] plotStripResponse(1.0);
// root [3] plotTimeResponse(1.0);
// root [4] testResponse1(1.0);
// root [5] testResponse2();
// root [6] testResponse3();
// root [7] generateResponse(0.85, 0.86, 232, 1000000L, 1000000L, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat");
// root [8] testResponse4("Reco_FakeEvents.root", 10, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", 130, 273.15+20, 0.85, 0.86, 232);
//
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <TFile.h>
#include <TTree.h>
#include <TTreeIndex.h>

//#define DEBUG

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/StripResponseCalculator.h"

//////////////////////////
//////////////////////////
//
// Generates strip response histograms and saves them to a separate ROOT file.
// NOTE: Already existing files will be preserved.
//
void generateResponse(double sigmaXY=1, // [mm] - effective diffusion along XY_DET
		      double sigmaZ=-1, // [mm] - effective diffusion along Z_DET
		      double peakingTime=-1, // [ns] - GET electronics peaking time
		      long npointsSpace=1000000, // use: >=10M to get smooth response distributions
		      long npointsTime=100000,   // use: >=10M to get smooth response distributions
		      const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		      int nbinsSpace=10, // use: >=50 for fitting purposes
		      int nbinsTime=20,  // use: >=50 for fitting purposes
		      int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		      int npads=14,  // use: (nstrips*2)
		      int ncells=50  // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
		      ) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  geo->SetTH2PolyPartition(3*200,2*200); // higher TH2Poly granularity speeds up initialization of XY response histograms!
  if(sigmaZ<0) sigmaZ=sigmaXY; // when sigmaZ is omitted assume that sigmaZ=sigmaXY
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime);
  const std::string resultFile( (peakingTime==0 ?
				 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				      sigmaXY, sigmaZ) :
				 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				      sigmaXY, sigmaZ, peakingTime) ) );
  if(calc->loadHistograms(resultFile.c_str())) {
    std::cout << "ERROR: Response histogram file " << resultFile << " already exists!" << std::endl;
    return;
  }
  calc->setDebug(true);
  calc->initializeStripResponse(npointsSpace, nbinsSpace); // overwrite default value
  calc->initializeTimeResponse(npointsTime, nbinsTime); // overwrite default value
  std::cout << "Saving strip response matrix to file: "<< resultFile << std::endl;
  calc->saveHistograms(resultFile.c_str());
}

//////////////////////////
//////////////////////////
//
// This helper function creates 3 strip response matrices for testing purposes.
//
void testGenerateResponse() {
  std::vector<double> sigma{0.5, 1, 2}; // mm
  for(auto & s : sigma) {
    generateResponse(s); // generates histograms and saves them to a separate ROOT file
  }
}

//////////////////////////
//////////////////////////
//
// This helper function reads existing response histograms (both XY_DET and Z_DET part) that correspond to some REFERENCE time settings:
// - REF peaking time input argument,
// - REF longitudinal diffusion input argument,
// - REF geometry file with drift velocity and sampling rate,
// and recomputes only Z_DET part using one or more NEW time settings:
// - NEW peaking time input argument,
// - NEW longitudinal diffusion input argument,
// - NEW geometry input file with drift velocity and samping rate,
// - NEW number of sampling points to generate Z_DET part,
// - NEW number of bins for sub-cell resolution for Z_DET part.
// In this way the most time consuming XY_DET part, which does not depend on longitudinal diffusion, drift velocity,
// sampling rate and peaking time, can be recycled to speed up generation of necessary strip response combinations.
// 
void replaceTimeResponse(double ref_sigmaXY=0.5,         // [mm] - same for REF and NEW response histograms
			 double ref_sigmaZ=-1,           // [mm]
			 double ref_peakingTime=-1,      // [ns]
			 const char *ref_geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
			 double new_sigmaZ=-1,           // [mm]
			 double new_peakingTime=232,     // [ns]
			 const char *new_geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
			 long new_npointsTime=10000000L, // use: >=10M to get smooth response distributions
			 int new_nbinsTime=50,           // use: >=50 for fitting purposes
			 int nstrips=7,                  // same for REF and NEW response histograms
			 int npads=14,                   // same for REF and NEW response histograms
			 int ncells=50) {                // same for REF and NEW response histograms
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  if(ref_sigmaZ<0) ref_sigmaZ=ref_sigmaXY; // when REF sigmaZ is omitted assume that REF sigmaZ=sigmaXY
  auto new_sigmaXY=ref_sigmaXY;
  if(new_sigmaZ<0) new_sigmaZ=new_sigmaXY; // when NEW sigmaZ is omitted assume that NEW sigmaZ=sigmaXY
  if(ref_peakingTime<0) ref_peakingTime=0; // when REF peakingTime is ommitted turn off additional smearing due to GET electronics
  if(new_peakingTime<0) new_peakingTime=0; // when NEW peakingTime is ommitted turn off additional smearing due to GET electronics
  auto ref_geo=std::make_shared<GeometryTPC>(ref_geometryFile, false);
  auto new_geo=std::make_shared<GeometryTPC>(new_geometryFile, false);
  const std::string referenceFile( (ref_peakingTime==0 ?
				    Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
					 nstrips, ncells, npads, ref_geo->GetSamplingRate(), ref_geo->GetDriftVelocity(),
					 ref_sigmaXY, ref_sigmaZ) :
				    Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
					 nstrips, ncells, npads, ref_geo->GetSamplingRate(), ref_geo->GetDriftVelocity(),
					 ref_sigmaXY, ref_sigmaZ, ref_peakingTime) ) );
  const std::string resultFile( (new_peakingTime==0 ?
				 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				      nstrips, ncells, npads, new_geo->GetSamplingRate(), new_geo->GetDriftVelocity(),
				      new_sigmaXY, new_sigmaZ) :
				 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				      nstrips, ncells, npads, new_geo->GetSamplingRate(), new_geo->GetDriftVelocity(),
				      new_sigmaXY, new_sigmaZ, new_peakingTime) ) );
  auto new_calc=new StripResponseCalculator(new_geo, nstrips, ncells, npads, new_sigmaXY, new_sigmaZ, new_peakingTime,
					    referenceFile.c_str(), true);
  if(new_calc->loadHistograms(resultFile.c_str())) {
    std::cout << "ERROR: Response histogram file " << resultFile << " already exists!" << std::endl;
    return;
  }
  new_calc->setDebug(true);
  new_calc->initializeTimeResponse(new_npointsTime, new_nbinsTime); // overwrite default value
  std::cout << "Saving strip response matrix to file: "<< resultFile << std::endl;
  new_calc->saveHistograms(resultFile.c_str());
}

//////////////////////////
//////////////////////////
//
// This function visualizes XY_DET part of 2D signal response matrix that accounts for
// the effective transverse diffusion (active volume, GEMs).
//
void plotStripResponse(double sigmaXY=0.5, // [mm] - effective diffusion along XY_DET
		       double sigmaZ=-1, // [mm] - effective diffusion along Z_DET
		       double peakingTime=-1, // [ns] - GET electronics peaking time
		       const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		       int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		       int npads=14, // use: (nstrips*2)
		       int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  if(sigmaZ<0) sigmaZ=sigmaXY; // when sigmaZ is omitted assume that sigmaZ=sigmaXY
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime,
					(peakingTime==0 ?
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ) :
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ, peakingTime) ), true);
  auto c=new TCanvas("c","c",5*300, 2.5*300);
  auto pad=0;
  c->Divide(5,3);
  for(auto dir=0; dir<3; dir++) {
    for(auto delta=-2; delta<=2; delta++) {
      pad++;
      c->cd(pad);
      gPad->SetRightMargin(0.2);
      gPad->SetLogz(false);
      auto h=calc->getStripResponseHistogram(dir,delta);
      if(!h) continue;
      h->SetStats(false);
      h->SetTitleOffset(1.8, "Z");
      h->DrawClone("COLZ"); // ("CONT4Z");
    }
  }
  c->Print(TString(Form("plot_responseXY_T%gmm", sigmaXY)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_responseXY_T%gmm", sigmaXY)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_responseXY_T%gmm", sigmaXY)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
//
// This function visualizes Z_DET part of 1D signal response matrix that accounts for:
// - effective longitudinal diffusion (detector volume, GEMs)
// - (optional) peaking time of the GET electronics.
//
void plotTimeResponse(double sigmaXY=0.5, // [mm] - effective diffusion along XY_DET
		      double sigmaZ=-1, // [mm] - effective diffusion along Z_DET
		      double peakingTime=-1, // [ns] - GET electronics peaking time
		      const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		      int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		      int npads=14, // use: (nstrips*2)
		      int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  if(sigmaZ<0) sigmaZ=sigmaXY; // when sigmaZ is omitted assume that sigmaZ=sigmaXY
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime,
					(peakingTime==0 ?
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ) :
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ, peakingTime) ), true);
  auto c=new TCanvas("c","c",3*300, 2.5*300);
  auto pad=0;
  //  std::vector<int> cells{-ncells, -ncells+1, -ncells+2, -1, 0, 1, ncells-2, ncells-1, ncells};
  std::vector<int> cells{-2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
  //  c->Divide(3,3);
  c->Divide(4,4);
  for(auto & cell : cells) {
    pad++;
    c->cd(pad);
    gPad->SetLeftMargin(0.2);
    gPad->SetRightMargin(0.05);
    gPad->SetLogy(false);
    auto h=calc->getTimeResponseHistogram(cell);
    if(!h) continue;
    h->SetStats(false);
    h->SetFillColor(kBlue);
    h->SetFillStyle(3001);
    h->SetLineColor(kBlue);
    h->SetLineWidth(2);
    h->SetTitleOffset(1.8, "Y");
    h->DrawClone("HISTO");
  }
  c->Print(TString(Form("plot_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".root");

  // alternative representation
  c->Clear();
  c->Divide(1,1);
  c->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.05);
  gPad->SetLogy(false);
  TH1D* first_hist=NULL;
  auto ymin=1e30;
  auto ymax=-1e30;
  for(auto cell=-ncells; cell<=ncells; cell++) {
    auto h=calc->getTimeResponseHistogram(cell);
    if(!h) continue;
    ymin=std::min(h->GetMinimum(), ymin);
    ymax=std::max(h->GetMaximum(), ymax);
    h->SetStats(false);
    if(cell<0) {
      h->SetLineColor(kBlue);
      h->SetFillStyle(0);
    } else if(cell>0) {
      h->SetLineColor(kRed+2);
      h->SetFillStyle(0);
    } else {
      h->SetFillColor(kGreen+2);
      h->SetLineColor(kGreen+2);
      h->SetFillStyle(3001);
    }
    h->SetLineWidth(2);
    h->SetTitleOffset(2, "Y");
    if(!first_hist) {
      first_hist=(TH1D*)h->DrawClone("HISTO");
    } else h->DrawClone("HISTO SAME");
  }
  first_hist->SetMinimum(ymin*0.95);
  first_hist->SetMaximum(ymax*1.05);
  first_hist->SetTitle(Form("Vertical response for #sigma_{Z}=%g mm, #tau=%g ns", sigmaZ, peakingTime));
  first_hist->SetTitleOffset(1.8, "Y");
  c->Update();
  c->Modified();
  c->Print(TString(Form("plot2_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot2_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot2_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".root");

  // intergral x-check per center position
  c->Clear();
  c->Divide(1,1);
  c->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.05);
  gPad->SetLogy(false);
  TH1D* h_sum=NULL;
  for(auto cell=-ncells; cell<=ncells; cell++) {
    auto h=calc->getTimeResponseHistogram(cell);
    if(!h) continue;
    if(!h_sum) {
      h_sum=(TH1D*)(h->Clone("h_sum"));
    } else {
      h_sum->Add(h.get());
    }
  }
  h_sum->SetStats(false);
  h_sum->SetLineWidth(2);
  h_sum->SetTitleOffset(2, "Y");
  h_sum->SetTitle(Form("Integrals of vertical response for #sigma_{Z}=%g mm, #tau=%g ns", sigmaZ, peakingTime));
  h_sum->SetTitleOffset(1.8, "Y");
  h_sum->DrawClone("HISTO");
  c->Update();
  c->Modified();
  c->Print(TString(Form("plot3_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot3_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot3_responseZ_L%gmm_P%gns", sigmaZ, peakingTime)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
void addConstantToTH2D(TH2D* h, double c) {
  if(!h) return;
  for(auto ibin1=1; ibin1<=h->GetNbinsX(); ibin1++) {
    for(auto ibin2=1; ibin2<=h->GetNbinsY(); ibin2++) {
      h->SetBinContent(ibin1, ibin2, h->GetBinContent(ibin1, ibin2) + c);
    }
  }
}


//////////////////////////
//////////////////////////
//
// This function demonstrates how to use StripResponseCalculator to obtain signals
// from a dummy point-like ionization cluster:
// 1. For filling full EventTPC(or just PEventTPC) charge deposits can be
//    added to existing PEventTPC by calling StripResponseCalculator::addCharge() with 3 arguments.
//    After filling all the charges a new EventTPC can be created out of PEventTPC::GetChargeMap().
//    This mode is usefull for signal digitization of Toy Monte Carlo tracks.
// 2. For filling three 2D projections (UZ, VZ and WZ) from merged strips
//    one has to declare histograms to be filled beforehand, and only then subsequently call
//    StripResponseCalculator::addCharge() with just 2 arguments.
//    This mode is useful for detailed fitting of dE/dx templates created on-the-fly for each
//    of three 2D projections (UZ, VZ and WZ).
//
void testResponse1(double sigmaXY=0.5, // [mm] - effective diffusion along XY_DET
		   double sigmaZ=-1, // [mm] - effective diffusion along Z_DET
		   double peakingTime=-1, // [ns] - GET electronics peaking time
		   const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		   int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		   int npads=14, // use: (nstrips*2)
		   int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  if(sigmaZ<0) sigmaZ=sigmaXY; // when sigmaZ is omitted assume that sigmaZ=sigmaXY
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime,
					(peakingTime==0 ?
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ) :
					 Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
					      nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
					      sigmaXY, sigmaZ, peakingTime) ) );

  // define point-like charge
  auto pos=TVector3(50.0, 0.0, 0.0); // mm
  auto charge=1000;

  // create dummy event to get empty UZ/VZ/WZ projection histograms
  auto event=new EventTPC(); // empty event
  event->SetGeoPtr(geo);
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm); // FIX 
    histosRaw[strip_dir] =  event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw); // FIX
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
  }

  // first example of use: fills EventTPC/PEventTPC and then extacts time projection from all strips
  //
  auto pevent=std::make_shared<PEventTPC>(); // empty PEventTPC
  calc->addCharge(pos, charge, pevent);
  event->SetChargeMap(pevent->GetChargeMap());
  auto hProjectionInMM=event->get1DProjection(definitions::projection_type::DIR_TIME, filter_type::none, scale_type::mm); // FIX
  auto hProjectionRaw=event->get1DProjection(definitions::projection_type::DIR_TIME, filter_type::none, scale_type::raw); // FIX

  // second example of use: directly fills UZ/VZ/WZ histograms
  //
  // associate UZ/VZ/WZ histograms with reponse calculator
  calc->setUVWprojectionsRaw(histosRaw);
  calc->setUVWprojectionsInMM(histosInMM);
  calc->addCharge(pos, charge);

  // plot results
  auto c=new TCanvas("c","c",4*500, 500);
  auto pad=0;
  c->Divide(4,1);
  //  for(auto & h : histosInRaw) {
  for(auto & h : histosRaw) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(false);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  pad++;
  c->cd(pad);
  gPad->SetRightMargin(0.2);
  gPad->SetLogy(false);
  auto h=hProjectionRaw;
  // auto h=hProjectionInMM;
  h->SetMinimum(minval); // for log scale
  h->SetStats(false);
  h->SetTitleOffset(1.5, "X");
  h->SetTitleOffset(1.5, "Y");
  h->DrawClone("HISTO");

  c->Print(TString(Form("plot_UVW_T%gmm_L%gmm_P%gns", sigmaXY, sigmaZ, peakingTime)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_UVW_T%gmm_L%gmm_P%gns", sigmaXY, sigmaZ, peakingTime)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_UVW_T%gmm_L%gmm_P%gns", sigmaXY, sigmaZ, peakingTime)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
//
// This function demonstrates how to use StripResponseCalculator in a loop to fill three projections
// (UV, VZ and WZ) from the merged strips.
// For the very last event three 2D projections (UV, VZ and WZ) are drawn as a cross-check.
// This mode is usefull for preparing many 2D signal templates while fitting raw-data/MC EventTPC.
//
void testResponse2(int nevents=1,
		   const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		   int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		   int npads=14, // use: (nstrips*2)
		   int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  std::vector<double> sigma{0.5, 1, 2}; // mm
  std::vector<StripResponseCalculator*> calc(sigma.size());
  for(auto i=0; i<calc.size(); i++) {
    auto sigmaXY=sigma[i];
    auto sigmaZ=sigma[i];
    calc[i]=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, 0, Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root", nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(), sigmaXY, sigmaZ));
  }

  TStopwatch t;
  t.Start();

  // create dummy event to get empty UZ/VZ/WZ projection histograms
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(geo);
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm); // FIX
    histosRaw[strip_dir] =  event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw); // FIX
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
  }

  // associate UZ/VZ/WZ histograms (merged strip sections) with reponse calculator
  for(auto i=0; i<calc.size(); i++) {
    calc[i]->setUVWprojectionsInMM(histosInMM);
    //    calc[i]->setUVWprojectionsRaw(histosRaw);
  }
  
  for(auto ievent=0; ievent<nevents; ievent++) {
    if(ievent%100==0) std::cout << "event=" << ievent << std::endl;

    // reset associated 2D histograms
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      histosInMM[strip_dir]->Reset();
      addConstantToTH2D(histosInMM[strip_dir].get(), minval);
      //      histosRaw[strip_dir]->Reset();
      //      addConstantToTH2D(histosRaw[strip_dir].get(), minval);
    }
    
    // fill UZ/VZ/WZ histograms (merged strip sections) - simulate triangular shape of dE/dx
    for(auto i=0; i<calc.size(); i++) {
      auto pos0=TVector3(50.0, 50.0, -30.0);
      auto pos=pos0+TVector3(0, -50, 0)*i;
      auto charge=10000;
      auto length=50.0; // mm
      auto npoints=(int)(3*length/geo->GetStripPitch());
      auto unit_vec=TVector3(1,1,1).Unit();
      for(auto ipoint=0; ipoint<npoints; ipoint++) {
	calc[i]->addCharge(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
      }
    }
    /*
    // fill UZ/VZ/WZ histograms (merged strip sections) - simulate point charge
    for(auto i=0; i<calc.size(); i++) {
      auto pos0=TVector3(60.0, 60.0, 40.0);
      auto pos=pos0+TVector3(0, -50, 0)*i;
      auto charge=10000;
      calc[i]->addCharge(pos, charge);
    }
    */
  }
  
  t.Stop();
  std::cout << "===========" << std::endl;
  std::cout << "CPU time needed to generate " << nevents << " TH2D triplets:" << std::endl;
  t.Print();
  std::cout << "===========" << std::endl;

  // plot results
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(true);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  c->Print("plot_UVW_mix.pdf");
  c->Print("plot_UVW_mix.png");
  c->Print("plot_UVW_mix.root");
}

//////////////////////////
//////////////////////////
//
// This function demonstrates how to use StripResponseCalculator in a loop to fill three projections
// (UV, VZ and WZ) from the merged strips using constant dummy tracks.
// For the very last event three 2D projections (UV, VZ and WZ) are drawn as a cross-check.
// This mode is usefull for preparing many 2D signal templates while fitting raw-data/MC EventTPC.
//
void testResponse3(int nevents=1,
		   const char *geometryFile="geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
		   int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		   int npads=14, // use: (nstrips*2)
		   int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){ // FIX
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  std::vector<double> sigma{0.5, 1, 2};
  std::vector<StripResponseCalculator*> calc(sigma.size());
  for(auto i=0; i<calc.size(); i++) {
    auto sigmaXY=sigma[i];
    auto sigmaZ=sigma[i];
    calc[i]=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, 0, Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root", nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(), sigmaXY, sigmaZ));
  }

  TStopwatch t;
  t.Start();

  // create in memory N dummy events for algorithm's speed test
  // NOTE: re-use existing EventTPC pointer
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(geo);

  for(auto ievent=0; ievent<nevents; ievent++) {
    if(ievent%100==0) std::cout << "event=" << ievent << std::endl;

    // fill EventTPC - simulate triangular shape of dE/dx
    auto pevent=std::make_shared<PEventTPC>(); // empty PEventTPC
    for(auto i=0; i<calc.size(); i++) {
      auto pos0=TVector3(50.0, 50.0, -30.0);
      auto pos=pos0+TVector3(0, -50, 0)*i;
      auto charge=10000;
      auto length=50.0; // [mm]
      auto npoints=(int)(3*length/geo->GetStripPitch());
      auto unit_vec=TVector3(1,1,1).Unit();
      for(auto ipoint=0; ipoint<npoints; ipoint++) {
	calc[i]->addCharge(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints, pevent);
	event->SetChargeMap(pevent->GetChargeMap());
      }
    }
  }

  t.Stop();
  std::cout << "===========" << std::endl;
  std::cout << "CPU time needed to generate " << nevents << " EventTPC objects:" << std::endl;
  t.Print();
  std::cout << "===========" << std::endl;

  // get UZ/VZ/WZ histograms (merged strip sections) from EventTPC
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm); // FIX
    histosRaw[strip_dir] =  event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw); // FIX
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
  }

  // plot results for last event
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(true);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  c->Print("plot_UVW_mix2.pdf");
  c->Print("plot_UVW_mix2.png");
  c->Print("plot_UVW_mix2.root");
}

//////////////////////////
//////////////////////////
//
// This function demonstrates how to use StripResponseCalculator in a loop to create new PEventTPC ROOT tree
// out of existing Track3D ROOT tree.
// For the very last event three 2D projections (UV, VZ and WZ) are drawn as a cross-check.
// This mode is usefull for signal digitization of Toy Monte Carlo tracks.
//
void testResponse4(const char *fname, // input ROOT file with Track3D tree
		   long maxevents=0, // maximal number of event to process
		   const char *geometryFile="geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat",
		   double pressure_mbar=130.0, // working pressure
		   double temperature_K=273.15+20, // working temperature
		   double sigmaXY_mm=0.85, // [mm] - effective diffusion along XY_DET
		   double sigmaZ_mm=0.86, // [mm] - effective diffusion along Z_DET
		   double peakingTime=232, // [ns] - GET electronics peaking time
		   int nstrips=7, // use: nstrips>(3*sigmaXY)/strip_pitch
		   int npads=14, // use: (nstrips*2)
		   int ncells=50) { // use: ncells>(3*sigmaZ)/(drift_velocity/sampling_frequency)
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }

  // initialize TPC geometry and electronics parameters
  //
  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  // initialize strip response calculator
  //
  // NOTE: From MAGBOLTZ simulations at { p=250 mbar, T=293 K }
  // 1. For drift region at E = 2744 V / 196 mm = 140 V/cm::
  // * W   = 0.4050 cm/us
  // * D_T = 205.846 um/sqrt(cm)
  // * D_L = 205.499 um/sqrt(cm)
  // 2. Extrapolation to 98 mm of drift (nominal beam axis to GEM distance):
  // * sigma_x = sigma_y = D_T*sqrt(9.8cm/1cm)=0.64 mm
  // * sigma_z =           D_L*sqrt(9.8cm/1cm)=0.64 mm
  // 3. For transfer regions at E = 350 V / 3 mm = 1166.67 V/cm:
  // * W   = 5.729 cm/us - linear interpolation between 1000 V/cm and 1250 V/cm
  // * D_T = 242.980 um/sqrt(cm) - linear interpolation between 1000 V/cm and 1250 V/cm
  // * D_L = 272.577 um/sqrt(cm) - linear interpolation between 1000 V/cm and 1250 V/cm
  // 4. Extrapolation to drift across three transfer regions of 3 mm thickness each:
  // * sigma_x = sigma_y = D_T*sqrt(3*0.3cm/1cm) = 0.23 mm
  // * sigma_z =           D_L*sqrt(3*0.3cm/1cm) = 0.26 mm
  // 5. Overall combined:
  // * sigma_x = sigma_y = sqrt( 0.64^2 + 0.23^2 ) mm = 0.68 mm
  // * sigma_z =           sqrt( 0.64^2 + 0.26^2 ) mm = 0.69 mm
  //
  // NOTE: From MAGBOLTZ simulations at { p=130 mbar, T=293 K }
  // 1. For drift region at E = 1764 V / 196 mm = 90 V/cm:
  // * W   = 0.5000 cm/us
  // * D_T = 262.422 um/sqrt(cm)
  // * D_L = 261.901 um/sqrt(cm)
  // 2. Extrapolation to 98 mm of drift (nominal beam axis to GEM distance):
  // * sigma_x = sigma_y = D_T*sqrt(9.8cm/1cm)=0.82 mm
  // * sigma_z =           D_L*sqrt(9.8cm/1cm)=0.82 mm
  // 3. For transfer regions at E = 310 V / 3 mm = 1033.33 V/cm:
  // * W   = 4.7386 cm/us - linear interpolation between 1000 V/cm and 1250 V/cm
  // * D_T = 218.494 um/sqrt(cm) - linear interpolation between 1000 V/cm and 1250 V/cm
  // * D_L = 272.459 um/sqrt(cm) - linear interpolation between 1000 V/cm and 1250 V/cm
  // 4. Extrapolation to drift across three transfer regions of 3 mm thickness each:
  // * sigma_x = sigma_y = D_T*sqrt(3*0.3cm/1cm) = 0.21 mm
  // * sigma_z =           D_L*sqrt(3*0.3cm/1cm) = 0.26 mm
  // 5. Overall combined:
  // * sigma_x = sigma_y = sqrt( 0.82^2 + 0.21^2 ) mm = 0.85 mm
  // * sigma_z =           sqrt( 0.82^2 + 0.26^2 ) mm = 0.86 mm
  //
  double sigmaXY=sigmaXY_mm; // 0.64; // educated guess of transverse charge spread after 10 cm of drift (middle of drift cage)
  double sigmaZ=sigmaZ_mm; // 0.64; // educated guess of longitudinal charge spread after 10 cm of drift (middle of drift cage)
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  const std::string initFile( (peakingTime==0 ?
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				    nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				    sigmaXY, sigmaZ) :
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				    nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				    sigmaXY, sigmaZ, peakingTime) ) );
  auto calcStrip=std::make_shared<StripResponseCalculator>(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime, initFile.c_str());
  std::cout << "Loading strip response matrix from file: "<< initFile << std::endl;
  // initialize ion range and dE/dx calculator
  //
  auto calcIon=std::make_shared<IonRangeCalculator>(CO2, pressure_mbar, temperature_K, false);
  double pointDensity=10/1.0; // [points/mm], density of generated points along the track
  double adcPerMeV=1e5; // [(ADC units)/MeV] scaling factor (track's integrated charge)/(track's Ekin_LAB)

  // opens input ROOT file with generated tracks (Track3D objects)
  //
  // NOTE: Tree's and branch names are kept the same as in HIGS_analysis
  //       for easy MC-reco comparison. To be replaced with better
  //       class/object in future toy MC.
  // NOTE: * branch RecoEvent - must always be present
  //       * branch EventInfo - is optional in primitive toy MC
  TFile *aFile = new TFile(fname, "OLD");
  TTree *aTree=(TTree*)aFile->Get("TPCRecoData");
  if(!aTree) {
    std::cerr<<"ERROR: Cannot find 'TPCRecoData' tree!"<<std::endl;
    return;
  }
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
    return;
  }
  aBranch->SetAddress(&aTrack);
  eventraw::EventInfo *aEventInfo = 0;
  TBranch *aBranchInfo = aTree->GetBranch("EventInfo");
  if(!aBranchInfo) {
   std::cerr<<"WARNING: Cannot find 'EventInfo' branch!"<<std::endl;
  }
  else{
    aEventInfo = new eventraw::EventInfo();
    aBranchInfo->SetAddress(&aEventInfo);
  }
  unsigned int nEntries = aTree->GetEntries();

  // When (optional) "EventInfo" branch is present try to sort input tree in ascending order of {runID, eventID}
  TTreeIndex *I=NULL;
  Long64_t* index=NULL;
  if(aBranchInfo) {
    aTree->BuildIndex("runId", "eventId");
    I=(TTreeIndex*)aTree->GetTreeIndex(); // get the tree index
    index=I->GetIndex();
  }

  // output ROOT file
  TFile *outFile = new TFile("Generated_PEventTPC.root", "RECREATE");
  TTree outTree("TPCData","");
  auto pevent=std::make_shared<PEventTPC>(); // empty PEventTPC
  auto persistent_pevent_ptr = pevent.get();
  Int_t bufsize=128000;
  int splitlevel=2;
  outTree.Branch("Event", &persistent_pevent_ptr, bufsize, splitlevel);

  // create in memory N dummy events for algorithm's speed test
  // NOTE: re-use existing EventTPC pointer
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(geo);

  // measure elapsed time
  TStopwatch t;
  t.Start();

  // main event creation loop
  maxevents=(maxevents<=0 ? nEntries : std::min((unsigned int)maxevents, nEntries));
  for(auto ievent=0; ievent<maxevents; ievent++) {
    if(index) {
      aBranch->GetEntry(index[ievent]);
      aBranchInfo->GetEntry(index[ievent]);
    } else {
      aBranch->GetEntry(ievent);
    }

    if(ievent%100==0) std::cout << "event=" << ievent << std::endl;

    // fill EventTPC with simulated realistic track topologies and dE/dx profiles
    //    auto pevent=std::make_shared<PEventTPC>(); // empty PEventTPC
    pevent->Clear();

    const int ntracks = aTrack->getSegments().size();
    TrackSegment3DCollection list = aTrack->getSegments();

    // loop over generated tracks
    for(auto & track: list) {
      auto origin=track.getStart(); // mm
      auto length=track.getLength(); // mm
      auto npoints=std::max((int)(pointDensity*length), 10);
      auto unit_vec=track.getTangent();
      auto pid=track.getPID();
      auto Ekin_LAB=calcIon->getIonEnergyMeV(pid, length); // MeV, Ekin should be equal to Bragg curve integral
      auto curve(calcIon->getIonBraggCurveMeVPerMM(pid, Ekin_LAB, npoints)); // MeV/mm
#ifdef DEBUG
      double sum_charge=0.0;
#endif
      for(auto ipoint=0; ipoint<npoints; ipoint++) { // generate NPOINTS hits along the track
	auto depth=(ipoint+0.5)*length/npoints; // mm
	auto hitPosition=origin+unit_vec*depth; // mm
	auto hitCharge=adcPerMeV*curve.Eval(depth)*(length/npoints); // ADC units
	calcStrip->addCharge(hitPosition, hitCharge, pevent);
	if(aEventInfo) pevent->SetEventInfo(*aEventInfo);
#ifdef DEBUG
	sum_charge+=hitCharge;
	std::cout << "sim depth=" << depth << ", sim charge=" << hitCharge << std::endl;
#endif
      }
#ifdef DEBUG
      std::cout << "sim particle: PID=" << pid << ", sim.range[mm]=" << length
		<< ", sim.charge[ADC units]=" << sum_charge
		<< ", E(range)[MeV]=" << Ekin_LAB
		<< ", charge/adcPerMeV[MeV]=" << sum_charge/adcPerMeV << std::endl;
#endif
    }
    outTree.Fill();
    //    event->SetChargeMap(pevent->GetChargeMap());  // no need to fill EventTPC
    }
  outTree.Write("", TObject::kOverwrite); // save only the new version of the tree

  t.Stop();
  std::cout << "===========" << std::endl;
  std::cout << "CPU time needed to generate " << maxevents << " EventTPC objects:" << std::endl;
  t.Print();
  std::cout << "===========" << std::endl;

  // get UZ/VZ/WZ histograms (merged strip sections) from last EventTPC
  event->SetChargeMap(pevent->GetChargeMap()); // fill only the last EventTPC to make example plots
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  std::shared_ptr<TH1D> histoTimeProjRaw;
  std::shared_ptr<TH1D> histoTimeProjInMM;
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm); // FIX
    histosRaw[strip_dir] =  event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw); // FIX
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
  }
  histoTimeProjInMM = event->get1DProjection(definitions::projection_type::DIR_TIME, filter_type::none, scale_type::mm);
  histoTimeProjRaw =  event->get1DProjection(definitions::projection_type::DIR_TIME, filter_type::none, scale_type::raw);

  aFile->Close();

  // plot results for last event
  auto c=new TCanvas("c","c",2*500, 2*500);
  auto pad=0;
  c->Divide(2,2);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(false);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  pad++;
  c->cd(pad);
  gPad->SetRightMargin(0.2);
  gPad->SetLogz(false);
  auto h2=histoTimeProjInMM;
  h2->SetStats(false);
  h2->SetTitleOffset(1.5, "X");
  h2->SetTitleOffset(1.5, "Y");
  h2->DrawClone();

  c->Print("plot_UVW_lastevent.pdf");
  c->Print("plot_UVW_lastevent.png");
  c->Print("plot_UVW_lastevent.root");
}
