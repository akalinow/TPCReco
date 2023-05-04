/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// Example of using ROOT fitter with PEventTPC, Track3D, StripResponseCalculator, IonRangeCalculator
//
// Mikolaj Cwiok (UW) - 21 Apr 2023
//
//
// 1. Example of single event reconstruction:
// root -l << EOF |& tee log
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] loop(1)
// root[2] EOF
//
// 2. EVENT FITTER (PEventTPC from Toy MC digitizer and TRUE tracks from Toy MC generator, events 0..100):
// root -l << EOF |& tee log
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] loop_Graw_Track3D("/mnt/data/mikolaj/TPCReco/results/fake_tracks_HIGS/sample_20230212/Gen/Generated_Track3D__250mbar_13.1MeV_sigma0keV__noBoost_ISO_O16_fixedVtx_extTrg_contained__100k.root","/mnt/data/mikolaj/TPCReco/results/fake_tracks_HIGS/sample_20230212/Gen/jobs/Generated_PEventTPC__250mbar_13.1MeV_sigma0keV__noBoost_ISO_O16_fixedVtx_extTrg_contained__20k_1001.root", 0, 100, "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat", 250.0, 293.15, 0.68, 0.69, 232, true, true, true, true, false, false);
// root[2] EOF
//
// 3. EVENT FITTER WITH PENALTY TERMS (GRAW from real data, starting point from human-based RECO, 2- and 3-prong events 3852..4052):
// root -l << EOF |& tee log
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] FitOptionType options;
// root[2] options.use_penalty_momentum_CMS=true;
// root[3] options.use_penalty_carbon_length=true;
// root[4] options.use_penalty_alpha_length=true;
// root[5] options.photonUnitVec_DET_LAB=TVector3(-1,0,0);
// root[6] options.photonEnergyInMeV_LAB=9.845;
// root[7] options.maxDeviationInMM=7.0;
// root[8] options.expectedAdcNoiseRMS=7.0;
// root[9] loop_Graw_Track3D("/mnt/NAS_STORAGE_ANALYSIS/higs_2022/MonteCarlo/DataFit_9.845MeV/Reco_init/Reco_20220822T220311_0001_mk.root", "/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd0_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd1_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd2_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd3_2022-08-22T22:03:11.798_0001.graw", 3852, 3852+200, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", 130.0, 293.15, 1.2, 1.2, 232, true, false, true, true, true, true, options);
// root[10] EOF
//
// 4. EVENT PLAYER (PEventTPC from Toy MC digitizer and TRUE tracks from Toy MC generator, events 0..100):
// root -l
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] loop_Graw_Track3D_Display("/mnt/data/mikolaj/TPCReco/results/fake_tracks_HIGS/sample_20230212/Gen/Generated_Track3D__250mbar_13.1MeV_sigma0keV__noBoost_ISO_O16_fixedVtx_extTrg_contained__100k.root","/mnt/data/mikolaj/TPCReco/results/fake_tracks_HIGS/sample_20230212/Gen/jobs/Generated_PEventTPC__250mbar_13.1MeV_sigma0keV__noBoost_ISO_O16_fixedVtx_extTrg_contained__20k_1001.root", 0, 100, "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat", 13.1, 250.0, 293.15, true, true, true, true, false, false);
//
// 5. EVENT PLAYER (GRAW from real data, FITTED tracks from human-based RECO, 2- and 3-prong events 3852..4052)
// root -l
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] loop_Graw_Track3D_Display("/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_85MeV/clicked/Reco_20220822T220311_0001_mk.root", "/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd0_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd1_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd2_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd3_2022-08-22T22:03:11.798_0001.graw", 3852, 3852+200, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", 9.845, 130.0, 293.15, true, false, true, true, false, true);
//
// 6. EVENT PLAYER (GRAW from real data, FITTED track from automatic RECO, REFERENCE/TRUE tracks from human-based RECO, 2- and 3-prong events 3852..7703)
// root -l
// root[0] .L ../test/testStripResponseFitter.cxx
// root[1] loop_Graw_Track3D_Display("/mnt/NAS_STORAGE_ANALYSIS/higs_2022/MonteCarlo/DataFit_9.845MeV/Reco_T1.5mm_L1.5mm_P232ns_50x50bins/Reco_Track3D_0001.root","/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd0_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd1_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd2_2022-08-22T22:03:11.798_0001.graw,/mnt/NAS_STORAGE/20220822_extTrg_CO2_130mbar/CoBo0_AsAd3_2022-08-22T22:03:11.798_0001.graw", 3852, 3852+3852-1, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", 9.845, 130.0, 293.15, true, false, true, true, false, true, "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/MonteCarlo/DataFit_9.845MeV/Reco_init/Reco_20220822T220311_0001_mk.root");
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#define WITH_GET // temporary HACK - TO BE REPLACED WITH PROPER CMAKE FLAG

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../GrawToROOT/include)
R__ADD_INCLUDE_PATH($GET_DIR/GetSoftware_bin/$GET_RELEASE/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "colorText.h"

#include <TMath.h>
#include <TRandom3.h>
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
#include <Math/Functor.h>
#include <Fit/Fitter.h>

#include "CommonDefinitions.h"
#include "GeometryTPC.h"
#include "EventTPC.h"
#include "EventSourceBase.h"
#ifdef WITH_GET
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#endif
#include "EventSourceROOT.h"
#include "TrackSegment3D.h"
#include "Track3D.h"
#include "IonRangeCalculator.h"
#include "StripResponseCalculator.h"
#include "UtilsMath.h"
#include "RunIdParser.h"
#include "HIGGS_analysis.h"

#define DEBUG_CHARGE    false
#define DEBUG_SIGNAL    false // plot UVW projections for reference data and starting point template at each step
#define DEBUG_CHI2      false
#define DEBUG_DRAW_FIT  false // plot UVW projections of final fitted templates
#define DRAW_AUTO_ZOOM  true  // plot UVW projection using automatic zoom algorithm
#define DRAW_SAME_SCALE false // plot UVW projection using the same scale of dE/dx for all strip directions
#define DRAW_SAME_SCALE_MARGIN 0.05 // add +/- 5% margin for common dE/dx scale
#define DRAW_AUTO_ZOOM_MARGIN 0.4 // add +/- 40% margin for histogram ranges in AUTO ZOOM mode (if possible)
#define DRAW_PAD_FILL_COLOR kAzure-6 // matches default kBird palette
#define DRAW_LOG_SCALE true // use LOG scale for dE/dx axis (NOTE: differential plots will still use LIN scale)

#define MISSING_PID_REPLACEMENT_ENABLE true // TODO - to be parameterized
#define MISSING_PID_1PRONG             ALPHA // TODO - to be parameterized
#define MISSING_PID_2PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_2PRONG_TRAILING    CARBON_12 // TODO - to be parameterized
#define MISSING_PID_3PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_MIDDLE      ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_TRAILING    ALPHA // TODO - to be parameterized

enum class BeamDirection{
  PLUS_X,
  MINUS_X
};

using namespace ROOT::Math;

class GeometryTPC;
class StripResponseCalculator;
class IonRangeCalculator;
class TrackSegment3D;
class Track3D;
class HIGGS_analysis;

// _______________________________________
//
// helper struct for storing fit parameters and constraints
typedef struct {
  // fitter parameters:
  double tolerance{1e-4}; // ROOT::Fit::Fitter TOLERANCE parameter
  double expectedAdcNoiseRMS{7.0}; // ADC units, estimate of RMS noise per channel per time cell, e.g. from pedestal runs
  double maxDeviationInMM{7.0}; // mm, maximal deviation from the starting point for setting parameter limits
  double maxAdcFactor{10.0}; // maximal multiplicative deviation (>1) from the starting point for setting ADC scale limits
  // region of interest to be fitted:
  bool use_initial_track_region{false};
  double radius_initial_track_region{10.0}; // mm, radius/width of slot-shaped region of interest around initial track segments
  // chi2 penalty terms:
  bool use_penalty_momentum_CMS{false};
  bool use_penalty_momentum_LAB{false};
  bool use_penalty_carbon_length{false};
  bool use_penalty_alpha_length{false};
  double scale_penalty_momentum{1.0}; // penalty=1 when (out_of_limit_discrepancy/scale)=1
  double scale_penalty_carbon_length{1.0}; // penalty=1 when (out_of_limit_discrepancy/scale)=1
  double scale_penalty_alpha_length{1.0}; // penalty=1 when (out_of_limit_discrepancy/scale)=1
  double delta_momentum_max{10.0}; // MeV/c, maximal allowed discrepancy for CMS/LAB momentum conservation
  double carbon_length_min{2.0}; // mm, minimal allowed length of carbon track
  double carbon_length_max{30.0}; // mm, maximal allowed length of carbon track
  double alpha_length_min{5.0}; // mm, minimal allowed length of alpha track
  double alpha_length_max{300.0}; // mm, maximal allowed length of alpha track
  bool use_nominal_beam_energy{true}; // for LAB-CMS boosts and momentum conservation constraints
  TVector3 photonUnitVec_DET_LAB{0,0,0}; // dimensionless, LAB reference frame, detector coordinate system
  double photonEnergyInMeV_LAB{0}; // MeV, nominal energy of the gamma beam for momentum conservation constraints
} FitOptionType;
// _______________________________________
//
// helper function to define shape of the penalty term:
// * for x < 0 : returns 0
// * for x > 0 : returns x^2
// This function is non-negative and differentiable at x=0.
double penaltyFunc(double x) {
  return pow(std::max(0.0, x), 2.0);
}
// _______________________________________
//
// helper class for smearing track endpoints (for fit convergence tests)
TVector3 getRandomDir(TRandom *r) {
  if(!r) {
    std::cout << __FUNCTION__ << ": Wrong random generator pointer!" << std::endl << std::flush;
    exit(1);
  }
  TVector3 unit_vec;
  unit_vec.SetMagThetaPhi(1.0, TMath::ACos(r->Uniform(-1.0, 1.0)), r->Uniform(0.0, TMath::TwoPi()));
  return unit_vec;
}
// _______________________________________
//
// function Object to be minimized / maximized
class HypothesisFit {
private:
  std::vector<TH2D*> myRefHistosInMM_orig; // pointers to UVW projections in mm (for reference, original)
  std::vector<TH2D*> myRefHistosInMM; // pointers to UVW projections in mm (for reference, after optional modifications)
  std::vector<TH2D*> myFitHistosInMM; // pointers to TEST UVW projections in mm (to be cleared and filled at each fit iteration)
  std::shared_ptr<StripResponseCalculator> myResponseCalc; // external calculator (must be properly initialized beforehand)
  std::shared_ptr<IonRangeCalculator> myRangeCalc; // external calculator (must be properly initialized beforehand)
  std::shared_ptr<GeometryTPC> myGeometryPtr; 
  double myPointsPerMM{10.0}; // density of generated points along the track [points/mm]
  int myPointsMin{100}; // minimum number of generated points along the track
  double myScale{1.0}; // global scaling factor for dE/dx of fitted tracks
  Track3D myTrackFit; // collection of fitted tracks
  std::vector<pid_type> myParticleList; // list of track PIDs for current hypothesis
  int myNparams{7}; // number of parameters to be fitted (minimal=7)
                    // 0 --> global ADC multiplicative scaling factor for the fitted histogram
                    // 1-3 --> common event vertex XYZ_DET [mm]
                    // 4-6 --> endpoint XYZ_DET [mm] of the 1st track
                    // 7-9 --> endpoint XYZ_DET [mm] of the 2nd track (optional), etc.
  unsigned long myRefNpoints{0}; // total number of non-empty bins in UVW reference histograms to be fitted
  FitOptionType myOptions;

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // generates three UVW projections out of fitted Track3D collection
  void fillFitHistogramsInMM() {

    // reset histograms
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      myFitHistosInMM[strip_dir]->Reset();
    }
    
    // loop over hypothesis tracks
    const int ntracks = myTrackFit.getSegments().size();
    TrackSegment3DCollection list = myTrackFit.getSegments();

    for(auto & track: list) {
      auto origin=track.getStart(); // mm
      auto length=track.getLength(); // mm
      auto npoints=std::max((int)(myPointsPerMM*length), myPointsMin);
      //      int npoints=1000; // FIXED
      auto unit_vec=track.getTangent();
      auto pid=track.getPID();
      auto Ekin_LAB=myRangeCalc->getIonEnergyMeV(pid, length); // MeV, Ekin should be equal to Bragg curve integral
      auto curve(myRangeCalc->getIonBraggCurveMeVPerMM(pid, Ekin_LAB, npoints)); // MeV/mm
#if defined(DEBUG_CHARGE) && DEBUG_CHARGE
      double sum_charge=0.0;
#endif
      for(auto ipoint=0; ipoint<npoints; ipoint++) { // generate NPOINTS hits along the track
	auto depth=(ipoint+0.5)*length/npoints; // mm
	auto hitPosition=origin+unit_vec*depth; // mm
	auto hitCharge=myScale*curve.Eval(depth)*(length/npoints); // ADC units
	myResponseCalc->addCharge(hitPosition, hitCharge);
#if defined(DEBUG_CHARGE) && DEBUG_CHARGE
	sum_charge+=hitCharge;
	std::cout << "sim depth=" << depth << ", sim charge=" << hitCharge << std::endl;
#endif
      }
#if defined(DEBUG_CHARGE) && DEBUG_CHARGE
      std::cout << "sim particle: PID=" << pid << ", sim.range[mm]=" << length
		<< ", sim.charge[ADC units]=" << sum_charge
		<< ", E(range)[MeV]=" << Ekin_LAB
		<< ", charge/scale[MeV]=" << sum_charge/myScale << std::endl;
#endif
      //// DEBUG
      // std::cout<<__FUNCTION__<<": length=" << length << ", npoints=" << npoints << std::endl;
      // origin.Print();
      // unit_vec.Print();
      // for(auto &it: myFitHistosInMM) {
      // 	std::cout << " referenceHistosInMM[*]: GetIntegral=" << it->Integral() << ", GetEntries=" << it->GetEntries() << ", GetEffectiveEntries=" << it->GetEffectiveEntries() << std::endl;
      // }
      //// DEBUG
    }

    //// DEBUG
#if DEBUG_SIGNAL
    static auto isFirst=true;
    if(isFirst) {
      TFile f("fit_ref_histos.root", "RECREATE");
      f.cd();
      TCanvas c("c", "c", 1200, 400);
      c.SetName("c_ref");
      c.SetTitle(c.GetName());
      c.Divide(3,1);
      int ipad=0;
      for(auto &it: myRefHistosInMM) {
	ipad++;
	c.cd(ipad);
	auto h=(TH2D*)(it->DrawClone("COLZ"));
	h->SetDirectory(0);
	h->SetName(Form("ref_%s",h->GetName()));
	h->SetTitle(Form("REF %s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
      }
      c.Update();
      c.Modified();
      c.Write();
      c.Clear();
      c.SetName("c_fit");
      c.SetTitle(c.GetName());
      c.Divide(3,1);
      ipad=0;
      for(auto &it: myFitHistosInMM) {
	ipad++;
	c.cd(ipad);
	auto h=(TH2D*)(it->DrawClone("COLZ"));
	h->SetDirectory(0);
	h->SetName(Form("fit_%s",h->GetName()));
	h->SetTitle(Form("FIT %s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
      }
      c.Update();
      c.Modified();
      c.Write();
      f.Close();
      isFirst=false;
    }
#endif
    //// DEBUG
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void initializeFitHistogramsInMM() {

    // reset
    for(auto &it: myFitHistosInMM) {
      if(it) {
	delete it; // it->Delete();
      }
    }
    myFitHistosInMM.clear();
    
    // create empty TH2D projections
    for(auto &it: myRefHistosInMM) {
      if(!it) {
	std::cout << __FUNCTION__ << ": Wrong external reference TH2D pointer!" << std::endl << std::flush;
	exit(1);
      } else {
	auto h=(TH2D*)(it->Clone(Form("fit_%s",(it)->GetName())));
	if(!h) {
	  std::cout << __FUNCTION__ << ": Cannot duplicate external reference TH2D histogram!" << std::endl << std::flush;
	  exit(1);
	}
	h->SetDirectory(0);
	h->Sumw2(true);
	h->Reset();
       myFitHistosInMM.push_back(h);
      }
    }

    // associate TH2D projections with StripResponseCalculator
    myResponseCalc->setUVWprojectionsInMM(myFitHistosInMM);
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  unsigned long countNonEmptyBins(TH1 *h) const {
    unsigned long result=0;
    switch (h->GetDimension()) {
    case 1:
      for(auto iBin=1; iBin <= h->GetNbinsX(); iBin++) {
	if(h->GetBinError(iBin) || h->GetBinContent(iBin)) result++;
      }
      break;
    case 2:
      for(auto iBinX=1; iBinX <= h->GetNbinsX(); iBinX++) {
	for(auto iBinY=1; iBinY <= h->GetNbinsY(); iBinY++) {
	  if(h->GetBinError(iBinX, iBinY) || h->GetBinContent(iBinX, iBinY)) result++;
	}
      }
      break;
    default:
      std::cout << __FUNCTION__ << ": Wrong histogram dimension!" << std::endl << std::flush;
      exit(1);
    };
    return result;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // modifies original 2D histogram and
  // leaves only hits inside specific regions of interest given by TGraphs
  void cropCutRegion2D(TH2D *h2, std::vector<TGraph> cuts) const {
    if(!h2) return;
    for(auto &gr: cuts) if(gr.GetN()<3) return;
    auto h2_copy=(TH2D*)h2->Clone(Form("%s_clone", h2->GetName())); // make a complete copy
    if(!h2_copy) return;
    h2->Reset(); // remove all content
    for(auto iBinX=1; iBinX <= h2_copy->GetNbinsX(); iBinX++) {
      for(auto iBinY=1; iBinY <= h2_copy->GetNbinsY(); iBinY++) {
	double val=0.0, err=0.0;
	if((err=h2_copy->GetBinError(iBinX, iBinY)) || (val=h2_copy->GetBinContent(iBinX, iBinY))) {
	  for(auto &gr: cuts) {
	    if(gr.IsInside(h2_copy->GetXaxis()->GetBinCenter(iBinX), h2_copy->GetYaxis()->GetBinCenter(iBinY))) {
	      h2->SetBinContent(iBinX, iBinY, val); // allow hits inside specific region of interest
	      h2->SetBinError(iBinX, iBinY, err);
	      break; // skip other remaining cuts (logic OR)
	    }
	  }
	}
      }
    }
    h2_copy->Delete();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // creates a closed-loop slot-shaped region along track segment projection for a given UZ/VZ/WZ projection
  TGraph initializeCutRegion2D(TrackSegment3D &aSeg, int dir) {
    TGraph result;
    if(dir<definitions::projection_type::DIR_U || dir>definitions::projection_type::DIR_W) return result;
    const auto vtx=aSeg.getStart();
    const auto endpoint=aSeg.getEnd();
    const auto geo=aSeg.getGeometry();
    if(!geo) return result;
    auto err=false;
    double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), dir, err); // strip position [mm] for a given XY_DET position
    double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), dir, err); // strip position [mm] for a given XY_DET position
    if(err) return result;
    double zstart_pos=vtx.Z(); // drift time [mm]
    double zend_pos=endpoint.Z(); // drift time [mm]
    const auto radius=myOptions.radius_initial_track_region; // mm
    const auto density=1.0; // points/mm for arc
    const auto narc=(int)(TMath::Pi()*radius*density);

    auto point=TVector2(zstart_pos, sstart_pos); // position
    auto track=TVector2(zend_pos-zstart_pos, send_pos-sstart_pos); // direction

    for(auto ipoint=0; ipoint<=narc; ipoint++) {
      auto p=point + radius*(track.Rotate(TMath::Pi()*(0.5+(double)(ipoint)/narc)).Unit());
      result.SetPoint(result.GetN(), p.X(), p.Y());
    }
    for(auto ipoint=0; ipoint<=narc; ipoint++) {
      auto p=point + track + radius*(track.Rotate(TMath::Pi()*(-0.5+(double)(ipoint)/narc)).Unit());
      result.SetPoint(result.GetN(), p.X(), p.Y());
    }
    return result;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // creates list of 2D cuts (TGraph objects) for a given UZ/VZ/WZ projection
  // and Track3D collection
  std::vector<TGraph> initializeCutRegion2DList(Track3D *aTrack, int dir) {
    std::vector<TGraph> result;
    if(!aTrack) return result;
    for(auto &aSeg: aTrack->getSegments()) {
      auto gr = initializeCutRegion2D(aSeg, dir);
      if(gr.GetN()==0) continue;
      result.push_back(gr);
    }
    return result;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void restoreOriginalRefHistogramsInMM() {
    myRefNpoints=0;
    // update existing histograms or create new ones
    if(myRefHistosInMM.size()==myRefHistosInMM_orig.size()) {
      int dir=0;
      for(auto &it: myRefHistosInMM_orig) {
	myRefNpoints += countNonEmptyBins(it);
	it->Copy(*myRefHistosInMM[dir]);
	dir++;
      }
    } else if(myRefHistosInMM.size()==0) {
      for(auto &it: myRefHistosInMM_orig) {
	myRefNpoints += countNonEmptyBins(it);
	TH2D* h=(TH2D*)(it->Clone(Form("%s_copy",it->GetName())));
	if(h) h->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
	myRefHistosInMM.push_back(h);
      }
    }
  }

public:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // current reference histograms will be replaced by their copy
  // with applied 2D cuts around Track3D collections corresponding
  // to a given set of parameters (e.g. initial starting point, final fit results)
  void cropRefHistogramsInMM(const int npar, const double *par) {
    // sanity check
    if(!myOptions.use_initial_track_region || myOptions.radius_initial_track_region<=0.0) return; // nothing to do
    if (npar != myNparams || par==NULL) {
      std::cout << __FUNCTION__ << " Wrong input NPAR or PAR value!" << std::endl << std::flush;
      exit(1);
    }
    Track3D aTrack = calculateTrack3D(npar, par); // set track collection for 2D cuts
    restoreOriginalRefHistogramsInMM();
    auto dir=0;
    myRefNpoints=0;
    for(auto &it: myRefHistosInMM) {
      if(!it) {
	std::cout << __FUNCTION__ << ": Wrong pointer of external UVW reference histogram!" << std::endl << std::flush;
	exit(1);
      }
      // crop reference histograms to the region of interest around initial/seed tracks
      auto cuts = initializeCutRegion2DList(&aTrack, dir);
      cropCutRegion2D(it, cuts);
      myRefNpoints += countNonEmptyBins(it);
      dir++;
    }
    std::cout << __FUNCTION__ << ": There are " << myRefNpoints << " non-empty bins in REFERENCE histograms to be fitted." << std::endl;
  }

private:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void setRefHistogramsInMM(const std::vector<TH2D*> &reference_data) {
    // sanity check
    if( reference_data.size()!=3 ) {
      std::cout << __FUNCTION__<<": Wrong number of external UVW reference histograms!" << std::endl << std::flush;
      exit(1);
    }
    myRefHistosInMM_orig.clear();
    for(auto &it: reference_data) {
      if(!it) {
	std::cout << __FUNCTION__ << ": Wrong pointer of external UVW reference histogram!" << std::endl << std::flush;
	exit(1);
      }
      myRefHistosInMM_orig.push_back(it);
    }
    restoreOriginalRefHistogramsInMM();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void setRefHistogramsInMM(const std::vector<std::shared_ptr<TH2D> > &reference_data) {
    decltype(myRefHistosInMM) aHistVec;
    for(auto &it: reference_data) {
      aHistVec.push_back(it.get());
    }
    setRefHistogramsInMM(aHistVec);
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void setCalculators(const std::shared_ptr<StripResponseCalculator> &aResponseCalc,
		      const std::shared_ptr<IonRangeCalculator> &aRangeCalc) {
    // sanity check
    if( !aRangeCalc->IsOK() ) {
      std::cout << __FUNCTION__ << ": Wrong ion range calcualtor!" << std::endl << std::flush;
      exit(1);
    }
    myRangeCalc=aRangeCalc;
    myResponseCalc=aResponseCalc;
    myGeometryPtr=myResponseCalc->getGeometryPtr();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void setParticleList(const std::vector<pid_type> &aParticleList) {
    // sanity check
    if( aParticleList.size()==0 || aParticleList.size()>3 ) {
      std::cout << __FUNCTION__ << ": Wrong number of particles in RECO hypothesis!" << std::endl << std::flush;
      exit(1);
    }
    myParticleList=aParticleList;
    myNparams = 3 * ( myParticleList.size() + 1 ) + 1;
    myTrackFit=Track3D();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // computes Track3D collection corresponding to a given set of fitted parameters
  Track3D calculateTrack3D(const int npar, const double *par) {
    // sanity check
    if (npar != myNparams || par==NULL) {
      std::cout << __FUNCTION__ << " Wrong input NPAR or PAR value!" << std::endl << std::flush;
      exit(1);
    }
    Track3D aTrack;
    for(auto itrack=0; itrack<myParticleList.size(); itrack++) {
      TrackSegment3D aSeg;
      aSeg.setGeometry(myGeometryPtr);
      aSeg.setStartEnd(TVector3(par[1], par[2], par[3]), TVector3(par[4+itrack*3], par[5+itrack*3], par[6+itrack*3]));
      aSeg.setPID(myParticleList[itrack]);
      aTrack.addSegment(aSeg);
    }
    return aTrack;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // intializes all global scale and Track3D collection for a given set of fit parameters
  void initializeTrack3D(const int npar, const double *par) {
    // sanity check
    if (npar==0 || par==NULL) {
      std::cout << __FUNCTION__ << " Wrong input NPAR or PAR value!" << std::endl << std::flush;
      exit(1);
    }
    myScale=par[0]; // set global scaling factor for dE/dx
    myTrackFit=calculateTrack3D(npar, par); // set track collection
  }

public:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  HypothesisFit(const std::vector<std::shared_ptr<TH2D> > &reference_data,
		const std::shared_ptr<StripResponseCalculator> &aResponseCalc,
		const std::shared_ptr<IonRangeCalculator> &aRangeCalc,
		const std::vector<pid_type> &aParticleList,
		const FitOptionType &aOption)
    : myOptions(aOption)
  {
    myOptions.photonUnitVec_DET_LAB=myOptions.photonUnitVec_DET_LAB.Unit(); // protection
    myOptions.photonEnergyInMeV_LAB=fabs(myOptions.photonEnergyInMeV_LAB); // protection
    setRefHistogramsInMM(reference_data);
    setCalculators(aResponseCalc, aRangeCalc);
    setParticleList(aParticleList);
    initializeFitHistogramsInMM();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  HypothesisFit(const std::vector<TH2D*> &reference_data,
		const std::shared_ptr<StripResponseCalculator> &aResponseCalc,
		const std::shared_ptr<IonRangeCalculator> &aRangeCalc,
		const std::vector<pid_type> &aParticleList,
		const FitOptionType &aOption)
    : myRefHistosInMM(reference_data), myOptions(aOption)
  {
    myOptions.photonUnitVec_DET_LAB=myOptions.photonUnitVec_DET_LAB.Unit(); // protection
    myOptions.photonEnergyInMeV_LAB=fabs(myOptions.photonEnergyInMeV_LAB); // protection
    setRefHistogramsInMM(reference_data);
    setCalculators(aResponseCalc, aRangeCalc);
    setParticleList(aParticleList);
    initializeFitHistogramsInMM();
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  int getNparams() const { return myNparams; }
  
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  unsigned long getRefNpoints() const { return myRefNpoints; }
  
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // returns clone of underlying reference histograms
  std::vector<std::shared_ptr<TH2D> > getRefHistogramsInMM() {
    std::vector<std::shared_ptr<TH2D> > aHistVec;
    for(auto &it: myRefHistosInMM) {
      TH2D* hist=(TH2D*)(it->Clone());
      if(hist) hist->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
      aHistVec.push_back( std::shared_ptr<TH2D>(hist) );
    }
    return aHistVec;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // returns clone of undeerlying fit/test hypothesis histograms
  std::vector<std::shared_ptr<TH2D> > getFitHistogramsInMM() {
    std::vector<std::shared_ptr<TH2D> > aHistVec;
    for(auto &it: myFitHistosInMM) {
      TH2D* hist=(TH2D*)(it->Clone());
      if(hist) hist->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
      aHistVec.push_back( std::shared_ptr<TH2D>(hist) );
    }
    return aHistVec;
  }

private:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // helper function for penalty Chi^2 calculation
  TLorentzVector getTotalP4_DET_LAB() {
    TLorentzVector sumP4_DET_LAB{0,0,0,0};

    // loop over hypothesis tracks
    const int ntracks = myTrackFit.getSegments().size();
    auto list = myTrackFit.getSegments();
    for(auto & track: list) {
      auto origin=track.getStart(); // mm
      auto length=track.getLength(); // mm
      auto unit_vec=track.getTangent();
      auto pid=track.getPID();
      auto Ekin_LAB=myRangeCalc->getIonEnergyMeV(pid, length); // MeV, Ekin should be equal to Bragg curve integral
      auto mass=myRangeCalc->getIonMassMeV(pid); // MeV/c^2, isotopic mass
      auto p_LAB=sqrt(Ekin_LAB*(Ekin_LAB+2*mass)); // MeV/c
      sumP4_DET_LAB += TLorentzVector(p_LAB*unit_vec, Ekin_LAB+mass); // [MeV/c, MeV/c, MeV/c, MeV/c^2]
    }
    return sumP4_DET_LAB;
  }
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // helper function for penalty Chi^2 calculation
  double getChi2PenaltyTerm_momentumLAB() {
    double result=0.0;
    if(!myOptions.use_penalty_momentum_LAB) return result;

    auto delta_momentum_LAB=getTotalP4_DET_LAB().Mag(); // MeV/c
    result=penaltyFunc((delta_momentum_LAB-myOptions.delta_momentum_max)/myOptions.scale_penalty_momentum);

#if(DEBUG_CHI2)
    std::cout<<__FUNCTION__<<": Chi2 penalty term for momentum conservation in LAB = "<<result<<std::endl;
#endif
    return result;
  }
  ///////////////////////////////
  ///////////////////////////////
  // helper function for penalty Chi^2 calculation
  // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
  TVector3 getBetaVectorOfCMS(double nucleusMassInMeV) {
    return getBetaOfCMS(nucleusMassInMeV)*myOptions.photonUnitVec_DET_LAB;
  }
  ///////////////////////////////
  ///////////////////////////////
  // helper function for penalty Chi^2 calculation
  // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
  double getBetaOfCMS(double nucleusMassInMeV) {
    return myOptions.photonEnergyInMeV_LAB/(myOptions.photonEnergyInMeV_LAB+nucleusMassInMeV);
  }
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // helper function for penalty Chi^2 calculation
  double getChi2PenaltyTerm_momentumCMS() {
    double result=0.0;
    if(!myOptions.use_penalty_momentum_CMS) return result;

    // loop over hypothesis tracks and determine parent nucleus
    auto parentPID=pid_type::UNKNOWN;
    const int ntracks = myTrackFit.getSegments().size();
    auto list = myTrackFit.getSegments();
    switch(ntracks) {
    case 2:
      if(list.front().getPID()==pid_type::ALPHA &&
	 list.back().getPID()==pid_type::CARBON_12) {
	parentPID=pid_type::OXYGEN_16;
	break;
      }
      if(list.front().getPID()==pid_type::ALPHA &&
	 list.back().getPID()==pid_type::CARBON_13) {
	parentPID=pid_type::OXYGEN_17;
	break;
      }
      if(list.front().getPID()==pid_type::ALPHA &&
	 list.back().getPID()==pid_type::CARBON_14) {
	parentPID=pid_type::OXYGEN_18;
	break;
      }
      if(list.front().getPID()==pid_type::PROTON &&
	 list.back().getPID()==pid_type::NITROGEN_15) {
	parentPID=pid_type::OXYGEN_16;
	break;
      }
    case 3:
     if(list.at(0).getPID()==pid_type::ALPHA &&
	list.at(1).getPID()==pid_type::ALPHA &&
	list.at(2).getPID()==pid_type::ALPHA) {
       parentPID=pid_type::CARBON_12;
       break;
      }
    default: return result;  // only selected 2-prong or 3-prongs reactions are supported
    };

    // get total P4 in DET/LAB frame
    auto sumP4_DET_LAB=getTotalP4_DET_LAB(); // [MeV/c, MeV/c, MeV/c, MeV/c^2]
    auto sumP4_DET_CMS=TLorentzVector(sumP4_DET_LAB);
    auto parentMassGroundState=myRangeCalc->getIonMassMeV(parentPID); // MeV/c^2, isotopic mass
    TVector3 beta_DET_LAB;
    if(myOptions.use_nominal_beam_energy) {
      beta_DET_LAB=getBetaVectorOfCMS(parentMassGroundState); // assume nominal direction and nominal gamma beam energy
    } else {
      const double photon_E_LAB=sumP4_DET_LAB.E()-parentMassGroundState; // reconstructed gamma beam energy in LAB
      beta_DET_LAB=getBetaVectorOfCMS(parentMassGroundState).Unit()*(photon_E_LAB/(photon_E_LAB+parentMassGroundState));
    }
    // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
    sumP4_DET_CMS.Boost(-1.0*beta_DET_LAB);
    auto delta_momentum_CMS=sumP4_DET_CMS.Vect().Mag(); // MeV/c
    result=penaltyFunc((delta_momentum_CMS-myOptions.delta_momentum_max)/myOptions.scale_penalty_momentum);

#if(DEBUG_CHI2)
    std::cout<<__FUNCTION__<<": Chi2 penalty term for momentum conservation in CMS = "<<result<<std::endl;
#endif
    return result;
  }
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  double getChi2PenaltyTerm_alphaLength() {
    double result=0.0;
    if(!myOptions.use_penalty_alpha_length) return result;

    // loop over hypothesis tracks
    TrackSegment3DCollection list = myTrackFit.getSegments();
    for(auto & track: list) {
      auto length=track.getLength(); // mm
      auto pid=track.getPID();
      if(pid==pid_type::ALPHA) {
	result += penaltyFunc((length-myOptions.alpha_length_max)/myOptions.scale_penalty_alpha_length) +
	  penaltyFunc((-length+myOptions.alpha_length_min)/myOptions.scale_penalty_alpha_length);
      }
    }

#if(DEBUG_CHI2)
    std::cout<<__FUNCTION__<<": Chi2 penalty term for ALPHA length range = "<<result<<std::endl;
#endif
    return result;
  }
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  double getChi2PenaltyTerm_carbonLength() {
    double result=0.0;
    if(!myOptions.use_penalty_carbon_length) return result;

    // loop over hypothesis tracks
    TrackSegment3DCollection list = myTrackFit.getSegments();
    for(auto & track: list) {
      auto length=track.getLength(); // mm
      auto pid=track.getPID();
      if(pid==pid_type::CARBON_12 || pid==pid_type::CARBON_13 || pid==pid_type::CARBON_14) {
	result += penaltyFunc((length-myOptions.carbon_length_max)/myOptions.scale_penalty_carbon_length) +
	  penaltyFunc((-length+myOptions.carbon_length_min)/myOptions.scale_penalty_carbon_length);
      }
    }

#if(DEBUG_CHI2)
    std::cout<<__FUNCTION__<<": Chi2 penalty term for CARBON length range = "<<result<<std::endl;
#endif
    return result;
  }

public:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  double getChi2() {
    const int nhist=myRefHistosInMM.size();
    const double expected_rms_noise=myOptions.expectedAdcNoiseRMS;// [ADC units] - typical rms of noise from pedestal runs @ 12.5 MHz
    double sumChi2=0.0;
    auto ndf=0L;
    for(auto ihist=0; ihist<nhist; ihist++) {
      for(auto ix=1; ix<=myRefHistosInMM[ihist]->GetNbinsX(); ix++) {
  	for(auto iy=1; iy<=myRefHistosInMM[ihist]->GetNbinsY(); iy++) {
  	  auto ibin=myRefHistosInMM[ihist]->GetBin(ix, iy);
  	  auto val1=myRefHistosInMM[ihist]->GetBinContent(ibin); // [ADC units] - observed (experimental data)
  	  auto val2=myFitHistosInMM[ihist]->GetBinContent(ibin); // [ADC units] - expected (fitted model)
  	  auto err1=myRefHistosInMM[ihist]->GetBinError(ibin); // [ADC units]
  	  auto err2=myFitHistosInMM[ihist]->GetBinError(ibin); // [ADC units]
  	  if(err1 || err2) { // skip empty bins
	    sumChi2 += (val1-val2)*(val1-val2)/expected_rms_noise/expected_rms_noise; // unitless
	    ndf++;
	  }
  	}
      }
    }
#if(DEBUG_CHI2)
    std::cout<<__FUNCTION__<<": Sum of chi2 from "<<nhist<<" histograms = "<<sumChi2<<", ndf="<<ndf<<std::endl;
#endif
    sumChi2 *= 1.0 +
               getChi2PenaltyTerm_momentumLAB() +
	       getChi2PenaltyTerm_momentumCMS() +
	       getChi2PenaltyTerm_alphaLength() +
               getChi2PenaltyTerm_carbonLength();
    return sumChi2 * 0.5; // NOTE: for FUMILI minimizer the returned function should be: 0.5 * chi^2
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // implementation of the function to be minimized
  double operator() (const double *par) {

    // convert params to new Track3D collection
    initializeTrack3D(myNparams, par);

    // convert current Track3D collection to new set of TH2D UVW projections
    fillFitHistogramsInMM();

    // calcualte (reduced) CHI2 for new TH2D histograms
    return getChi2();
  }
};

// _______________________________________
//
// Adds visualization of a vertex to the specified TPad with existing world coordinate system:
// * x = Z_DET [mm] - position along drift electric field
// * y = U, V or W [mm] - postion along a given strip direction wrt (X=0,Y=0)_DET
// Also sets line and marker attributes such that they cannot be edited.
//
void DrawVertexOnUVWProjection(TPad *tpad, TrackSegment3D &aSeg, int dir, int color=kMagenta) {
  if(!tpad) return;
  if(dir<definitions::projection_type::DIR_U || dir>definitions::projection_type::DIR_W) return;
  const auto vertex_marker=kCircle;
  const auto vertex_marker2=kPlus;
  const auto vertex_color=color;
  const auto vertex_size=2;
  const auto vtx=aSeg.getStart();
  const auto geo=aSeg.getGeometry();
  if(!geo) return;
  auto err=false;
  double strip_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), dir, err); // strip position [mm] for a given XY_DET position
  if(err) return;
  double z_pos=vtx.Z(); // drift time [mm]
  tpad->cd();
  auto *m1=new TMarker(z_pos, strip_pos, vertex_marker);
  m1->SetMarkerColor(vertex_color);
  m1->SetMarkerSize(vertex_size);
  m1->Draw();
  m1->SetBit(kCannotPick); // prevents editing
  auto *m2=new TMarker(z_pos, strip_pos, vertex_marker2);
  m2->SetMarkerColor(vertex_color);
  m2->SetMarkerSize(vertex_size);
  m2->Draw();
  m2->SetBit(kCannotPick); // prevents editing
  tpad->Modified();
}
// _______________________________________
//
// Adds visualization of a track endpoint to the specified TPad with existing world coordinate system:
// * x = Z_DET [mm] - position along drift electric field
// * y = U, V or W [mm] - postion along a given strip direction wrt (X=0,Y=0)_DET
// Also sets line and marker attributes such that they cannot be edited.
//
void DrawEndpointOnUVWProjection(TPad *tpad, TrackSegment3D &aSeg, int dir, int color=kBlack) {
  if(!tpad) return;
  if(dir<definitions::projection_type::DIR_U || dir>definitions::projection_type::DIR_W) return;
  const auto line_color=color;
  const auto line_width=2;
  const auto line_style=kSolid;
  const auto endpoint_marker=kCircle; // open circle
  const auto endpoint_color=color;
  const auto endpoint_size=2;
  const auto vtx=aSeg.getStart();
  const auto endpoint=aSeg.getEnd();
  const auto geo=aSeg.getGeometry();
  if(!geo) return;
  auto err=false;
  double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), dir, err); // strip position [mm] for a given XY_DET position
  double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), dir, err); // strip position [mm] for a given XY_DET position
  if(err) return;
  double zstart_pos=vtx.Z(); // drift time [mm]
  double zend_pos=endpoint.Z(); // drift time [mm]
  tpad->cd();
  auto *m=new TMarker(zend_pos, send_pos, endpoint_marker);
  m->SetMarkerColor(endpoint_color);
  m->SetMarkerSize(endpoint_size);
  m->Draw();
  m->SetBit(kCannotPick); // prevents editing
  auto *l=new TLine(zstart_pos, sstart_pos, zend_pos, send_pos);
  l->SetLineColor(line_color);
  l->SetLineWidth(line_width);
  l->SetLineStyle(line_style);
  l->Draw();
  l->SetBit(kCannotPick); // prevents editing
  tpad->Modified();
}
// _______________________________________
//
// Adds visualization of all tracks to the specified TPad with existing world coordinate system:
// * x = Z_DET [mm] - position along drift electric field
// * y = U, V or W [mm] - postion along a given strip direction wrt (X=0,Y=0)_DET
// Also sets line and marker attributes such that they cannot be edited.
//
void DrawTracksOnUVWProjection(TPad *tpad, Track3D *aTrack, int dir, bool useUniformColor=false) {
  if(!tpad || !aTrack || aTrack->getSegments().size()==0) return;
  auto vertex_color = (useUniformColor ? kWhite : kMagenta);
  std::vector<int> track_color={ kRed, kGreen, kAzure+7}; // kBlue };
  DrawVertexOnUVWProjection(tpad, aTrack->getSegments().at(0), dir, vertex_color);
  auto track_index=0;
  for(auto & aSeg: aTrack->getSegments()) {
    auto color = (useUniformColor ? kWhite : track_color[track_index % track_color.size()] );
    DrawEndpointOnUVWProjection(tpad, aSeg, dir, color);
    track_index++;
  }
}
// _______________________________________
//
// Adds arbitrary comment using NDC coordinates of the pad.
//
void DrawTLatexOnUVWProjection(TPad *tpad, const double xNDC, const double yNDC, const char *comment="") {
  if(!tpad) return;
  const auto text_color=kBlack;
  const auto text_font=42; // Arial normal, precision=2 (scalable, size in NDC)
  const auto text_size=0.05; // size in NDC
  tpad->cd();
  auto *t=new TLatex(xNDC, yNDC, comment);
  t->SetTextAlign(22); // horizontal=centered, vertical=middle
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
// Define simple data structure for N-prong fit (1<=N<=3)
//
typedef struct { Float_t eventId, runId, ntracks,
    energy[3]={0,0,0}, // MeV
    pid[3]={0,0,0},
    chi2,
    ndf,
    status,
    ncalls,
    lengthTrue[3]={0,0,0}, // mm
    phiTrue[3]={0,0,0}, // rad
    cosThetaTrue[3]={0,0,0},
    scaleTrue, // ADC/MeV
    xVtxTrue, yVtxTrue, zVtxTrue, // mm
    xEndTrue[3]={0,0,0}, yEndTrue[3]={0,0,0}, zEndTrue[3]={0,0,0}, // mm
    lengthReco[3]={0,0,0}, // mm
    phiReco[3]={0,0,0}, // rad
    cosThetaReco[3]={0,0,0},
    scaleReco, // ADC/MeV
    xVtxReco, yVtxReco, zVtxReco, // mm
    xEndReco[3]={0,0,0}, yEndReco[3]={0,0,0}, zEndReco[3]={0,0,0}, // mm
    scaleRecoErr, // ADC/MeV
    xVtxRecoErr, yVtxRecoErr, zVtxRecoErr, // mm
    xEndRecoErr[3]={0,0,0}, yEndRecoErr[3]={0,0,0}, zEndRecoErr[3]={0,0,0}, // mm
    initScaleDeviation, // intial conditions: fabs(difference from TRUE) [ADC/MeV]
    initVtxDeviation, // initial conditions: radius from TRUE vertex position [mm]
    initEndDeviation[3]={0,0,0}; // initial conditions: radius from TRUE endpoint position [mm]
} FitDebugData3prong;
/////////////////////////

// _______________________________________
//
// Divide existing TCanvas into multiple TPads to visualize tracks (initial and/or fitted)
// separately for each U/V/W projection.
//
void DrawFitResults(TCanvas *tcanvas, // input TCanvas
		    std::vector<std::shared_ptr<TH2D> > *refHistogramsInMM, // REF histogram used in the fit
		    std::vector<std::shared_ptr<TH2D> > *fitHistogramsInMM=NULL, // result of the fit to be displayed
		    Track3D *fitTrack=NULL, // result of the fit to be displayed
		    Track3D *initialTrack=NULL, // intitial conditions used in the fit
		    FitDebugData3prong *fit_debug_data=NULL) { // optional debug info to be displayed
  if(!tcanvas || !refHistogramsInMM || refHistogramsInMM->size()>3) return;
  const auto hasFitHistos=(fitHistogramsInMM && fitHistogramsInMM->size()==refHistogramsInMM->size());
  const auto hasFitTrack=(fitTrack && fitTrack->getSegments().size()>0);
  const auto hasInitTrack=(initialTrack && initialTrack->getSegments().size()>0);
  const auto hasFitInfo=(fit_debug_data!=NULL);
  const auto pad_width=500;
  const auto pad_right_margin=0.15;
  const auto pad_left_margin=0.15;
  const auto npadx=refHistogramsInMM->size();
  const auto npady=1+hasFitHistos*2; // REF + (FIT + DIFFERENCE)
  tcanvas->Clear();
  tcanvas->SetWindowSize(pad_width*npadx, pad_width*npady);
  tcanvas->Divide(npadx,npady);

  int idir;
#if(DRAW_AUTO_ZOOM)
  // find bounding boxes per UVW projection from fitted and initial tracks
  double zDET_min=1E30; // mm
  double zDET_max=-1E30; // mm
  std::vector<double> posUVW_min{1E30, 1E30, 1E30}; // mm
  std::vector<double> posUVW_max{-1E30, -1E30, -1E30};; // mm
  if(hasInitTrack) {
    for(auto &aSeg: initialTrack->getSegments()) {
      zDET_min=std::min(zDET_min, std::min(aSeg.getStart().Z(), aSeg.getEnd().Z()));
      zDET_max=std::max(zDET_max, std::max(aSeg.getStart().Z(), aSeg.getEnd().Z()));
      auto err=false;
      const auto vtx=aSeg.getStart();
      const auto endpoint=aSeg.getEnd();
      const auto geo=aSeg.getGeometry();
      if(!geo) continue;
      for(int idir=definitions::projection_type::DIR_U; idir<=definitions::projection_type::DIR_W; idir++) {
	double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), idir, err); // strip position [mm] for a given XY_DET position
	double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), idir, err); // strip position [mm] for a given XY_DET position
	posUVW_min[idir]=std::min(posUVW_min[idir], std::min(sstart_pos, send_pos));
	posUVW_max[idir]=std::max(posUVW_max[idir], std::max(sstart_pos, send_pos));
      }
    }
  }
  if(hasFitTrack) {
    for(auto &aSeg: fitTrack->getSegments()) {
      zDET_min=std::min(zDET_min, std::min(aSeg.getStart().Z(), aSeg.getEnd().Z()));
      zDET_max=std::max(zDET_max, std::max(aSeg.getStart().Z(), aSeg.getEnd().Z()));
      auto err=false;
      const auto vtx=aSeg.getStart();
      const auto endpoint=aSeg.getEnd();
      const auto geo=aSeg.getGeometry();
      if(!geo) continue;
      for(int idir=definitions::projection_type::DIR_U; idir<=definitions::projection_type::DIR_W; idir++) {
	double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), idir, err); // strip position [mm] for a given XY_DET position
	double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), idir, err); // strip position [mm] for a given XY_DET position
	posUVW_min[idir]=std::min(posUVW_min[idir], std::min(sstart_pos, send_pos));
	posUVW_max[idir]=std::max(posUVW_max[idir], std::max(sstart_pos, send_pos));
      }
    }
  }
#endif

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
#if(DRAW_AUTO_ZOOM)
  idir=definitions::projection_type::DIR_U;
  for(auto &it: *refHistogramsInMM) {
    auto xcenter=0.5*(zDET_min+zDET_max);
    auto ycenter=0.5*(posUVW_min[idir]+posUVW_max[idir]);
    auto width=std::max(zDET_max-zDET_min, posUVW_max[idir]-posUVW_min[idir]); // common width to keep aspect ratio
    it->GetXaxis()->SetRangeUser( xcenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, xcenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    it->GetYaxis()->SetRangeUser( ycenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, ycenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    //    it->GetXaxis()->SetRangeUser( zDET_min-DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min), zDET_max+DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min) );
    //    it->GetYaxis()->SetRangeUser( posUVW_min[idir]-DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]), posUVW_max[idir]+DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]) );
    idir++;
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
    h->SetName(Form("ref_%s",h->GetName()));
    h->SetTitle(Form("REF %s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
    // draw initial conditions
    if(hasInitTrack) DrawTracksOnUVWProjection((TPad*)gPad, initialTrack, idir, true); // white markers/lines
    // draw fit results
    if(hasFitTrack) DrawTracksOnUVWProjection((TPad*)gPad, fitTrack, idir, false); // multi-color markrers/lines
    ipad++;
    idir++;
  }

  // draw fitted + residual histograms
  std::vector<TH2D*> residualHistogramsInMM;
  if(hasFitHistos) {
#if(DRAW_SAME_SCALE)
    // first pass to get min/max values
    idir=definitions::projection_type::DIR_U;
    auto fit_min=1E30;
    auto fit_max=-1E30;
    for(auto &it: *fitHistogramsInMM) {
      fit_min=std::min(fit_min, it->GetBinContent(it->GetMinimumBin())); // true MIN of histogram
      fit_max=std::max(fit_max, it->GetBinContent(it->GetMaximumBin())); // true MAX of histogram
      idir++;
    }
    for(auto &it: *fitHistogramsInMM) {
      it->SetMinimum( fit_min-DRAW_SAME_SCALE_MARGIN*(fit_max-fit_min) );
      it->SetMaximum( fit_max+DRAW_SAME_SCALE_MARGIN*(fit_max-fit_min) );
    }
#endif
#if(DRAW_AUTO_ZOOM)
  idir=definitions::projection_type::DIR_U;
  for(auto &it: *fitHistogramsInMM) {
    auto xcenter=0.5*(zDET_min+zDET_max);
    auto ycenter=0.5*(posUVW_min[idir]+posUVW_max[idir]);
    auto width=std::max(zDET_max-zDET_min, posUVW_max[idir]-posUVW_min[idir]); // common width to keep aspect ratio
    it->GetXaxis()->SetRangeUser( xcenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, xcenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    it->GetYaxis()->SetRangeUser( ycenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, ycenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    //    it->GetXaxis()->SetRangeUser( zDET_min-DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min), zDET_max+DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min) );
    //    it->GetYaxis()->SetRangeUser( posUVW_min[idir]-DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]), posUVW_max[idir]+DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]) );
    idir++;
  }
#endif
    // draw fit histograms
    idir=definitions::projection_type::DIR_U;
    ipad=npadx+1;
    for(auto &it: *fitHistogramsInMM) {
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
      h->SetName(Form("fit_%s",h->GetName()));
      h->SetTitle(Form("FIT %s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
      if(hasFitTrack) DrawTracksOnUVWProjection((TPad*)gPad, fitTrack, idir);
      ipad++;
      idir++;
      residualHistogramsInMM.push_back((TH2D*)h->Clone());
    }
    // calculate residual (FIT-REF) histograms
    idir=definitions::projection_type::DIR_U;
    for(auto &it: residualHistogramsInMM) {
      it->Add(refHistogramsInMM->at(idir).get(), -1.0); // subtract REFERENCE histogram from FIT histogram
      idir++;
    }
#if(DRAW_SAME_SCALE)
    // first pass to set common min/max value
    idir=definitions::projection_type::DIR_U;
    auto diff_min=1E30;
    auto diff_max=-1E30;
    for(auto &it: residualHistogramsInMM) {
      diff_min=std::min(diff_min, it->GetBinContent(it->GetMinimumBin())); // true MIN of histogram
      diff_max=std::max(diff_max, it->GetBinContent(it->GetMaximumBin())); // true MAX of histogram
      idir++;
    }
    for(auto &it: residualHistogramsInMM) {
      it->SetMinimum( diff_min-DRAW_SAME_SCALE_MARGIN*(diff_max-diff_min) );
      it->SetMaximum( diff_max+DRAW_SAME_SCALE_MARGIN*(diff_max-diff_min) );
    }
#endif
#if(DRAW_AUTO_ZOOM)
  idir=definitions::projection_type::DIR_U;
  for(auto &it: residualHistogramsInMM) {
    auto xcenter=0.5*(zDET_min+zDET_max);
    auto ycenter=0.5*(posUVW_min[idir]+posUVW_max[idir]);
    auto width=std::max(zDET_max-zDET_min, posUVW_max[idir]-posUVW_min[idir]); // common width to keep aspect ratio
    it->GetXaxis()->SetRangeUser( xcenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, xcenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    it->GetYaxis()->SetRangeUser( ycenter-(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width, ycenter+(1.0+DRAW_AUTO_ZOOM_MARGIN)*0.5*width );
    //    it->GetXaxis()->SetRangeUser( zDET_min-DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min), zDET_max+DRAW_AUTO_ZOOM_MARGIN*(zDET_max-zDET_min) );
    //    it->GetYaxis()->SetRangeUser( posUVW_min[idir]-DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]), posUVW_max[idir]+DRAW_AUTO_ZOOM_MARGIN*(posUVW_max[idir]-posUVW_min[idir]) );
    idir++;
  }
#endif
    // draw residual (FIT-REF) histograms
    idir=definitions::projection_type::DIR_U;
    ipad=2*npadx+1;
    for(auto &it: residualHistogramsInMM) {
      //      it->Add(refHistogramsInMM->at(idir).get(), -1.0); // subtract REFERENCE histogram from FIT histogram
      tcanvas->cd(ipad);
      if(gPad) {
	gPad->SetLogx(false);
	gPad->SetLogy(false);
	gPad->SetLogz(false);
	gPad->SetLeftMargin(pad_left_margin);
	gPad->SetRightMargin(pad_right_margin);
      }
      auto h=(TH2D*)(it->DrawClone("COLZ1")); // skip zero
      h->SetDirectory(0);
      h->SetStats(false);
      h->SetName(Form("diff_%s",h->GetName()));
      h->SetTitle(Form("RESIDUAL %s;%s;%s;%s",h->GetTitle(),h->GetXaxis()->GetTitle(),h->GetYaxis()->GetTitle(),h->GetZaxis()->GetTitle()));
      if(hasFitTrack) DrawTracksOnUVWProjection((TPad*)gPad, fitTrack, idir);
      if(hasFitInfo) DrawTLatexOnUVWProjection((TPad*)gPad, 0.6, 0.2,
					       Form("#splitline{#chi^{2} = %.5lg}{ndf = %ld}",
						    (double)fit_debug_data->chi2,
						    (long)fit_debug_data->ndf));
      ipad++;
      idir++;
    }
  }
  tcanvas->Update();
  tcanvas->Modified();
}

// _______________________________________
//
// This function: fits N (1<=N<=3) straight pseudo-tracks.having common vertex
// It returns CHI2 of the fit.
// It also fills FitDebugData3prong structure.
//
double fit_Nprong(std::vector<std::shared_ptr<TH2D> > &referenceHistosInMM,
		  std::shared_ptr<GeometryTPC> geo,
		  std::shared_ptr<StripResponseCalculator> calcResponse,
		  std::shared_ptr<IonRangeCalculator> calcRange,
		  std::vector<pid_type> pidList, // determines hypothesis and number of tracks
		  std::vector<double> initialParameters, // starting points of parametric fit
		  FitOptionType &fit_options,
		  FitDebugData3prong &fit_debug_data,
		  std::vector<std::shared_ptr<TH2D> > *fitted_histograms_coll=NULL, // optional pointer to a vector
                                                                                    // of TH2D pointers for storing the
                                                                                    // final fit result in UVW projections
		  std::vector<std::set<int> > fixedParametersMask=std::vector<std::set<int> >()) { // optional list of
		                                                                                   // parameters to be frozen
		                                                                                   // at STEP1, STEP2, etc
  // initilize fitter
  //
  HypothesisFit myFit(referenceHistosInMM, calcResponse, calcRange, pidList, fit_options); //beamEnergyInMeV, beamDir);
  const int npar=myFit.getNparams();
  ROOT::Fit::Fitter fitter;
  ROOT::Math::Functor fcn(myFit, npar); // this line must be called after fixing all local charge density function parameters

  if(npar!=initialParameters.size()) {
    std::cout << __FUNCTION__ << " Wrong number of parameters for HypothesisFit class!" << std::endl << std::flush;
    exit(1);
  }
  double pStart[npar];
  int index=0;
  for(auto &it: initialParameters) {
    pStart[index] = it;
    index++;
  }
  // optionally consider only hits inside slot-shaped regions of interest around initial tracks in all UZ/VZ/WZ projections
  // NOTE: such regions of interest are independent from hit cluster settings
  myFit.cropRefHistogramsInMM(npar, pStart);
  fitter.SetFCN(fcn, pStart, myFit.getRefNpoints(), true);
  ////// DEBUG
  // std::cout << __FUNCTION__ << " After fitter.SetFCN" << std::endl << std::flush;
  // auto coll=myFit.getRefHistogramsInMM();
  // auto c=new TCanvas("c","c",300*coll.size(), 300);
  // c->Divide(coll.size(), 1);
  // c->cd();
  // int ipad=1;
  // for(auto &it: coll) {
  //   c->cd(ipad);
  //   it->DrawClone("COLZ");
  //   ipad++;
  //   gPad->Update();
  //   gPad->Modified();
  // }
  // c->Print("c.root");
  // c->Print("c.pdf");
  ////// DEBUG

  // compute better ADC scaling factor
  double ref_integral=0.0; // integral of reference (data) histograms
  for(auto &it: referenceHistosInMM) { ref_integral += it->Integral(); }
  double fit_integral=0.0; // integral of initial fitted histograms
  double chi2=myFit(pStart);
  std::cout << __FUNCTION__ << ": CHI2 evaluated at INITIAL starting point =" << chi2 << std::endl;
  for(auto &it: myFit.getFitHistogramsInMM()) { fit_integral += it->Integral(); }
  double corr_factor=ref_integral/fit_integral; // correction factor for ADC scale
  std::cout << __FUNCTION__ << ": Multiplying original ADC scaling factor by " << corr_factor << std::endl;
  pStart[0]*=corr_factor;
  fitter.Config().ParSettings(0).SetValue(pStart[0]);
  chi2=myFit(pStart);
  std::cout << __FUNCTION__ << ": CHI2 evaluated at CORRECTED starting point =" << chi2 << std::endl;

  // set limits on the fitted ADC scale
  if(fit_options.maxAdcFactor<1.0) {
    std::cout << "Wrong maximal ADC scaling factor!" << std::endl;
    fitter.Config().ParSettings(0).SetLimits(0.0, 1E30);
  } else {
    fitter.Config().ParSettings(0).SetLimits(pStart[0]/fit_options.maxAdcFactor, pStart[0]*fit_options.maxAdcFactor);
  }
  std::cout << "Setting par[" << 0 << "]  limits=["
	    << fitter.Config().ParSettings(0).LowerLimit()<<", "
	    << fitter.Config().ParSettings(0).UpperLimit()<<"]" << std::endl;

  // set limits on the fitted position of track's vertex & reference_endpoint
  // to be within +/- maxDeviationInMM from initial guess values
  for (int i = 1; i < myFit.getNparams(); ++i) {
    double parmin=0.0, parmax=0.0;
    double xmin, xmax, ymin, ymax, zmin, zmax;
    std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=geo->rangeXYZ();
    switch((i-1)%3) {
    case 0: // X_DET
      parmin = std::max( xmin, pStart[i]-fit_options.maxDeviationInMM );
      parmax = std::min( xmax, pStart[i]+fit_options.maxDeviationInMM );
      break;
    case 1: // Y_DET
      parmin = std::max( ymin, pStart[i]-fit_options.maxDeviationInMM );
      parmax = std::min( ymax, pStart[i]+fit_options.maxDeviationInMM );
      break;
    case 2: // Z_DET
      parmin = std::max( zmin, pStart[i]-fit_options.maxDeviationInMM );
      parmax = std::min( zmax, pStart[i]+fit_options.maxDeviationInMM );
      break;
    default:
      std::cout << __FUNCTION__ << ": Invalid coordinate index!" << std::endl << std::flush;
      exit(1);
    };
    fitter.Config().ParSettings(i).SetLimits(parmin, parmax);
    std::cout<<"Setting par["<<i<<"]  limits=["
	     <<fitter.Config().ParSettings(i).LowerLimit()<<", "
	     <<fitter.Config().ParSettings(i).UpperLimit()<<"]"<<std::endl;
  }

  // set step sizes
  fitter.Config().ParSettings(0).SetStepSize(100); // step for ADC scaling factor [ADC/MeV]
  for (int i = 1; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).SetStepSize(0.1); // step for spatial dimensions [mm]
  }

  // set numerical tolerance
  fitter.Config().MinimizerOptions().SetTolerance(fit_options.tolerance); // default=1e-4 for MC-MC tests

  // set print debug level
  fitter.Config().MinimizerOptions().SetPrintLevel(2);

  bool ok=false;

  // perform fit either in single step, or in steps using extrnal list of frozen parameters per step
  switch(fixedParametersMask.size()) {
  case 0:
    ////////// STEP 1 - Minuit2 / Fumili2
    //
    for (int i = 0; i < myFit.getNparams(); ++i) {
      std::cout <<"Initial value of par[" << i << "]=" << fitter.Config().ParSettings(i).Value() <<" limits=["
		<< fitter.Config().ParSettings(i).LowerLimit()<<", "
		<< fitter.Config().ParSettings(i).UpperLimit()<<"]" << std::endl;
      std::cout<<"Releasing par["<<i<<"]"<<std::endl;
      fitter.Config().ParSettings(i).Release();
    }
    // for (int i = 0; i < myFit.getNparams(); ++i) {
    //   std::cout<<"Fixing par["<<i<<"]="<<fitter.Config().ParSettings(i).Value()<<std::endl;
    //   fitter.Config().ParSettings(i).Fix();
    // }
    fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
    fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Fumili2");
    fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000);
    fitter.Config().MinimizerOptions().SetMaxIterations(1000);
    fitter.Config().MinimizerOptions().SetStrategy(1);
    fitter.Config().MinimizerOptions().SetTolerance(fit_options.tolerance); // default=1e-4 for MC-MC comparison
    fitter.Config().MinimizerOptions().SetPrecision( 1e-6 ); // 1e-4 //increased to speed up calculations

    // check fit results
    ok = fitter.FitFCN();
    if (!ok) {
      std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit failed."<<std::endl;
      //    return 1;
    } else {
      std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit ok."<<std::endl;
    }
    break;

  default:
    auto istep=0;
    for(auto &mask: fixedParametersMask) {
      istep++;
      ////////// STEP N - Minuit2 / Fumili2
      //
      if(istep>1) {
	const ROOT::Fit::FitResult & result = fitter.Result();
	std::cout << ">>>>>> STEP " << istep << " out of " << fixedParametersMask.size() << " <<<<<<" << std::endl;
	result.Print(std::cout);
      }
      for (int i = 0; i < myFit.getNparams(); ++i) {
	if(istep==1) {
	  std::cout << "STEP " << istep << " out of " << fixedParametersMask.size()
		    <<": Initial value of par[" << i << "]=" << fitter.Config().ParSettings(i).Value() <<" limits=["
		    << fitter.Config().ParSettings(i).LowerLimit()<<", "
		    << fitter.Config().ParSettings(i).UpperLimit()<<"]" << std::endl;
	} else {
	  const ROOT::Fit::FitResult & result = fitter.Result();
	  auto parmin=fitter.Config().ParSettings(i).LowerLimit();
	  auto parmax=fitter.Config().ParSettings(i).UpperLimit();
	  auto parval=result.GetParams()[i];
	  //	  fitter.Config().SetFromFitResult(result); // prepare starting point for next step
	  std::cout << "STEP " << istep << " out of " << fixedParametersMask.size()
		    <<": Current value of par[" << i << "]=" << fitter.Config().ParSettings(i).Value() <<" limits=["
		    << fitter.Config().ParSettings(i).LowerLimit()<<", "
		    << fitter.Config().ParSettings(i).UpperLimit()<<"]" << std::endl;
	  fitter.Config().ParSettings(i).RemoveLimits();
	  fitter.Config().ParSettings(i).SetLimits(parmin, parmax);
	  fitter.Config().ParSettings(i).SetValue(parval);
	}
	fitter.Config().ParSettings(i).Release();
	std::cout<<"STEP " << istep << " out of " << fixedParametersMask.size()
		 << ": Releasing par["<<i<<"]  isFixed=" << fitter.Config().ParSettings(i).IsFixed()
		 << " isBound=" << fitter.Config().ParSettings(i).IsBound() << std::endl;
      }
      for(auto &ipar: mask) {
	fitter.Config().ParSettings(ipar).Fix();
	std::cout<<"STEP " << istep << " out of " << fixedParametersMask.size()
		 << ": Fixing par["<<ipar<<"]="<<fitter.Config().ParSettings(ipar).Value()
		 << " isFixed=" << fitter.Config().ParSettings(ipar).IsFixed()
		 << " isBound=" << fitter.Config().ParSettings(ipar).IsBound() << std::endl;
      }
      fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
      fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Fumili2");
      fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000);
      fitter.Config().MinimizerOptions().SetMaxIterations(1000);
      fitter.Config().MinimizerOptions().SetStrategy(1);
      fitter.Config().MinimizerOptions().SetTolerance(fit_options.tolerance); // default=1e-4 for MC-MC comparison
      fitter.Config().MinimizerOptions().SetPrecision( 1e-6 ); // 1e-4 //increased to speed up calculations

      // check fit results
      ok = fitter.FitFCN();
      if (!ok) {
	std::cout<<__FUNCTION__<<": STEP " << istep << " out of " << fixedParametersMask.size() << " (Minuit2/Fumili2) --> Fit failed."<<std::endl;
	//    return 1;
      } else {
	std::cout<<__FUNCTION__<<": STEP " << istep << " out of " << fixedParametersMask.size() << " (Minuit2/Fumili2) --> Fit ok."<<std::endl;
      }
    }
  };

  // get fit parameters
  const ROOT::Fit::FitResult & result = fitter.Result();
  const double * parFit = result.GetParams();
  const double * parErr = result.GetErrors();

  result.Print(std::cout);

  // fill FitDebugData with RECO information per event
  fit_debug_data.chi2 = result.Chi2();
  fit_debug_data.ndf = 1.*result.Ndf();
  fit_debug_data.status = 1.*result.Status();
  fit_debug_data.ncalls = 1.*result.NCalls();
  fit_debug_data.scaleReco = parFit[0];
  fit_debug_data.scaleRecoErr = parErr[0];
  fit_debug_data.xVtxReco = parFit[1];
  fit_debug_data.yVtxReco = parFit[2];
  fit_debug_data.zVtxReco = parFit[3];
  fit_debug_data.xVtxRecoErr = parErr[1];
  fit_debug_data.yVtxRecoErr = parErr[2];
  fit_debug_data.zVtxRecoErr = parErr[3];

  for(int itrack=0; itrack<pidList.size(); itrack++) {

    // create TrackSegment3D collections with RECO information
    TrackSegment3D fit_seg;
    fit_seg.setGeometry(geo);
    fit_seg.setStartEnd(TVector3(parFit[1], parFit[2], parFit[3]),
			TVector3(parFit[4+itrack*3], parFit[5+itrack*3], parFit[6+itrack*3]));
    fit_seg.setPID(pidList[itrack]);

    // fill FitDebugData with RECO information per track
    fit_debug_data.lengthReco[itrack] = fit_seg.getLength();
    fit_debug_data.phiReco[itrack] = fit_seg.getTangent().Phi();
    fit_debug_data.cosThetaReco[itrack] = fit_seg.getTangent().CosTheta();
    fit_debug_data.xEndReco[itrack] = fit_seg.getEnd().X();
    fit_debug_data.yEndReco[itrack] = fit_seg.getEnd().Y();
    fit_debug_data.zEndReco[itrack] = fit_seg.getEnd().Z();
    fit_debug_data.xEndRecoErr[itrack] = parErr[4+itrack*3];
    fit_debug_data.yEndRecoErr[itrack] = parErr[5+itrack*3];
    fit_debug_data.zEndRecoErr[itrack] = parErr[6+itrack*3];
  }
  // on request, store final fitted TH2D histograms as well
  if(fitted_histograms_coll) {
    for(auto &hist: *fitted_histograms_coll) {
      if(hist) hist->Delete();
    }
    fitted_histograms_coll->resize(0);
    auto chi2=myFit(parFit);

    ////// DEBUG
    std::cout << __FUNCTION__ << ": CHI2 evaluated at FINAL point: myFit()=" << chi2 << ", fitter.Result().Chi2()=" << result.Chi2() << std::endl;
    ////// DEBUG

    *fitted_histograms_coll = myFit.getFitHistogramsInMM();
  }
  return result.MinFcnValue();
}

// _______________________________________
//
// Generates a sample of randomly generated pseudo-events
// according to a given reaction hypothesis and strip response model
// and then tries to reconstruct all tracks taking as a starting point
// randomly smeared true vertex and true track endpoints.
// Can be used to measure stability of the fit as a function
// of divergence of the starting point from the true value.
//
int loop(const long maxIter=1, const int runId=1234) {

  gRandom->SetSeed(0);
  ////// DEBUG
  //  gRandom->SetSeed(12345678);
  ////// DEBUG
  
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }

  /////////// input parameters
  //
  std::vector<pid_type> pidList{ALPHA, CARBON_12}; // particle ID
  std::vector<double> E_MeV{3.0, 1.0}; // particle kinetic energy [MeV]
  // std::vector<pid_type> pidList{CARBON_12}; // particle ID
  // std::vector<double> E_MeV{1.0}; // particle kinetic energy [MeV]
  // std::vector<pid_type> pidList{ALPHA}; // particle ID
  // std::vector<double> E_MeV{3.0}; // particle kinetic energy [MeV]

  const auto sigmaXY_mm=2.0; // mm
  const auto sigmaZ_mm=2.0; // mm
  //  const auto peakingTime_ns=0.0; // ns
  const auto peakingTime_ns=232.0; // ns
  const std::string geometryFileName = "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat";
  const auto pressure_mbar = 130.0; // mbar
  //  const std::string geometryFileName = "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat";
  //  const auto pressure_mbar = 250.0; // mbar
  const auto temperature_K = 273.15+20; // K

  const auto reference_adcPerMeV=1e5;
  const auto reference_nPointsMin=1000; // fine granularity for crearing reference UVW histograms to be fitted
  const auto reference_nPointsPerMM=100.0; // fine granularity for creating reference UVW histograms to be fitted

  /////////// initialize TPC geometry, electronic parameters and gas conditions
  //
  auto aGeometry = std::make_shared<GeometryTPC>(geometryFileName.c_str(), false);
  aGeometry->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  /////////// create FIXED vertex for all events
  //
  // NOTE: This vertex is located at the intersection of 3 central strips: U_67, V_113 and W_113.
  //
  bool err;
  TVector2 point;
  aGeometry->GetUVWCrossPointInMM(definitions::projection_type::DIR_U,
				  aGeometry->Strip2posUVW(definitions::projection_type::DIR_U, 67, err),
				  definitions::projection_type::DIR_W,
				  aGeometry->Strip2posUVW(definitions::projection_type::DIR_W, 113, err), point);
  const TVector3 origin(point.X(), point.Y(), 0.0);

  ////////// initialize strip response calculator
  //
  int nstrips=7; // 6;  // NOTE: for sigmas < 2mm use 6x30x12 model while for sigmas >= 2mm use 7x50x14 model
  int ncells=50; // 30; // NOTE: for sigmas < 2mm use 6x30x12 model while for sigmas >= 2mm use 7x50x14 model
  int npads=nstrips*2;
  const std::string responseFileName( (peakingTime_ns==0 ?
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				    nstrips, ncells, npads, aGeometry->GetSamplingRate(), aGeometry->GetDriftVelocity(),
				    sigmaXY_mm, sigmaZ_mm) :
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				    nstrips, ncells, npads, aGeometry->GetSamplingRate(), aGeometry->GetDriftVelocity(),
				    sigmaXY_mm, sigmaZ_mm, peakingTime_ns) ) );
  auto aCalcResponse=std::make_shared<StripResponseCalculator>(aGeometry, nstrips, ncells, npads, sigmaXY_mm, sigmaZ_mm, peakingTime_ns, responseFileName.c_str());
  std::cout << "Loading strip response matrix from file: "<< responseFileName << std::endl;

  ////////// initialize ion range and dE/dx calculator
  //
  auto aCalcRange=std::make_shared<IonRangeCalculator>(CO2, pressure_mbar, temperature_K, false);

  ////////// create dummy EventTPC to get empty UVW projections
  //
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(aGeometry);
  std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    referenceHistosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm);
  }

  /////////// open output ROOT files
  //
  const std::string rootFileName = "FitDebug.root";
  TFile* outputROOTFile = new TFile(rootFileName.c_str(),"RECREATE");
  if(!outputROOTFile || !outputROOTFile->IsOpen()) {
    std::cerr<<"ERROR: Cannot create output file: "<<rootFileName<<"!"<<std::endl;
    return 1;
  }
  outputROOTFile->cd();
  TTree *tree = new TTree("t", "Fit debug tree");
  FitDebugData3prong fit_debug_data;
  tree->Branch("fit",&fit_debug_data);

  const std::vector<std::string> fileName={ "Generated_Track3D.root", "Reco_Track3D.root" };
  std::vector<TFile*> filePtr={ NULL, NULL };
  std::vector<Track3D*> trackPtr={ NULL, NULL };
  std::vector<eventraw::EventInfo*> eventInfoPtr={ NULL, NULL };
  std::vector<TTree*> treePtr={ NULL, NULL };
  for(auto ifile=0; ifile<2; ifile++) {
    filePtr[ifile] = new TFile(fileName[ifile].c_str(), "RECREATE");
    if(!filePtr[ifile] || !filePtr[ifile]->IsOpen()) {
      std::cerr<<"ERROR: Cannot create output file: "<<fileName[ifile]<<"!"<<std::endl;
      return 1;
    }
    filePtr[ifile]->cd();
    trackPtr[ifile] = new Track3D();
    treePtr[ifile] = new TTree("TPCRecoData","");
    treePtr[ifile]->Branch("RecoEvent", &trackPtr[ifile]);
    eventInfoPtr[ifile] = new eventraw::EventInfo();
    treePtr[ifile]->Branch("EventInfo", &eventInfoPtr[ifile]);
  }

#if(DEBUG_DRAW_FIT)
  ////////// opens output ROOT file with TCanvas
  //
  const std::string rootFileNameCanvas = "fit_ref_histos.root";
  TFile *outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
  outputCanvasROOTFile->cd();
  TCanvas *outputCanvas=new TCanvas("c_result", "c_result", 500, 500);
#endif

  ////////// sets fit options
  //
  const auto tolerance = 1e-4; // default=1e-4 for MC-MC comparison
  const auto maxDeviationInADC=reference_adcPerMeV*0.5; // for smearing starting point and setting parameter limits
  const auto maxDeviationInMM=5.0; // mm, for smearing fit starting point and setting parameter limits
  FitOptionType fit_options;
  fit_options.tolerance = tolerance;
  fit_options.maxDeviationInMM=maxDeviationInMM;

  // sanity check before main loop
  if(pidList.size()<1 || pidList.size()>3) {
    std::cout << __FUNCTION__ << ": Invalid number of tracks for FitDebugData3prong!" << std::endl << std::flush;
    exit(1);
  }

  // measure elapsed time
  TStopwatch t;
  t.Start();

  /////////// perform fits of randomly generated events
  //
  for(auto iter=0; iter<maxIter; iter++) {
    std::cout << "#####################" << std::endl
	      << "#### TEST ITER=" << iter << std::endl
	      << "#####################" << std::endl;

    // True MC: create single pseudo-track at given vertex position in DET coordinates, and at given kinetic energy in LAB frame.
    //
    for(auto &it: referenceHistosInMM) {
      if(!it) {
	std::cout << __FUNCTION__ << ": Cannot create empty reference UVW histogram!" << std::endl << std::flush;
	exit(1);
      }
      it->Sumw2(true);
      it->Reset();
    }
    aCalcResponse->setUVWprojectionsInMM(referenceHistosInMM);

    // create pseudo-data histograms to be fitted
    const auto reference_origin=origin; // mm
    std::vector<TVector3> reference_endpoint(pidList.size());
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      const auto length=aCalcRange->getIonRangeMM(pidList[itrack], E_MeV[itrack]); // mm
      auto unit_vec=getRandomDir(gRandom);

      ////// DEBUG - quick hack for creating 2-prong back-to-back events in LAB reference frame
      if(pidList.size()==2 && itrack==1) {
	unit_vec=-(reference_endpoint[0]-reference_origin).Unit();
      }
      ////// DEBUG

      reference_endpoint[itrack]=reference_origin+unit_vec*length;
      const int reference_npoints=std::max((int)(reference_nPointsPerMM*length+0.5), reference_nPointsMin);
      auto curve(aCalcRange->getIonBraggCurveMeVPerMM(pidList[itrack], E_MeV[itrack], reference_npoints)); // MeV/mm
      for(auto ipoint=0; ipoint<reference_npoints; ipoint++) { // generate NPOINTS hits along the track
	auto depth=(ipoint+0.5)*length/reference_npoints; // mm
	auto hitPosition=reference_origin+unit_vec*depth; // mm
	auto hitCharge=reference_adcPerMeV*curve.Eval(depth)*(length/reference_npoints); // ADC units, arbitrary scaling factor
	aCalcResponse->addCharge(hitPosition, hitCharge); // fill referenceHistosInMM
      }
    }

    // create fit starting point
    const auto deviationDir=getRandomDir(gRandom); // for origin
    const auto deviationStepInMM=1.0; // [mm]
    auto deviationRadiusInMM = deviationStepInMM * (gRandom->Integer((int)(maxDeviationInMM/deviationStepInMM))+1);
    std::vector<double> pStart{
      reference_adcPerMeV+gRandom->Uniform(-maxDeviationInADC, maxDeviationInADC),
	reference_origin.X() + deviationRadiusInMM*deviationDir.X(),
	reference_origin.Y() + deviationRadiusInMM*deviationDir.Y(),
	reference_origin.Z() + deviationRadiusInMM*deviationDir.Z() };
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      const auto deviationDir = getRandomDir(gRandom); // for each endpoint
      pStart.push_back(reference_endpoint[itrack].X() + deviationRadiusInMM*deviationDir.X());
      pStart.push_back(reference_endpoint[itrack].Y() + deviationRadiusInMM*deviationDir.Y());
      pStart.push_back(reference_endpoint[itrack].Z() + deviationRadiusInMM*deviationDir.Z());
    }

    // create TrackSegment3D collections with TRUE information
    Track3D reference_track3d;
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      TrackSegment3D reference_seg;
      reference_seg.setGeometry(aGeometry);
      reference_seg.setStartEnd(reference_origin, reference_endpoint[itrack]);
      reference_seg.setPID(pidList[itrack]);
      reference_track3d.addSegment(reference_seg);
    }
    *trackPtr[0]=reference_track3d;

    // fill FitDebugData with TRUE information per event
    fit_debug_data.eventId = iter;
    fit_debug_data.runId = runId;
    fit_debug_data.ntracks = pidList.size();
    fit_debug_data.scaleTrue = reference_adcPerMeV;
    fit_debug_data.xVtxTrue = reference_origin.X();
    fit_debug_data.yVtxTrue = reference_origin.Y();
    fit_debug_data.zVtxTrue = reference_origin.Z();
    fit_debug_data.initScaleDeviation = fabs(pStart[0]-reference_adcPerMeV);
    fit_debug_data.initVtxDeviation = sqrt( pow(pStart[1]-reference_origin.X(), 2) +
					    pow(pStart[2]-reference_origin.Y(), 2) +
					    pow(pStart[3]-reference_origin.Z(), 2) );

    // fill FitDebugData with TRUE information per track
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      fit_debug_data.energy[itrack] = E_MeV[itrack];
      fit_debug_data.pid[itrack] = pidList[itrack];
      fit_debug_data.lengthTrue[itrack] = reference_track3d.getSegments().at(itrack).getLength();
      fit_debug_data.phiTrue[itrack] = reference_track3d.getSegments().at(itrack).getTangent().Phi();
      fit_debug_data.cosThetaTrue[itrack] = reference_track3d.getSegments().at(itrack).getTangent().CosTheta();
      fit_debug_data.xEndTrue[itrack] = reference_endpoint[itrack].X();
      fit_debug_data.yEndTrue[itrack] = reference_endpoint[itrack].Y();
      fit_debug_data.zEndTrue[itrack] = reference_endpoint[itrack].Z();
      fit_debug_data.initEndDeviation[itrack] = sqrt( pow(pStart[4]-reference_endpoint[itrack].X(), 2) +
						      pow(pStart[5]-reference_endpoint[itrack].Y(), 2) +
						      pow(pStart[6]-reference_endpoint[itrack].Z(), 2) );
    }

    // get fit results
#if(DEBUG_DRAW_FIT)
    std::vector<std::shared_ptr<TH2D> > fit_histograms;
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart,
			   fit_options, fit_debug_data, &fit_histograms);
#else
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart,
			   fit_options, fit_debug_data);
