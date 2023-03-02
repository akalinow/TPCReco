//
// Example of using ROOT fitter with PEventTPC, Track3D, StripResponseCalculator, IonRangeCalculator
//
// Mikolaj Cwiok (UW) - 25 Feb 2023
//
//

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../GrawToROOT/include)
R__ADD_INCLUDE_PATH($GET_DIR/GetSoftware_bin/$GET_RELEASE/include) // /home/euser/GetSoftware_bin/20190315_patched_v2/include/
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <vector>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "colorText.h"

#include <TMath.h>
#include <TRandom3.h>
#if DEBUG_SIGNAL
#include <TCanvas.h> // DEBUG
#endif
#include <TFile.h>
#include <TString.h>
#include <TH2D.h>
#include <TVector3.h>
#include <TTree.h>
#include <TBranch.h>
#include <Math/Functor.h>
#include <Fit/Fitter.h>

#include "CommonDefinitions.h"
#include "GeometryTPC.h"
#include "EventTPC.h"
#include "EventSourceBase.h"
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#include "TrackSegment3D.h"
#include "Track3D.h"
#include "IonRangeCalculator.h"
#include "StripResponseCalculator.h"
#include "UtilsMath.h"
#include "RunIdParser.h"
#include "HIGGS_analysis.h"

#define DEBUG_CHARGE    false
#define DEBUG_SIGNAL    false // plot UVW projections for reference data and starting point template
#define DEBUG_CHI2      false

#define MISSING_PID_REPLACEMENT_ENABLE true // TODO - to be parameterized
#define MISSING_PID_1PRONG             ALPHA // TODO - to be parameterized
#define MISSING_PID_2PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_2PRONG_TRAILING    CARBON_12 // TODO - to be parameterized
#define MISSING_PID_3PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_MIDDLE      ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_TRAILING    ALPHA // TODO - to be parameterized

enum class BeamDirection{
  X,
  MINUS_X
};

using namespace ROOT::Math;