#endif
    // compare TRUE and RECO observables
    std::cout << "Final FCN value " << fit_debug_data.chi2
	      << " (Status=" << fit_debug_data.status << ", Ncalls=" << fit_debug_data.ncalls << ", Ndf=" << fit_debug_data.ndf << ")" << std::endl;
    std::cout << "\nFitted SCALE [ADC/MeV] = " << fit_debug_data.scaleReco << " +/- " << fit_debug_data.scaleRecoErr << std::endl
	      <<   "  True SCALE [ADC/MeV] = " << fit_debug_data.scaleTrue << std::endl;
    std::cout << "\nFitted X_VTX [mm] = " << fit_debug_data.xVtxReco << " +/- " << fit_debug_data.xVtxRecoErr << std::endl
	      <<   "  True X_VTX [mm] = " << fit_debug_data.xVtxTrue << std::endl;
    std::cout << "\nFitted Y_VTX [mm] = " << fit_debug_data.yVtxReco << " +/- " << fit_debug_data.yVtxRecoErr << std::endl
	      <<   "  True Y_VTX [mm] = " << fit_debug_data.yVtxTrue << std::endl;
    std::cout << "\nFitted Z_VTX [mm] = " << fit_debug_data.zVtxReco << " +/- " << fit_debug_data.zVtxRecoErr << std::endl
	      <<   "  True Z_VTX [mm] = " << fit_debug_data.zVtxTrue << std::endl;
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      std::cout << "\nTrack " << itrack+1 << ": Fitted X_END [mm] = " << fit_debug_data.xEndReco[itrack]
		<< " +/- " << fit_debug_data.xEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   True X_END [mm] = " << fit_debug_data.xEndTrue[itrack] << std::endl;
      std::cout << "\nTrack " << itrack+1 << ": Fitted Y_END [mm] = " << fit_debug_data.yEndReco[itrack]
		<< " +/- " << fit_debug_data.yEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   True Y_END [mm] = " << fit_debug_data.yEndTrue[itrack] << std::endl;
      std::cout << "\nTrack " << itrack+1 << ": Fitted Z_END [mm] = " << fit_debug_data.zEndReco[itrack]
		<< " +/- " << fit_debug_data.zEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   True Z_END [mm] = " << fit_debug_data.zEndTrue[itrack] << std::endl;
    }

    // create TrackSegment3D collections with RECO information
    Track3D fit_track3d;
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      TrackSegment3D fit_seg;
      fit_seg.setGeometry(aGeometry);
      fit_seg.setStartEnd(TVector3(fit_debug_data.xVtxReco,fit_debug_data.yVtxReco,fit_debug_data.zVtxReco),
			  TVector3(fit_debug_data.xEndReco[itrack],fit_debug_data.yEndReco[itrack],fit_debug_data.zEndReco[itrack]));
      fit_seg.setPID(pidList[itrack]);
      fit_track3d.addSegment(fit_seg);
    }
    *trackPtr[1]=fit_track3d;

    // update trees
    outputROOTFile->cd();
    tree->Fill();
    for(auto ifile=0; ifile<2; ifile++) {
      filePtr[ifile]->cd();
      eventInfoPtr[ifile]->SetRunId(runId); // fake run ID (below 1E6)
      eventInfoPtr[ifile]->SetEventId(iter);
      treePtr[ifile]->Fill();
    }

    // optionally draw resulting histograms as well
#if(DEBUG_DRAW_FIT)
    // if(isFirst) {
    outputCanvasROOTFile->cd();
    DrawFitResults(outputCanvas, &referenceHistosInMM, &fit_histograms, trackPtr[1], trackPtr[0], &fit_debug_data);
    outputCanvas->SetName(Form("c_run%ld_evt%ld", (long)fit_debug_data.runId, (long)fit_debug_data.eventId));
    outputCanvas->SetTitle(outputCanvas->GetName());
    outputCanvas->Write();
    // isFirst=false;
    // }
#endif
  }

  // measure elapsed time
  t.Stop();
  std::cout << "===========" << std::endl;
  std::cout << "CPU time needed to fit " << maxIter << " events:" << std::endl;
  t.Print();
  std::cout << "===========" << std::endl;

  ////////// write trees and close output ROOT files
  //
  for(auto ifile=0; ifile<2; ifile++) {
    filePtr[ifile]->cd();
    treePtr[ifile]->Write("",TObject::kOverwrite);
    filePtr[ifile]->Close();
  }
  outputROOTFile->cd();
  tree->Write("",TObject::kOverwrite);
  outputROOTFile->Close();
#if(DEBUG_DRAW_FIT)
  outputCanvasROOTFile->Close();
#endif

  return 0;
}