class GeometryTPC;
class StripResponseCalculator;
class IonRangeCalculator;
class TrackSegment3D;
class Track3D;
class HIGGS_analysis;

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
  
  std::vector<TH2D*> myRefHistosInMM; // pointers to UVW projections in mm (for reference)
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
  void setRefHistogramsInMM(const std::vector<TH2D*> &reference_data) {
    myRefNpoints=0;
    // sanity check
    if( reference_data.size()!=3 ) {
      std::cout << __FUNCTION__<<": Wrong number of external UVW reference histograms!" << std::endl << std::flush;
      exit(1);
    }
    for(auto &it: reference_data) {
      if(!it) {
	std::cout << __FUNCTION__ << ": Wrong pointer of external UVW reference histogram!" << std::endl << std::flush;
	exit(1);
      }
      myRefHistosInMM.push_back(it);
      myRefNpoints += countNonEmptyBins(it);
    }
    std::cout << __FUNCTION__ << ": There are " << myRefNpoints << " non-empty bins in REFERENCE histograms to be fitted." << std::endl;
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
public:
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  HypothesisFit(const std::vector<std::shared_ptr<TH2D> > &reference_data,
		const std::shared_ptr<StripResponseCalculator> &aResponseCalc,
		const std::shared_ptr<IonRangeCalculator> &aRangeCalc,
		const std::vector<pid_type> &aParticleList)
  {
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
		const std::vector<pid_type> &aParticleList)
    : myRefHistosInMM(reference_data)
  {
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
  
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  double getChi2() {
    const int nhist=myRefHistosInMM.size();
    const double expected_rms_noise=7.0; // [ADC units] - typical rms of noise from pedestal runs @ 12.5 MHz
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
    return sumChi2 /* /ndf */ * 0.5; // NOTE: for FUMILI minimizer the returned function should be: chi^2 / 2
    //    return sumChi2/ndf;
  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  //   double getChi2() {
  //     const int nhist=myRefHistosInMM.size();
  //     const double expected_resolution=10.0/1000.0; // (typical rms noise)/(typical pulse height)
  //     double sumChi2=0.0;
  //     auto ndf=0L;
  //     for(auto ihist=0; ihist<nhist; ihist++) {
  //       for(auto ix=1; ix<=myRefHistosInMM[ihist]->GetNbinsX(); ix++) {
  //   	for(auto iy=1; iy<=myRefHistosInMM[ihist]->GetNbinsY(); iy++) {
  //   	  auto ibin=myRefHistosInMM[ihist]->GetBin(ix, iy);
  //   	  auto val1=myRefHistosInMM[ihist]->GetBinContent(ibin); // observed (experimental data)
  //   	  auto val2=myFitHistosInMM[ihist]->GetBinContent(ibin); // expected (fitted model)
  //   	  auto err1=myRefHistosInMM[ihist]->GetBinError(ibin);
  //   	  auto err2=myFitHistosInMM[ihist]->GetBinError(ibin);
  //   	  if((err1 || err2) && val2) {
  // 	    sumChi2 += (val1-val2)*(val1-val2)/fabs(val2);
  // 	    ndf++;
  // 	  }
  //   	}
  //       }
  //     }
  // #if(DEBUG_CHI2)
  //     std::cout<<__FUNCTION__<<": Sum of chi2 from "<<nhist<<" histograms = "<<sumChi2<<", ndf="<<ndf<<std::endl;
  // #endif
  //     return (sumChi2/ndf)/(expected_resolution);
  //   }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // double getChi2() {
  //   const int nhist=myRefHistosInMM.size();
  //   double sumChi2=0.0;
  //   auto ndf=0L;
  //   for(auto ihist=0; ihist<nhist; ihist++) {
  //     for(auto ix=1; ix<=myRefHistosInMM[ihist]->GetNbinsX(); ix++) {
  // 	for(auto iy=1; iy<=myRefHistosInMM[ihist]->GetNbinsY(); iy++) {
  // 	  auto ibin=myRefHistosInMM[ihist]->GetBin(ix, iy);
  // 	  auto val1=myRefHistosInMM[ihist]->GetBinContent(ibin);
  // 	  auto val2=myFitHistosInMM[ihist]->GetBinContent(ibin);
  // 	  auto err1=myRefHistosInMM[ihist]->GetBinError(ibin);
  // 	  auto err2=myFitHistosInMM[ihist]->GetBinError(ibin);
  // 	  if((err1 || err2) && val1+val2) {
  // 	    sumChi2 += 2.0*(val1-val2)*(val1-val2)/fabs(val1+val2); // symmetric formula
  // 	    ndf++;
  // 	  }
  // 	}
  //     }
  //   }
// #if(DEBUG_CHI2)
//     std::cout<<__FUNCTION__<<": Sum of chi2 from "<<nhist<<" histograms = "<<sumChi2<<", ndf="<<ndf<<std::endl;
// #endif
  //   return sumChi2/ndf;
  // }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // double getChi2() {
  //   const int nhist=myRefHistosInMM.size();
  //   double sumChi2=0.0;
  //   auto ref_hits=0L;
  //   auto fit_hits=0L;
  //   auto ndf=0L;
  //   for(auto ihist=0; ihist<nhist; ihist++) {
  //     for(auto ix=1; ix<=myRefHistosInMM[ihist]->GetNbinsX(); ix++) {
  // 	for(auto iy=1; iy<=myRefHistosInMM[ihist]->GetNbinsY(); iy++) {
  // 	  //	  auto found=false;
  // 	  auto ibin=myRefHistosInMM[ihist]->GetBin(ix, iy);
  // 	  auto val1=myRefHistosInMM[ihist]->GetBinContent(ibin);
  // 	  auto err1=myRefHistosInMM[ihist]->GetBinError(ibin);
  // 	  auto val2=myFitHistosInMM[ihist]->GetBinContent(ibin);
  // 	  auto err2=myFitHistosInMM[ihist]->GetBinError(ibin);
  // 	  // if(fabs(val1)>1) {
  // 	  //   ref_hits++;
  // 	  //   found=true;
  // 	  // }
  // 	  // if(fabs(val2)>1) {
  // 	  //   fit_hits++;
  // 	  //   found=true;
  // 	  // }
  // 	  if(err1 || err2) {
  // 	    ndf++;
  // 	    sumChi2+=(val1-val2)*(val1-val2)/(err1*err1+err2*err2);
  // 	  }
  // 	}
  //     }
  //   }
  // #if(DEBUG_CHI2)
  //     std::cout<<__FUNCTION__<<": Sum of chi2 from "<<nhist<<" histograms = "<<sumChi2<<", ndf="<<ndf<<std::endl;
  // #endif
  //   return sumChi2/ndf;
  // }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // double getChi2() {
  //   const int nhist=myRefHistosInMM.size();
  //   double sumChi2=0.0;
  //   for(auto ihist=0; ihist<nhist; ihist++) {
  //     double chi2=0.0;
  //     int ndf=0;
  //     int igood=0;
  //     double p = myRefHistosInMM[ihist]->Chi2TestX(myFitHistosInMM[ihist], chi2, ndf, igood, "WW P");
  //     //      std::cout<<__FUNCTION__<<": DIR=" << ihist << ", prob=" << p << ", igood=" << igood << ", chi2=" << chi2 << ", ndf=" << ndf << std::endl;
  //     //      sumChi2 += chi2/ndf;
  //     sumChi2 += chi2;
  //   }
  //   std::cout<<__FUNCTION__<<": Sum of chi2 from "<<nhist<<" histograms = "<<sumChi2<<std::endl;
  //   //    return sumChi2/nhist;
  //   return sumChi2;
  // }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // generates Track3D collection out of current fit parameters
  void convertParamsToTracks(const int npar, const double *par) {

    // sanity checks
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
    myScale=par[0]; // set global scaling factor for dE/dx
    myTrackFit=aTrack; // set track collection 
  }
  
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  // implementation of the function to be minimized
  double operator() (const double *par) {

    // convert params to new Track3D collection
    convertParamsToTracks(myNparams, par);

    // convert current Track3D collection to new set of TH2D UVW projections
    fillFitHistogramsInMM();

    // calcualte (reduced) CHI2 for new TH2D histograms
    return getChi2();
  }
};

// _______________________________________
//
// Define simple data structure for N-prong fit (1<=N<=3)
//
typedef struct {Float_t eventId, runId, ntracks,
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
// This function: generates and fits N (1<=N<=3) straight pseudo-tracks.having common vertex
// It returns CHI2 of the fit.
// It also fills FitDebugData3prong structure.
//
double fit_Nprong(std::vector<std::shared_ptr<TH2D> > &referenceHistosInMM,
		  std::shared_ptr<GeometryTPC> geo,
		  std::shared_ptr<StripResponseCalculator> calcResponse,
		  std::shared_ptr<IonRangeCalculator> calcRange,
		  std::vector<pid_type> pidList, // determines hypothesis and number of tracks
		  std::vector<double> initialParameters, // starting points of parametric fit
		  double maxDeviationInMM, // for setting parameter limits
		  double tolerance,
		  FitDebugData3prong &fit_debug_data
		  ) {

  // initilize fitter
  //
  HypothesisFit myFit(referenceHistosInMM, calcResponse, calcRange, pidList);
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

  fitter.SetFCN(fcn, pStart, myFit.getRefNpoints(), true);

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
  // to be within factor of 2 from initial guess value
  //  fitter.Config().ParSettings(0).SetLimits(0.5*pStart[0], 2.0*pStart[0]);
  fitter.Config().ParSettings(0).SetLimits(0.1*pStart[0], 10.0*pStart[0]);
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
      parmin = std::max( xmin, pStart[i]-maxDeviationInMM );
      parmax = std::min( xmax, pStart[i]+maxDeviationInMM );
      break;
    case 1: // Y_DET
      parmin = std::max( ymin, pStart[i]-maxDeviationInMM );
      parmax = std::min( ymax, pStart[i]+maxDeviationInMM );
      break;
    case 2: // Z_DET
      parmin = std::max( zmin, pStart[i]-maxDeviationInMM );
      parmax = std::min( zmax, pStart[i]+maxDeviationInMM );
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
  fitter.Config().MinimizerOptions().SetTolerance(tolerance); // default=1e-4 for MC-MC tests

  // set print debug level
  fitter.Config().MinimizerOptions().SetPrintLevel(2);

  bool ok=false;

  ////////// STEP 1 - Minuit2 / Fumili2
  //
  for (int i = 0; i < myFit.getNparams(); ++i) {
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
  fitter.Config().MinimizerOptions().SetTolerance(tolerance); // default=1e-4 for MC-MC comparison
  fitter.Config().MinimizerOptions().SetPrecision( 1e-6 ); // 1e-4 //increased to speed up calculations

  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit ok."<<std::endl;
  }

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
  return result.MinFcnValue();
}

// _______________________________________
//
int loop(const long maxIter=1, const int runId=1234) {

  gRandom->SetSeed(0);
  
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
  const auto peakingTime_ns=0.0; // ns
  const std::string geometryFileName = "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat";
  const auto pressure_mbar = 250.0; // mbar
  const auto temperature_K = 273.15+20; // K

  const auto tolerance=1e-4; // default=1e-4 for MC-MC comparison
  const auto reference_adcPerMeV=1e5;
  const auto reference_nPointsMin=1000; // fine granularity for crearing reference UVW histograms to be fitted
  const auto reference_nPointsPerMM=100.0; // fine granularity for creating reference UVW histograms to be fitted
  const auto maxDeviationInADC=reference_adcPerMeV*0.5; // for smearing starting point and setting parameter limits
  const auto maxDeviationInMM=5.0; // for smearing fit starting point and setting parameter limits

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
  int nstrips=6;
  int ncells=30;
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

  // sanity check before main loop
  if(pidList.size()<1 || pidList.size()>3) {
    std::cout << __FUNCTION__ << ": Invalid number of tracks for FitDebugData3prong!" << std::endl << std::flush;
    exit(1);
  }

  // measure elapsed time
  TStopwatch t;
  t.Start();

  /////////// pereform fits of randomly generated events
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
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart, maxDeviationInMM, tolerance, fit_debug_data);

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

  return 0;
}

// _______________________________________
//
// Takes Track3D input tree as a starting point to fit the corresponding GRAW raw-data file(s).
// Events from the two sources are are matched by their {runId, eventId}.
//
int loop_Graw_Track3D(const char *recoInputFile, // Track3D collection with initial RECO data corresponding to GRAW file(s)
		      const char *grawInputFile, // comma-separated list of GRAW files
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
		      bool flag_3prong=true  // enable fitting of 3-prong events
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
  filter_type filterType = filter_type::threshold;
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
  hitFilterConfig.put("recoClusterEnable", true);
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
  const auto maxDeviationInADC=reference_adcPerMeV*0.5; // for smearing starting point and setting parameter limits
  const auto maxDeviationInMM=5.0; // for smearing fit starting point and setting parameter limits
  double sigmaXY=sigmaXY_mm; // 0.64; // educated guess of transverse charge spread after 10 cm of drift (middle of drift cage)
  double sigmaZ=sigmaZ_mm; // 0.64; // educated guess of longitudinal charge spread after 10 cm of drift (middle of drift cage)
  auto peakingTime=peakingTime_ns;
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  int nstrips=6;
  int ncells=30;
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

  // ////////// create dummy EventTPC to get empty UVW projections
  // //
  // auto event=std::make_shared<EventTPC>(); // empty event
  // event->SetGeoPtr(aGeometry);
  // std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
  // for(auto strip_dir=0; strip_dir<3; strip_dir++) {
  //   referenceHistosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm);
  // }

  //////// initialize event filter
  //
  auto beamDirPreset=BeamDirection::MINUS_X;
  TVector3 beamDir; // unit vector in DET coordinate system
  switch(beamDirPreset){
  case BeamDirection::X :
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
  // NOTE: Tree's and branch names are kept the same as in HIGS_analysis
  //       for easy MC-reco comparison. To be replaced with better
  //
  //       class/object in future toy MC.
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
  const char del = ','; // delimiter character
  std::set<std::string> fileNameList; // list of unique strings
  std::stringstream sstream(grawInputFile);
  std::string fileName;
  while (std::getline(sstream, fileName, del)) {
    if(fileName.size()>0) fileNameList.insert(fileName);
  };
  const int frameLoadRange=150; // important for single GRAW file mode
  const unsigned int AsadNboards=aGeometry->GetAsadNboards();
  std::shared_ptr<EventSourceBase> myEventSource;
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
  dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(true);
  dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(pedestalConfig);

  myEventSource->loadDataFile(grawInputFile);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
	    << std::endl;
  myEventSource->loadFileEntry(0); // load 1st frame (NOTE: otherwise LoadEventId does not work)

  // DEBUG - parsing RunId from file name
  auto id = RunIdParser(grawInputFile);
  std::cout << "Parsing whole file name list: " << grawInputFile
	    << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << std::endl;
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

    std::cout << "EventInfo: " << aEventTPC->GetEventInfo()<<std::endl;

    // prepare clustered UVW projections to be fitted
    // NOTE: ptree::find() with nested nodes does not work properly in interactive ROOT mode!
    //       Below is a workaround employing simple node.
    //    aEventTPC->setHitFilterConfig(filterType, aConfig.find("hitFilter")->second);
    aEventTPC->setHitFilterConfig(filterType, hitFilterConfig);
    std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir] = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filter_type::threshold, scale_type::mm);
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
    const auto tolerance = 1e-5; // default=1e-4 for MC-MC comparison
    const auto deviationDir=getRandomDir(gRandom); // for origin, enpoints
    //    const auto deviationStepInMM=1.0; // [mm]
    //    auto deviationRadiusInMM = deviationStepInMM * (gRandom->Integer((int)(maxDeviationInMM/deviationStepInMM))+1);
    auto deviationRadiusInMM = 0.0; // no deviation
    auto maxDeviationInMM=7.0; // [mm] - for setting perameter limits (and for optional smearing of the starting points)
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

    // get fit results
    auto chi2 = fit_Nprong(referenceHistosInMM, aGeometry, aCalcResponse, aCalcRange, pidList, pStart, maxDeviationInMM, tolerance, fit_debug_data);
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

    //////// discard UVW projectons that are not needed anymore
    //
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      referenceHistosInMM[strip_dir].reset();
    }

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

  return 0;
}