// _______________________________________
//
// Takes Track3D input tree as a starting point to fit the corresponding GRAW raw-data file(s).
// Events from the two sources are are matched by their {runId, eventId}.
//
static FitOptionType fit_options_default;
int loop_Graw_Track3D(const char *recoInputFile, // Track3D collection with initial RECO data corresponding to GRAW file(s)
		      const char *rawInputFile, // single ROOT file, or single GRAW, or comma-separated list of GRAW files
		      unsigned long firstEvent=0,        // first eventId to process
		      unsigned long lastEvent=0,         // last eventId to process (0 = up to the end)
		      const char *geometryFile="geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat",
		      double pressure_mbar=250.0,
		      double temperature_K=273.15+20,
		      double sigmaXY_mm=2,
		      double sigmaZ_mm=2,
		      double peakingTime_ns=232,
		      bool flag_fiducial_cuts=true, // enable detector/electronics fiducial cuts as in HIGS_analysis
		      bool flag_1prong=true, // enable fitting of 1-prong events
		      bool flag_2prong=true, // enable fitting of 2-prong events
		      bool flag_3prong=true,  // enable fitting of 3-prong events
		      bool flag_clustering=true,  // enable fitting of clustered RAW data
		      bool flag_subtract_pedestals=true, // enable pedestal subtraction in case of GRAW files
		      FitOptionType input_fit_options=fit_options_default
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
  if (!gROOT->GetClass("EventSourceGRAW")){
    R__LOAD_LIBRARY(libTPCGrawToROOT.so);
  }
  if (!gROOT->GetClass("HIGGS_analysis")){
    R__LOAD_LIBRARY(libTPCAnalysis.so);
  }

  /////////// set parameters of EventTPC clustering
  //
  filter_type filterType = (flag_clustering ? filter_type::threshold : filter_type::none);
  //
  // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
  //       Below is a workaround employing simple node.
  // boost::property_tree::ptree aConfig;
  // aConfig.put("hitFilter.recoClusterEnable", true);
  // aConfig.put("hitFilter.recoClusterThreshold", 35.0); // [ADC units] - seed hits
  // aConfig.put("hitFilter.recoClusterDeltaStrips", 2); // band around seed hit in strip units
  // aConfig.put("hitFilter.recoClusterDeltaTimeCells", 5); // band around seed hit in time cells
  // aConfig.put("pedestal.minPedestalCell", 5);
  // aConfig.put("pedestal.maxPedestalCell", 25);
  // aConfig.put("pedestal.minSignalCell", 5);
  // aConfig.put("pedestal.maxSignalCell", 506);
  boost::property_tree::ptree pedestalConfig, hitFilterConfig;
  hitFilterConfig.put("recoClusterEnable", flag_clustering);
  hitFilterConfig.put("recoClusterThreshold", 35.0); // [ADC units] - seed hits
  hitFilterConfig.put("recoClusterDeltaStrips", 2); // band around seed hit in strip units
  hitFilterConfig.put("recoClusterDeltaTimeCells", 5); // band around seed hit in time cells
  pedestalConfig.put("minPedestalCell", 5);
  pedestalConfig.put("maxPedestalCell", 25);
  pedestalConfig.put("minSignalCell", 5);
  pedestalConfig.put("maxSignalCell", 506);
  //  std::cout << "BOOST_PTREE: " << aConfig.get<bool>("hitFilter.recoClusterEnable", false) << std::endl;
  //  std::cout << "BOOST_PTREE: " << aConfig.get<bool>("hitFilter.recoClusterEnable") << std::endl;
  //  std::cout << "BOOST_PTREE: " << aConfig.count("hitFilter.recoClusterEnable") << std::endl;
  //  std::cout << "BOOST_PTREE: " << (bool)(aConfig.find("hitFilter.recoClusterEnable")==aConfig.not_found()) << std::endl;
  //  std::cout << "BOOST_PTREE: " << (bool)(aConfig.find("hitFilter")==aConfig.not_found()) << std::endl;

  /////////// initialize TPC geometry, electronic parameters and gas conditions
  //
  auto aGeometry = std::make_shared<GeometryTPC>(geometryFile, false);
  aGeometry->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  ////////// initialize strip response calculator
  //
  const auto reference_adcPerMeV=1e5;
  //  const auto maxDeviationInADC=reference_adcPerMeV*0.5; // for smearing starting point and setting parameter limits
  //  const auto maxDeviationInMM=5.0; // for setting parameter limits
  double sigmaXY=sigmaXY_mm; // 0.64; // educated guess of transverse charge spread after 10 cm of drift (middle of drift cage)
  double sigmaZ=sigmaZ_mm; // 0.64; // educated guess of longitudinal charge spread after 10 cm of drift (middle of drift cage)
  auto peakingTime=peakingTime_ns;
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  int nstrips=6;  // NOTE: for sigmas < 2mm use 6x30x12 model while for sigmas >= 2mm use 7x50x14 model
  int ncells=30; // NOTE: for sigmas < 2mm use 6x30x12 model while for sigmas >= 2mm use 7x50x14 model
  int npads=nstrips*2;
  const std::string responseFileName( (peakingTime_ns==0 ?
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				    nstrips, ncells, npads, aGeometry->GetSamplingRate(), aGeometry->GetDriftVelocity(),
				    sigmaXY_mm, sigmaZ_mm) :
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				    nstrips, ncells, npads, aGeometry->GetSamplingRate(), aGeometry->GetDriftVelocity(),
				    sigmaXY_mm, sigmaZ_mm, peakingTime_ns) ) );
  auto aCalcResponse=std::make_shared<StripResponseCalculator>(aGeometry, nstrips, ncells, npads, sigmaXY_mm, sigmaZ_mm, peakingTime_ns, responseFileName.c_str());
  std::cout << "Loading strip response matrix from file: "<< responseFileName << std::endl;

  ////////// initialize ion range and dE/dx calculator
  //
  auto aCalcRange=std::make_shared<IonRangeCalculator>(CO2, pressure_mbar, temperature_K, false);

  //////// initialize event filter
  //
  auto beamDirPreset=BeamDirection::MINUS_X;
  TVector3 beamDir; // unit vector in DET coordinate system
  switch(beamDirPreset){
  case BeamDirection::PLUS_X :
    beamDir = TVector3(1,0,0);
    break;
  case BeamDirection::MINUS_X :
    beamDir = TVector3(-1,0,0);
    break;
  default:
    std::cerr<<"ERROR: Wrong beam direction preset!"<<std::endl;
    return 1;
  }
  auto beamEnergy_MeV=10.0; // [MeV] - some dummy value, not used in detector fiducial cuts
  HIGGS_analysis myAnalysisFilter(aGeometry, beamEnergy_MeV, beamDir, pressure_mbar, temperature_K); // just for filtering

  ////////// opens input ROOT file with tracks (Track3D objects)
  //
  // NOTE: Names of trees and branches are kept the same as in HIGS_analysis
  //       for easy MC-reco comparison. To be replaced with a better class/object in the future.
  //
  TFile *aFile = new TFile(recoInputFile, "OLD");
  TTree *aTree=(TTree*)aFile->Get("TPCRecoData");
  if(!aTree) {
    std::cerr<<"ERROR: Cannot find 'TPCRecoData' tree!"<<std::endl;
    return 1;
  }
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
    return 1;
  }
  aBranch->SetAddress(&aTrack);
  eventraw::EventInfo *aEventInfo = 0;
  TBranch *aBranchInfo = aTree->GetBranch("EventInfo");
  if(!aBranchInfo) {
    std::cerr<<"ERROR: Cannot find 'EventInfo' branch!"<<std::endl;
    return 1;
  }
  aEventInfo = new eventraw::EventInfo();
  aBranchInfo->SetAddress(&aEventInfo);

  const unsigned int nEntries = aTree->GetEntries();

  ////////// sort input tree in ascending order of {runID, eventID}
  //
  TTreeIndex *I=NULL;
  Long64_t* index=NULL;
  if(aBranchInfo) {
    aTree->BuildIndex("runId", "eventId");
    I=(TTreeIndex*)aTree->GetTreeIndex(); // get the tree index
    index=I->GetIndex();
  }

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
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(flag_subtract_pedestals);
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
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
	    << std::endl;
  myEventSource->loadFileEntry(0); // load 1st frame (NOTE: otherwise LoadEventId does not work)

  // DEBUG - parsing RunId from GRAW file name
#ifdef WITH_GET
  if(dynamic_cast<EventSourceGRAW*>(myEventSource.get())) {
    auto id = RunIdParser(rawInputFile);
    std::cout << "Parsing whole file name list: " << rawInputFile
	      << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << std::endl;
  }
#endif
  // DEBUG

  /////////// open output ROOT files
  //
  const std::string debugFileName = "FitDebug.root";
  TFile* debugFile = new TFile(debugFileName.c_str(),"RECREATE");
  if(!debugFile || !debugFile->IsOpen()) {
    std::cerr<<"ERROR: Cannot create DEBUG output file: "<<debugFileName<<"!"<<std::endl;
    return 1;
  }
  debugFile->cd();
  TTree *debugTree = new TTree("t", "Fit debug tree");
  FitDebugData3prong fit_debug_data;
  debugTree->Branch("fit",&fit_debug_data);

  const std::string outFileName = "Reco_Track3D.root";
  TFile* outFile = new TFile(outFileName.c_str(), "RECREATE");
  if(!outFile || !outFile->IsOpen()) {
    std::cerr<<"ERROR: Cannot create RECO output file: "<<outFileName<<"!"<<std::endl;
    return 1;
  }
  outFile->cd();
  Track3D* outTrack = new Track3D();
  TTree* outTree = new TTree("TPCRecoData","");
  outTree->Branch("RecoEvent", &outTrack);
  eventraw::EventInfo* outEventInfo = new eventraw::EventInfo();
  outTree->Branch("EventInfo", &outEventInfo);

#if(DEBUG_DRAW_FIT)
  ////////// opens output ROOT file with TCanvas
  //
  const std::string rootFileNameCanvas = "fit_ref_histos.root";
  TFile *outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
  outputCanvasROOTFile->cd();
  TCanvas *outputCanvas=new TCanvas("c_result", "c_result", 500, 500);
#endif

  ////////// sets fit options
  //
  FitOptionType fit_options=input_fit_options;

  ////////// measure elapsed time
  //
  TStopwatch t;
  t.Start();

  ////////// main event processing loop
  //
  long maxevents=-1; // scan all events
  long unsigned fitted_event_count=0; // actual number of fitted events
  maxevents=(maxevents<=0 ? nEntries : std::min((unsigned int)maxevents, nEntries));
  for(auto ievent=0; ievent<maxevents; ievent++) {
    if(index) {
      aBranch->GetEntry(index[ievent]);
      aBranchInfo->GetEntry(index[ievent]);
    } else {
      aBranch->GetEntry(ievent);
      aBranchInfo->GetEntry(ievent);
    }

    const unsigned long runId = (unsigned long)aEventInfo->GetRunId();
    const unsigned long eventId = (unsigned long)aEventInfo->GetEventId();
    const int nTracks = aTrack->getSegments().size();

    std::cout << "#####################" << std::endl
	      << "#### FIT ITER=" << ievent<< ", RUN=" << runId << ", EVENT=" << eventId << ", NTRACKS=" << nTracks << std::endl
	      << "#####################" << std::endl;


    if((firstEvent>0 && eventId<firstEvent) || (lastEvent>0 && eventId>lastEvent)) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event outside allowed eventId range: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    switch(nTracks) {
    case 3:
      if(!flag_3prong) { // ignore 3-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 3-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    case 2:
      if(!flag_2prong) { // ignore 2-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 2-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    case 1:
      if(!flag_1prong) { // ignore 1-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 1-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    default:;
    };

    // sanity check
    if(nTracks<1 || nTracks>3) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event with wrong number of initial tracks: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    /////// assign correct geometry pointer to initial track segments
    //
    for(auto &aSeg: aTrack->getSegments()) {
      aSeg.setGeometry(aGeometry);
    }

    // get sorted list of tracks (descending order by track length)
    auto coll=aTrack->getSegments();
    std::sort(coll.begin(), coll.end(),
	      [](const TrackSegment3D& a, const TrackSegment3D& b) {
		return a.getLength() > b.getLength();
	      });

#if(MISSING_PID_REPLACEMENT_ENABLE) // TODO - to be parameterized
    // assign missing PID to REF data file by track length
    switch(nTracks) {
    case 3:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_3PRONG_LEADING); // TODO - to be parameterized
      if(coll.at(1).getPID()==UNKNOWN) coll.at(1).setPID(MISSING_PID_3PRONG_MIDDLE); // TODO - to be parameterized
      if(coll.back().getPID()==UNKNOWN) coll.back().setPID(MISSING_PID_3PRONG_TRAILING); // TODO - to be parameterized
      break;
    case 2:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_2PRONG_LEADING); // TODO - to be parameterized
      if(coll.back().getPID()==UNKNOWN) coll.back().setPID(MISSING_PID_2PRONG_TRAILING); // TODO - to be parameterized
      break;
    case 1:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_1PRONG); // TODO - to be parameterized
      break;
    default:;
    };
#endif

    //////// apply fiducial cuts on initial input tracks using HIGS_analysis::eventFilter method
    //
    if(!myAnalysisFilter.eventFilter(aTrack)) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event that failed HIGS_analysis quality cuts: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    // initialize list of PIDs to be fitted
    std::vector<pid_type> pidList;
    for(auto &aSeg: coll) {
      pidList.push_back(aSeg.getPID());
    }

    // load EventTPC to be fitted from the GRAW file
    myEventSource->loadEventId(eventId);
    auto aEventTPC = myEventSource->getCurrentEvent();
    auto currentEventId = aEventTPC->GetEventInfo().GetEventId();
    auto currentRunId = aEventTPC->GetEventInfo().GetRunId();

    // sanity check
    if(currentRunId!=runId || currentEventId!=eventId) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event with missing RAW data: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    std::cout << __FUNCTION__ << ": " << aEventTPC->GetEventInfo()<<std::endl; exit(1);

    // prepare clustered UVW projections to be fitted
    // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
    //       Below is a workaround employing simple node.
    //    aEventTPC->setHitFilterConfig(filterType, aConfig.find("hitFilter")->second);
    aEventTPC->setHitFilterConfig(filterType, hitFilterConfig);
    std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir] = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
    }

    //////// create TrackSegment3D collections with TRUE/INIT information
    //
    const auto reference_origin=coll.front().getStart(); // mm
    std::vector<TVector3> reference_endpoint(pidList.size());
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      reference_endpoint[itrack] = coll.at(itrack).getEnd();
    }

    //////// create starting point (optionally smeared around TRUE/INIT value)
    //
    const auto deviationDir=getRandomDir(gRandom); // for origin, enpoints
    //    const auto deviationStepInMM=1.0; // [mm]
    //    auto deviationRadiusInMM = deviationStepInMM * (gRandom->Integer((int)(maxDeviationInMM/deviationStepInMM))+1);
    auto deviationRadiusInMM = 0.0; // no deviation
    auto reference_adcPerMeV=1e5; // [ADC units / MeV] - order of magnitude, this will be refinend automatically
    std::vector<double> pStart{
      reference_adcPerMeV,
      reference_origin.X() + deviationRadiusInMM*deviationDir.X(),
      reference_origin.Y() + deviationRadiusInMM*deviationDir.Y(),
      reference_origin.Z() + deviationRadiusInMM*deviationDir.Z() };
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      const auto deviationDir = getRandomDir(gRandom); // for each endpoint
      pStart.push_back(reference_endpoint[itrack].X() + deviationRadiusInMM*deviationDir.X());
      pStart.push_back(reference_endpoint[itrack].Y() + deviationRadiusInMM*deviationDir.Y());
      pStart.push_back(reference_endpoint[itrack].Z() + deviationRadiusInMM*deviationDir.Z());
    }

    // fill FitDebugData with TRUE/INIT information per event
    fit_debug_data.eventId = eventId;
    fit_debug_data.runId = runId;
    fit_debug_data.ntracks = pidList.size();
    fit_debug_data.scaleTrue = reference_adcPerMeV;
    fit_debug_data.xVtxTrue = reference_origin.X();
    fit_debug_data.yVtxTrue = reference_origin.Y();
    fit_debug_data.zVtxTrue = reference_origin.Z();
    fit_debug_data.initScaleDeviation = fabs(pStart[0]-reference_adcPerMeV);
    fit_debug_data.initVtxDeviation = sqrt( pow(pStart[1]-reference_origin.X(), 2) +
					    pow(pStart[2]-reference_origin.Y(), 2) +
					    pow(pStart[3]-reference_origin.Z(), 2) );

    // fill FitDebugData with TRUE/INIT information per track
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      fit_debug_data.energy[itrack] = aCalcRange->getIonEnergyMeV(pidList[itrack], coll.at(itrack).getLength()); // MeV
      fit_debug_data.pid[itrack] = pidList[itrack];
      fit_debug_data.lengthTrue[itrack] = coll.at(itrack).getLength(); // mm
      fit_debug_data.phiTrue[itrack] = coll.at(itrack).getTangent().Phi(); // rad
      fit_debug_data.cosThetaTrue[itrack] = coll.at(itrack).getTangent().CosTheta();
      fit_debug_data.xEndTrue[itrack] = reference_endpoint[itrack].X(); // mm
      fit_debug_data.yEndTrue[itrack] = reference_endpoint[itrack].Y(); // mm
      fit_debug_data.zEndTrue[itrack] = reference_endpoint[itrack].Z(); // mm
      fit_debug_data.initEndDeviation[itrack] = sqrt( pow(pStart[4]-reference_endpoint[itrack].X(), 2) +
						      pow(pStart[5]-reference_endpoint[itrack].Y(), 2) +
						      pow(pStart[6]-reference_endpoint[itrack].Z(), 2) ); // mm
    }

    // prepare fitting strategy for 2-prong and 3-prong cases
    std::vector<std::set<int> > fixedParametersMask;
    if(nTracks==2) {
      fixedParametersMask.push_back({1, 2, 4, 5, 7, 8});                      // STEP 1: fix only XY_DET coordinates
      fixedParametersMask.push_back({4, 5, 6, 7, 8, 9});                      // STEP 2: fix all endpoints
      fixedParametersMask.push_back({1, 2, 3});                               // STEP 3: fix vertex
      fixedParametersMask.push_back({0});                                     // STEP 4: all free but scale
      fixedParametersMask.push_back(std::set<int>());                         // STEP 5: all free
    } else if(nTracks==3) {
      fixedParametersMask.push_back({1, 2, 4, 5, 7, 8, 10, 11});              // STEP 1: fix only XY_DET coordinates
      fixedParametersMask.push_back({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}); // STEP 2: fix vertex + all endpoints
      fixedParametersMask.push_back({4, 5, 6, 7, 8, 9, 10, 11, 12});          // STEP 3: fix all endpoints
      fixedParametersMask.push_back({1, 2, 3, 7, 8, 9, 10, 11, 12});          // STEP 4: fix vertex + track 2 & 3
      fixedParametersMask.push_back({1, 2, 3, 4, 5, 6, 10, 11, 12});          // STEP 5: fix vertex + track 1 & 3
      fixedParametersMask.push_back({1, 2, 3, 4, 5, 6, 7, 8, 9});             // STEP 6: fix vertex + track 1 & 2
      fixedParametersMask.push_back({4, 5, 6, 7, 8, 9, 10, 11, 12});          // STEP 7: fix all endpoints
      fixedParametersMask.push_back({1, 2, 3});                               // STEP 8: fix vertex
      fixedParametersMask.push_back({0});                                     // STEP 9: all free but scale
      fixedParametersMask.push_back(std::set<int>());                         // STEP 10: all free
    }

    // get fit results
#if(DEBUG_DRAW_FIT)
    std::vector<std::shared_ptr<TH2D> > fit_histograms;
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart,
			   fit_options, fit_debug_data, &fit_histograms, fixedParametersMask);
#else
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart,
			   fit_options, fit_debug_data, NULL, fixedParametersMask);
#endif
    fitted_event_count++;

    // compare TRUE and RECO observables
    std::cout << "Final FCN value " << fit_debug_data.chi2
	      << " (Status=" << fit_debug_data.status << ", Ncalls=" << fit_debug_data.ncalls << ", Ndf=" << fit_debug_data.ndf << ")" << std::endl;
    std::cout << "\nFitted SCALE [ADC/MeV] = " << fit_debug_data.scaleReco << " +/- " << fit_debug_data.scaleRecoErr << std::endl
	      <<   "  TRUE/INIT SCALE [ADC/MeV] = " << fit_debug_data.scaleTrue << std::endl;
    std::cout << "\nFitted X_VTX [mm] = " << fit_debug_data.xVtxReco << " +/- " << fit_debug_data.xVtxRecoErr << std::endl
	      <<   "  TRUE/INIT X_VTX [mm] = " << fit_debug_data.xVtxTrue << std::endl;
    std::cout << "\nFitted Y_VTX [mm] = " << fit_debug_data.yVtxReco << " +/- " << fit_debug_data.yVtxRecoErr << std::endl
	      <<   "  TRUE/INIT Y_VTX [mm] = " << fit_debug_data.yVtxTrue << std::endl;
    std::cout << "\nFitted Z_VTX [mm] = " << fit_debug_data.zVtxReco << " +/- " << fit_debug_data.zVtxRecoErr << std::endl
	      <<   "  TRUE/INIT Z_VTX [mm] = " << fit_debug_data.zVtxTrue << std::endl;
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      std::cout << "\nTrack " << itrack+1 << ": Fitted X_END [mm] = " << fit_debug_data.xEndReco[itrack]
		<< " +/- " << fit_debug_data.xEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   TRUE/INIT X_END [mm] = " << fit_debug_data.xEndTrue[itrack] << std::endl;
      std::cout << "\nTrack " << itrack+1 << ": Fitted Y_END [mm] = " << fit_debug_data.yEndReco[itrack]
		<< " +/- " << fit_debug_data.yEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   TRUE/INIT Y_END [mm] = " << fit_debug_data.yEndTrue[itrack] << std::endl;
      std::cout << "\nTrack " << itrack+1 << ": Fitted Z_END [mm] = " << fit_debug_data.zEndReco[itrack]
		<< " +/- " << fit_debug_data.zEndRecoErr[itrack] << std::endl
		<<   "Track " << itrack+1 << ":   TRUE/INIT Z_END [mm] = " << fit_debug_data.zEndTrue[itrack] << std::endl;
    }

    //////// create TrackSegment3D collections with RECO information
    //
    Track3D fit_track3d;
    for(int itrack=0; itrack<pidList.size(); itrack++) {
      TrackSegment3D fit_seg;
      fit_seg.setGeometry(aGeometry);
      fit_seg.setStartEnd(TVector3(fit_debug_data.xVtxReco,fit_debug_data.yVtxReco,fit_debug_data.zVtxReco),
			  TVector3(fit_debug_data.xEndReco[itrack],fit_debug_data.yEndReco[itrack],fit_debug_data.zEndReco[itrack]));
      fit_seg.setPID(pidList[itrack]);
      fit_track3d.addSegment(fit_seg);
    }
    *outTrack=fit_track3d;

    //////// update trees
    //
    debugFile->cd();
    debugTree->Fill();
    outFile->cd();
    outEventInfo->SetRunId(runId);
    outEventInfo->SetEventId(eventId);
    outTree->Fill();

    //////// optionally draw resulting histograms as well
    //
#if(DEBUG_DRAW_FIT)
    ////// DEBUG
    if(aTrack) for(auto &aSeg: aTrack->getSegments()) {
	std::cout << __FUNCTION__ << ": ATRACK initial track: vtx=[" << aSeg.getStart().X()
		  << ", " << aSeg.getStart().Y()
		  << ", " << aSeg.getStart().Z()
		  << "], end=[" << aSeg.getEnd().X()
		  << ", " << aSeg.getEnd().Y()
		  << ", " << aSeg.getEnd().Z() << "], is_geometry_null=" << (bool)(aSeg.getGeometry()==NULL) << std::endl;
      }
    if(aTrack) for(auto &aSeg: coll) {
	std::cout << __FUNCTION__ << ": COLL initial track: vtx=[" << aSeg.getStart().X()
		  << ", " << aSeg.getStart().Y()
		  << ", " << aSeg.getStart().Z()
		  << "], end=[" << aSeg.getEnd().X()
		  << ", " << aSeg.getEnd().Y()
		  << ", " << aSeg.getEnd().Z() << "], is_geometry_null=" << (bool)(aSeg.getGeometry()==NULL) << std::endl;
      }
    ////// DEBUG

    outputCanvasROOTFile->cd();
    DrawFitResults(outputCanvas, &referenceHistosInMM, &fit_histograms, &fit_track3d, aTrack, &fit_debug_data);
    outputCanvas->SetName(Form("c_run%ld_evt%ld", runId, eventId));
    outputCanvas->SetTitle(outputCanvas->GetName());
    outputCanvas->Write();
    aFile->cd();
#endif

    //////// discard UVW projectons that are not needed anymore
    //
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir].reset();
    }

    ////// DEBUG
    //    if(fitted_event_count==1) break; // stop after 1st fitted event
    ////// DEBUG

  } // end of main processing loop

  ////////// measure elapsed time
  //
  t.Stop();
  std::cout << "===========" << std::endl;
  std::cout << "CPU time needed to fit " << fitted_event_count << " events:" << std::endl;
  t.Print();
  std::cout << "===========" << std::endl;

  ////////// write trees and close output ROOT files
  //
  outFile->cd();
  outTree->Write("",TObject::kOverwrite);
  outFile->Close();
  debugFile->cd();
  debugTree->Write("",TObject::kOverwrite);
  debugFile->Close();
  aFile->Close();
#if(DEBUG_DRAW_FIT)
  outputCanvasROOTFile->Close();
#endif
  return 0;
}

// _______________________________________
//
// Overlays Track3D from RECO file on top of PEventTPC raw-data deposits from GRAW file.
// Pedestal calculation and clustering settings have to be adjusted for the real-data and the Monte Carlo cases.
//
int loop_Graw_Track3D_Display(const char *recoInputFile, // Track3D collection with RECO data (e.g.FIT) corresponding to RAW file
			      const char *rawInputFile, // single ROOT file, or single GRAW, or comma-separated list of GRAW files
			      unsigned long firstEvent=0,        // first eventId to process
			      unsigned long lastEvent=0,         // last eventId to process (0 = up to the end)
			      const char *geometryFile="geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat",
			      double beamEnergy_MeV=13.1,
			      double pressure_mbar=250.0,
			      double temperature_K=273.15+20,
			      bool flag_fiducial_cuts=true, // enable detector/electronics fiducial cuts as in HIGS_analysis
			      bool flag_1prong=true, // enable fitting of 1-prong events
			      bool flag_2prong=true, // enable fitting of 2-prong events
			      bool flag_3prong=true, // enable fitting of 3-prong events
			      bool flag_clustering=false,  // enable displaying clustered RAW data
			      bool flag_subtract_pedestals=true,  // enable pedestal subtraction in case of GRAW files
			      const char *refInputFile=NULL // optional Track3D collection with REFERENCE (e.g. TRUE MC) RECO data corresponding to RAW file
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
  if (!gROOT->GetClass("EventSourceGRAW")){
    R__LOAD_LIBRARY(libTPCGrawToROOT.so);
  }
  if (!gROOT->GetClass("HIGGS_analysis")){
    R__LOAD_LIBRARY(libTPCAnalysis.so);
  }

  /////////// set parameters of EventTPC clustering
  //
  filter_type filterType = (flag_clustering ? filter_type::threshold : filter_type::none);
  //
  // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
  //       Below is a workaround employing simple node.
  // boost::property_tree::ptree aConfig;
  // aConfig.put("hitFilter.recoClusterEnable", true);
  // aConfig.put("hitFilter.recoClusterThreshold", 35.0); // [ADC units] - seed hits
  // aConfig.put("hitFilter.recoClusterDeltaStrips", 2); // band around seed hit in strip units
  // aConfig.put("hitFilter.recoClusterDeltaTimeCells", 5); // band around seed hit in time cells
  // aConfig.put("pedestal.minPedestalCell", 5);
  // aConfig.put("pedestal.maxPedestalCell", 25);
  // aConfig.put("pedestal.minSignalCell", 5);
  // aConfig.put("pedestal.maxSignalCell", 506);
  boost::property_tree::ptree pedestalConfig, hitFilterConfig;
  hitFilterConfig.put("recoClusterEnable", flag_clustering);
  hitFilterConfig.put("recoClusterThreshold", 35.0); // [ADC units] - seed hits
  hitFilterConfig.put("recoClusterDeltaStrips", 2); // band around seed hit in strip units
  hitFilterConfig.put("recoClusterDeltaTimeCells", 5); // band around seed hit in time cells
  pedestalConfig.put("minPedestalCell", 5);
  pedestalConfig.put("maxPedestalCell", 25);
  pedestalConfig.put("minSignalCell", 5);
  pedestalConfig.put("maxSignalCell", 506);
  //  std::cout << "BOOST_PTREE: " << aConfig.get<bool>("hitFilter.recoClusterEnable", false) << std::endl;
  //  std::cout << "BOOST_PTREE: " << aConfig.get<bool>("hitFilter.recoClusterEnable") << std::endl;
  //  std::cout << "BOOST_PTREE: " << aConfig.count("hitFilter.recoClusterEnable") << std::endl;
  //  std::cout << "BOOST_PTREE: " << (bool)(aConfig.find("hitFilter.recoClusterEnable")==aConfig.not_found()) << std::endl;
  //  std::cout << "BOOST_PTREE: " << (bool)(aConfig.find("hitFilter")==aConfig.not_found()) << std::endl;

  /////////// initialize TPC geometry, electronic parameters and gas conditions
  //
  auto aGeometry = std::make_shared<GeometryTPC>(geometryFile, false);
  aGeometry->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  //////// initialize event filter
  //
  auto beamDirPreset=BeamDirection::MINUS_X;
  TVector3 beamDir; // unit vector in DET coordinate system
  switch(beamDirPreset){
  case BeamDirection::PLUS_X :
    beamDir = TVector3(1,0,0);
    break;
  case BeamDirection::MINUS_X :
    beamDir = TVector3(-1,0,0);
    break;
  default:
    std::cerr<<"ERROR: Wrong beam direction preset!"<<std::endl;
    return 1;
  }
  HIGGS_analysis myAnalysisFilter(aGeometry, beamEnergy_MeV, beamDir, pressure_mbar, temperature_K); // just for filtering

  ////////// opens MAIN input ROOT file with tracks (Track3D objects)
  //
  // NOTE: Tree's and branch names are kept the same as in HIGS_analysis
  //       for easy MC-reco comparison. To be replaced with better
  //       class/object in future toy MC.
  TFile *aFile = new TFile(recoInputFile, "OLD");
  if(!aFile && strlen(recoInputFile)>0) {
    std::cerr<<"ERROR: Cannot open MAIN RECO file:" << recoInputFile << "!" << std::endl;
    return 1;
  }
  TTree *aTree=(TTree*)aFile->Get("TPCRecoData");
  if(!aTree) {
    std::cerr<<"ERROR: Cannot find 'TPCRecoData' tree!"<<std::endl;
    return 1;
  }
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
    return 1;
  }
  aBranch->SetAddress(&aTrack);
  eventraw::EventInfo *aEventInfo = 0;
  TBranch *aBranchInfo = aTree->GetBranch("EventInfo");
  if(!aBranchInfo) {
    std::cerr<<"ERROR: Cannot find 'EventInfo' branch!"<<std::endl;
    return 1;
  }
  aEventInfo = new eventraw::EventInfo();
  aBranchInfo->SetAddress(&aEventInfo);

  const unsigned int nEntries = aTree->GetEntries();

  ////////// opens optional REERENCE input ROOT file with tracks (Track3D objects)
  //
  TFile *refFile=NULL;
  TTree *refTree=NULL;
  Track3D *refTrack=NULL;
  TBranch *refBranch=NULL;
  eventraw::EventInfo *refEventInfo=NULL;
  TBranch *refBranchInfo=NULL;
  if(refInputFile) {
    refFile = new TFile(refInputFile, "OLD");
    if(!refFile) {
      std::cerr<<"ERROR: Cannot open REFERENCE RECO file:" << refInputFile << "!" << std::endl;
      return 1;
    }
  }
  if(refFile) {
    refTree=(TTree*)refFile->Get("TPCRecoData");
    if(!refTree) {
      std::cerr<<"ERROR: Cannot find 'TPCRecoData' tree!"<<std::endl;
    }
  }
  if(refTree) {
    refTrack = new Track3D();
    refBranch  = refTree->GetBranch("RecoEvent");
    if(!refBranch) {
      std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
      return 1;
    }
    refBranch->SetAddress(&refTrack);
    refBranchInfo = refTree->GetBranch("EventInfo");
    if(!refBranchInfo) {
      std::cerr<<"ERROR: Cannot find 'EventInfo' branch!"<<std::endl;
      return 1;
    }
    refEventInfo = new eventraw::EventInfo();
    refBranchInfo->SetAddress(&refEventInfo);

    // set friend class
    aTree->AddFriend(refTree);
  }

  ////////// sort input tree in ascending order of {runID, eventID}
  //
  TTreeIndex *I=NULL;
  Long64_t* index=NULL;
  if(aBranchInfo) {
    aTree->BuildIndex("runId", "eventId");
    I=(TTreeIndex*)aTree->GetTreeIndex(); // get the tree index
    index=I->GetIndex();
  }
  Long64_t* refIndex=NULL;
  if(refTree && refBranchInfo) {
    TTreeIndex *I=NULL;
    refTree->BuildIndex("runId", "eventId");
    I=(TTreeIndex*)refTree->GetTreeIndex(); // get the tree index
    refIndex=I->GetIndex();
  }


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
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(flag_subtract_pedestals);
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
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
	    << std::endl;
  myEventSource->loadFileEntry(0); // load 1st frame (NOTE: otherwise LoadEventId does not work)

  ////////// initialize output ROOT file with TCanvas
  //
  const std::string rootFileNameCanvas = "fit_ref_histos.root";
  TFile *outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
  outputCanvasROOTFile->cd();
  TCanvas *outputCanvas=new TCanvas("c_result", "c_result", 500, 500);

  ////////// main event processing loop
  //
  long maxevents=-1; // scan all events
  long unsigned displayed_event_count=0; // actual number of displayed events
  maxevents=(maxevents<=0 ? nEntries : std::min((unsigned int)maxevents, nEntries));
  for(auto ievent=0; ievent<maxevents; ievent++) {
    if(index) {
      aBranch->GetEntry(index[ievent]);
      aBranchInfo->GetEntry(index[ievent]);
    } else {
      aBranch->GetEntry(ievent);
      aBranchInfo->GetEntry(ievent);
    }

    const unsigned long runId = (unsigned long)aEventInfo->GetRunId();
    const unsigned long eventId = (unsigned long)aEventInfo->GetEventId();
    const int nTracks = aTrack->getSegments().size();

    // sanity check for optional REFERNCE tracks
    bool refOK=false;
    if(refTree) {
      if(refIndex) {
	refBranch->GetEntry(refTree->GetEntryNumberWithIndex(runId, eventId));
	refBranchInfo->GetEntry(refTree->GetEntryNumberWithIndex(runId, eventId));
      }
      std::cout << "REF CHECK: refEventInfo=" << refEventInfo << ", refTrack=" << refTrack << std::endl;
      std::cout << "REF CHECK: refEventInfo: " << *refEventInfo << std::endl;
      std::cout << "REF CHECK: refTrack: ntracks=" << refTrack->getSegments().size()<<std::endl;
      const unsigned long refRunId = (unsigned long)refEventInfo->GetRunId();
      const unsigned long refEventId = (unsigned long)refEventInfo->GetEventId();
      const int refNTracks = refTrack->getSegments().size();
      if(refRunId==runId && refEventId==eventId && nTracks==refNTracks) refOK=true;
    }

    std::cout << "#####################" << std::endl
	      << "#### EVENT PLAYER ITER=" << ievent<< ", RUN=" << runId << ", EVENT=" << eventId << ", NTRACKS=" << nTracks << std::endl
	      << "#####################" << std::endl;


    if((firstEvent>0 && eventId<firstEvent) || (lastEvent>0 && eventId>lastEvent)) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event outside allowed eventId range: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    switch(nTracks) {
    case 3:
      if(!flag_3prong) { // ignore 3-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 3-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    case 2:
      if(!flag_2prong) { // ignore 2-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 2-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    case 1:
      if(!flag_1prong) { // ignore 1-prong events on request
	std::cout << __FUNCTION__ << KRED << ": Skipping event with 1-prong tracks: run=" << runId << ", event=" << eventId
		  << RST << std::endl << std::flush;
	continue;
      }
      break;
    default:;
    };

    // sanity check
    if(nTracks<1 || nTracks>3) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event with wrong number of initial tracks: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    /////// assign correct geometry pointer to track segments
    //
    for(auto &aSeg: aTrack->getSegments()) {
      aSeg.setGeometry(aGeometry);
    }
    if(refTree) {
      for(auto &refSeg: refTrack->getSegments()) {
	refSeg.setGeometry(aGeometry);
      }
    }

    // get sorted list of tracks (descending order by track length)
    auto coll=aTrack->getSegments();
    std::sort(coll.begin(), coll.end(),
	      [](const TrackSegment3D& a, const TrackSegment3D& b) {
		return a.getLength() > b.getLength();
	      });

#if(MISSING_PID_REPLACEMENT_ENABLE) // TODO - to be parameterized
    // assign missing PID to REF data file by track length
    switch(nTracks) {
    case 3:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_3PRONG_LEADING); // TODO - to be parameterized
      if(coll.at(1).getPID()==UNKNOWN) coll.at(1).setPID(MISSING_PID_3PRONG_MIDDLE); // TODO - to be parameterized
      if(coll.back().getPID()==UNKNOWN) coll.back().setPID(MISSING_PID_3PRONG_TRAILING); // TODO - to be parameterized
      break;
    case 2:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_2PRONG_LEADING); // TODO - to be parameterized
      if(coll.back().getPID()==UNKNOWN) coll.back().setPID(MISSING_PID_2PRONG_TRAILING); // TODO - to be parameterized
      break;
    case 1:
      if(coll.front().getPID()==UNKNOWN) coll.front().setPID(MISSING_PID_1PRONG); // TODO - to be parameterized
      break;
    default:;
    };
#endif

    if(refTree) {
      // get sorted list of tracks (descending order by track length)
      auto coll2=refTrack->getSegments();
      std::sort(coll2.begin(), coll2.end(),
		[](const TrackSegment3D& a, const TrackSegment3D& b) {
		  return a.getLength() > b.getLength();
		});

#if(MISSING_PID_REPLACEMENT_ENABLE) // TODO - to be parameterized
      // assign missing PID to REF data file by track length
      switch(nTracks) {
      case 3:
	if(coll2.front().getPID()==UNKNOWN) coll2.front().setPID(MISSING_PID_3PRONG_LEADING); // TODO - to be parameterized
	if(coll2.at(1).getPID()==UNKNOWN) coll2.at(1).setPID(MISSING_PID_3PRONG_MIDDLE); // TODO - to be parameterized
	if(coll2.back().getPID()==UNKNOWN) coll2.back().setPID(MISSING_PID_3PRONG_TRAILING); // TODO - to be parameterized
	break;
      case 2:
	if(coll2.front().getPID()==UNKNOWN) coll2.front().setPID(MISSING_PID_2PRONG_LEADING); // TODO - to be parameterized
	if(coll2.back().getPID()==UNKNOWN) coll2.back().setPID(MISSING_PID_2PRONG_TRAILING); // TODO - to be parameterized
	break;
      case 1:
	if(coll2.front().getPID()==UNKNOWN) coll2.front().setPID(MISSING_PID_1PRONG); // TODO - to be parameterized
	break;
      default:;
      };
#endif
    }

    //////// apply fiducial cuts on initial input tracks using HIGS_analysis::eventFilter method
    //
    if(!myAnalysisFilter.eventFilter(aTrack)) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event that failed HIGS_analysis quality cuts: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    // load EventTPC to be displayed from the RAW file
    myEventSource->loadEventId(eventId);
    auto aEventTPC = myEventSource->getCurrentEvent();
    auto currentEventId = aEventTPC->GetEventInfo().GetEventId();
    auto currentRunId = aEventTPC->GetEventInfo().GetRunId();

    // sanity check
    if(currentRunId!=runId || currentEventId!=eventId) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event with missing RAW data: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    std::cout << __FUNCTION__ << ": " << aEventTPC->GetEventInfo() << std::endl;

    // prepare UVW projections to be displayed
    // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
    //       Below is a workaround employing simple node.
    //    aEventTPC->setHitFilterConfig(filterType, aConfig.find("hitFilter")->second);
    aEventTPC->setHitFilterConfig(filterType, hitFilterConfig);
    std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir] = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
    }

    //////// display UVW histograms
    //
    outputCanvasROOTFile->cd();
    DrawFitResults(outputCanvas, &referenceHistosInMM, NULL, aTrack, (refOK ? refTrack : NULL) );
    outputCanvas->SetName(Form("c_run%ld_evt%ld", runId, eventId));
    outputCanvas->SetTitle(outputCanvas->GetName());
    outputCanvas->Write();
    aFile->cd();
    displayed_event_count++;

    //////// discard UVW projectons that are not needed anymore
    //
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir].reset();
    }

    ////// DEBUG
    //    if(displayed_event_count==1) break; // stop after 1st displayed event
    ////// DEBUG

  } // end of main processing loop

  aFile->Close();
  if(refFile) {
    refFile->Close();
  }
  outputCanvasROOTFile->Close();

  return 0;
}
