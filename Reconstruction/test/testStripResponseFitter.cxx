//
// Example of using ROOT fitter with PEventTPC, Track3D, StripResponseCalculator, IonRangeCalculator
//
// Mikolaj Cwiok (UW) - 25 Feb 2023
//
//

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <vector>
#include <TMath.h>
#include <TRandom3.h>
//#include <TStyle.h>
#include <TCanvas.h> // DEBUG
#include <TFile.h> // DEBUG
#include <TGraph.h> // DEBUG
#include <TString.h>
#include <TH2D.h>
#include <TVector3.h>
//#include <TLine.h>
#include <TFile.h>
#include <TBranch.h>
#include <Math/Functor.h>
#include <Fit/Fitter.h>

#include "CommonDefinitions.h"
#include "GeometryTPC.h"
#include "EventTPC.h"
#include "TrackSegment3D.h"
#include "Track3D.h"
#include "IonRangeCalculator.h"
#include "StripResponseCalculator.h"
#include "UtilsMath.h"

#define DEBUG_CHARGE    false
#define DEBUG_SIGNAL    false
#define DEBUG_PAR0      false
#define DEBUG_CHI2      false

using namespace ROOT::Math;

class GeometryTPC;
class StripResponseCalculator;
class IonRangeCalculator;
class TrackSegment3D;
class Track3D;

TVector3 getRandomDir(TRandom *r) {
  if(!r) {
    std::cout << __FUNCTION__ << ": Wrong random generator pointer!" << std::endl;
    exit(-1);
  }
  TVector3 unit_vec;
  unit_vec.SetMagThetaPhi(1.0, r->Uniform(-1.0, 1.0), r->Uniform(0.0, TMath::TwoPi()));
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

    //// DEBUG
    //    std::cout << __FUNCTION__ << ": myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    //    for(auto &it: myFitHistosInMM) {
    //      std::cout << " " << it << std::flush;
    //    }
    //    std::cout << std::endl << std::flush;
    //// DEBUG

    // reset histograms
    for(auto strip_dir=0; strip_dir<3; strip_dir++) {
      myFitHistosInMM[strip_dir]->Reset();
    }
    
    // loop over hypothesis tracks
    const int ntracks = myTrackFit.getSegments().size();
    TrackSegment3DCollection list = myTrackFit.getSegments();

    //// DEBUG
    //    std::cout << __FUNCTION__ << ": Fitted ntracks=" << ntracks << std::endl << std::flush;
    //// DEBUG

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
#if defined(DEBUG_SIGNAL) && DEBUG_SIGNAL
    TFile f("fit_ref_histos.root", "RECREATE");
    f.cd();
    TCanvas c("c", "c", 1200, 400);
    c.Divide(3,1);
    int ipad=0;
    for(auto &it: myRefHistosInMM) {
      ipad++;
      c.cd(ipad);
      auto h=(TH2D*)(it->DrawClone("COLZ"));
      h->SetDirectory(0);    
    }
    c.Update();
    c.Modified();
    c.Write();
    c.Clear();
    c.Divide(3,1);
    ipad=0;
    for(auto &it: myFitHistosInMM) {
      ipad++;
      c.cd(ipad);
      auto h=(TH2D*)(it->DrawClone("COLZ"));
      h->SetDirectory(0);    
    }
    c.Update();
    c.Modified();
    c.Write();
    f.Close();
#endif
    //// DEBUG

  }

  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  void initializeFitHistogramsInMM() {

    //// DEBUG
    // std::cout << __FUNCTION__ << ": Before RESET myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    //// DEBUG
    
    // reset
    for(auto &it: myFitHistosInMM) {
      if(it) {
	delete it; // it->Delete();
      }
    }
    myFitHistosInMM.clear();
    
    //// DEBUG
    // std::cout << __FUNCTION__ << ": After RESET myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    //// DEBUG
    
    // create empty TH2D projections
    for(auto &it: myRefHistosInMM) {
      if(!it) {
	std::cout<<__FUNCTION__<<": Wrong external reference TH2D pointer!"<<std::endl;
	exit(-1);
      } else {
	auto h=(TH2D*)(it->Clone(Form("fit_%s",(it)->GetName())));
	//	auto h=new TH2D(*it);
	//	auto h=it;
	if(!h) {
	  std::cout<<__FUNCTION__<<": Cannot duplicate external reference TH2D histogram!"<<std::endl;
	  exit(-1);
	}
	h->Sumw2(true);
	h->Reset();
       myFitHistosInMM.push_back(h);
      }
    }

    //// DEBUG
    // std::cout << __FUNCTION__ << ": After CLONE myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    //// DEBUG

    // associate TH2D projections with StripResponseCalculator
    myResponseCalc->setUVWprojectionsInMM(myFitHistosInMM);

    //// DEBUG
    // std::cout << __FUNCTION__ << ": After RESPONSE REGISTER myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    //// DEBUG
  }
  
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  unsigned long countNonEmptyBins(TH1 *h) const {
    unsigned long result=0;
    switch (h->GetDimension()) {
    case 1:
      for(auto iBin=1; iBin <= h->GetNbinsX(); iBin++) {
	if(h->GetBinError(iBin)) result++;
      }
      break;
    case 2:
      for(auto iBinX=1; iBinX <= h->GetNbinsX(); iBinX++) {
	for(auto iBinY=1; iBinY <= h->GetNbinsY(); iBinY++) {
	  if(h->GetBinError(iBinX, iBinY)) result++;
	}
      }
      break;
    default:
      std::cout << __FUNCTION__ << ": ERROR: Wrong histogram dimension!" << std::endl;
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
      std::cout<<__FUNCTION__<<": Wrong number of external UVW reference histograms!"<<std::endl;
      exit(-1);		   
    }
    for(auto &it: reference_data) {
      if(!it) {
	std::cout<<__FUNCTION__<<": Wrong pointer of external UVW reference histogram!"<<std::endl;
	exit(-1);	
      }
      myRefHistosInMM.push_back(it);
      myRefNpoints += countNonEmptyBins(it);
    }
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
      std::cout<<__FUNCTION__<<": Wrong ion range calcualtor!"<<std::endl;
      exit(-1);		   
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
      std::cout<<__FUNCTION__<<": Wrong number of particles in RECO hypothesis!"<<std::endl;
      exit(-1);		   
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
    //// DEBUG
    // std::cout << __FUNCTION__ << ": STD_SHARED AAA" << std::endl << std::flush;
    //// DEBUG
    setRefHistogramsInMM(reference_data);
    setCalculators(aResponseCalc, aRangeCalc);
    setParticleList(aParticleList); 
    initializeFitHistogramsInMM();
    //// DEBUG
    //  std::cout << __FUNCTION__ << ": STD_SHARED BBB myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    // std::cout << __FUNCTION__ << ": STD_SHARED BBB myRefHistosInMM.size()=" << myRefHistosInMM.size() << std::flush;
    // for(auto &it: myRefHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    // std::cout << __FUNCTION__ << ": STD_SHARED BBB" << std::endl << std::flush;
    //// DEBUG
  }
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
  HypothesisFit(const std::vector<TH2D*> &reference_data,
		const std::shared_ptr<StripResponseCalculator> &aResponseCalc,
		const std::shared_ptr<IonRangeCalculator> &aRangeCalc,
		const std::vector<pid_type> &aParticleList)
    : myRefHistosInMM(reference_data)
  {
    //// DEBUG
    // std::cout << __FUNCTION__ << ": AAA" << std::endl << std::flush;
    //// DEBUG
    setRefHistogramsInMM(reference_data);
    setCalculators(aResponseCalc, aRangeCalc);
    setParticleList(aParticleList);
     initializeFitHistogramsInMM();
    //// DEBUG
    //  std::cout << __FUNCTION__ << ": BBB myFitHistosInMM.size()=" << myFitHistosInMM.size() << std::flush;
    // for(auto &it: myFitHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    // std::cout << __FUNCTION__ << ": BBB myRefHistosInMM.size()=" << myRefHistosInMM.size() << std::flush;
    // for(auto &it: myRefHistosInMM) {
    //   std::cout << " " << it << std::flush;
    // }
    // std::cout << std::endl << std::flush;
    // std::cout << __FUNCTION__ << ": BBB" << std::endl << std::flush;
    //// DEBUG
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
      std::cout<<__FUNCTION__<<" Wrong input NPAR or PAR value!"<<std::endl;
      exit(-1);
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
// Define simple data structure
//
typedef struct {Float_t eventId, runId,
    energy, // MeV
    pid,
    chi2,
    ndf,
    status,
    ncalls,
    scaleTrue, // ADC/MeV
    lengthTrue, // mm
    phiTrue, // rad
    cosThetaTrue,
    xVtxTrue, yVtxTrue, zVtxTrue, // mm
    xEndTrue, yEndTrue, zEndTrue, // mm
    scaleReco, // ADC/MeV
    lengthReco, // mm
    phiReco, // rad
    cosThetaReco,
    xVtxReco, yVtxReco, zVtxReco, // mm
    xEndReco, yEndReco, zEndReco, // mm
    initScaleDeviation, // intial conditions: fabs(difference from TRUE) [ADC/MeV]
    initVtxDeviation, // initial conditions: radius from TRUE vertex position [mm]
    initEndDeviation; // initial conditions: radius from TRUE endpoint position [mm]
    } FitDebugData;
/////////////////////////

// _______________________________________
//
// This function: generates and fits a single, straight pseudo-track.
// It returns CHI2 of the fit.
// It also fills FitDebugData structure.
//
double fit_1prong(FitDebugData &fit_debug_data,
		  TVector3 origin,
		  pid_type pid,
		  double E_MeV,
		  std::shared_ptr<GeometryTPC> geo,
		  double pressure_mbar=250.0,
		  double temperature_K=273.15+20,
		  double sigmaXY_mm=2.0, // 0.68,
		  double sigmaZ_mm=2.0, // 0.69,
		  double peakingTime=0 // 232
		  ) {

  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){ // FIX
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }

  // initialize TPC geometry and electronics parameters
  //
  //  auto geo=std::make_shared<GeometryTPC>(geometryFile, false);
  //  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  // initialize strip response calculator
  //
  double sigmaXY=sigmaXY_mm; // 0.64; // educated guess of transverse charge spread after 10 cm of drift (middle of drift cage)
  double sigmaZ=sigmaZ_mm; // 0.64; // educated guess of longitudinal charge spread after 10 cm of drift (middle of drift cage)
  if(peakingTime<0) peakingTime=0; // when peakingTime is ommitted turn off additional smearing due to GET electronics
  int nstrips=6;
  int ncells=30;
  int npads=nstrips*2;
  const std::string initFile( (peakingTime==0 ?
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm.root",
				    nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				    sigmaXY, sigmaZ) :
			       Form("StripResponseModel_%dx%dx%d_S%gMHz_V%gcmus_T%gmm_L%gmm_P%gns.root",
				    nstrips, ncells, npads, geo->GetSamplingRate(), geo->GetDriftVelocity(),
				    sigmaXY, sigmaZ, peakingTime) ) );
  auto calcResponse=std::make_shared<StripResponseCalculator>(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, peakingTime, initFile.c_str());
  std::cout << "Loading strip response matrix from file: "<< initFile << std::endl;

  // initialize ion range and dE/dx calculator
  //
  auto calcRange=std::make_shared<IonRangeCalculator>(CO2, pressure_mbar, temperature_K, false);

  // Create dummy EventTPCto get empty UVW projections
  //
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(geo);
  std::vector<std::shared_ptr<TH2D> > referenceHistosInMM(3);
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    referenceHistosInMM[strip_dir] = event->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::mm);
  }
  for(auto &it: referenceHistosInMM) {
    if(!it) {
      std::cout << __FUNCTION__ << ": Cannot create empty reference UVW histogram!" << std::endl;
      exit(-1);
    }
    it->Sumw2(true);
    it->Reset();
  }
  calcResponse->setUVWprojectionsInMM(referenceHistosInMM);
  
  // Create single pseudo-track at given vertex position in DET coordinates, and at given kinetic energy in LAB frame.
  //
  const auto reference_nPointsMin=1000; // fine granularity for crearing reference UVW histograms to be fitted
  const auto reference_nPointsPerMM=100.0; // fine granularity for creating reference UVW histograms to be fitted
  std::vector<pid_type> pidList{pid};
  const auto reference_origin=origin; // mm
  const auto length=calcRange->getIonRangeMM(pid, E_MeV); // mm
  //  auto length=length_MM; // mm
  //  auto Ekin_LAB=calcRange->getIonEnergyMeV(pid, length); // MeV, Ekin should be equal to Bragg curve integral
  //  const auto phi=gRandom->Uniform(0., TMath::TwoPi()); // rad
  //  const auto theta=acos(gRandom->Uniform(-1.0, 1.0)); // rad
  //  TVector3 unit_vec;
  //  unit_vec.SetMagThetaPhi(1.0, theta, phi);
  auto unit_vec=getRandomDir(gRandom);
  auto reference_endpoint=reference_origin+unit_vec*length;
  const int reference_npoints=std::max((int)(reference_nPointsPerMM*length+0.5), reference_nPointsMin);
  const double reference_adcPerMeV=1e5;
  auto curve(calcRange->getIonBraggCurveMeVPerMM(pidList.front(), E_MeV, reference_npoints)); // MeV/mm
  //  auto curve(calcRange->getIonBraggCurveMeVPerMM(pidList.front(), Ekin_LAB, reference_npoints)); // MeV/mm
  for(auto ipoint=0; ipoint<reference_npoints; ipoint++) { // generate NPOINTS hits along the track
    auto depth=(ipoint+0.5)*length/reference_npoints; // mm
    auto hitPosition=reference_origin+unit_vec*depth; // mm
    auto hitCharge=reference_adcPerMeV*curve.Eval(depth)*(length/reference_npoints); // ADC units, arbitrary scaling factor
    calcResponse->addCharge(hitPosition, hitCharge);
  }
  //// DEBUG
  // std::cout<<__FUNCTION__<<": length=" << length << ", npoints=" << reference_npoints << std::endl;
  // reference_origin.Print();
  // unit_vec.Print();
  // for(auto &it: referenceHistosInMM) {
  //   std::cout << " referenceHistosInMM[*]: GetIntegral=" << it->Integral() << ", GetEntries=" << it->GetEntries() << ", GetEffectiveEntries=" << it->GetEffectiveEntries() << std::endl;
  // }
  //// DEBUG
  
  // initilize fitter
  //
  HypothesisFit myFit(referenceHistosInMM, calcResponse, calcRange, pidList);
  const int npar=myFit.getNparams();
  ROOT::Fit::Fitter fitter;
  ROOT::Math::Functor fcn(myFit, npar); // this line must be called after fixing all local charge density function parameters

  const double maxDeviationInADC=reference_adcPerMeV*0.5; // for smearing starting point and setting parameter limits
  const double maxDeviationInMM=5.0; // for smearing fit starting point and setting parameter limits
  //  double pStart[] = { reference_adcPerMeV, reference_origin.X(), reference_origin.Y(), reference_origin.Z(), reference_endpoint.X(), reference_endpoint.Y(), reference_endpoint.Z() };
  // double pStart[] = { reference_adcPerMeV,
  // 		      reference_origin.X()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM),
  // 		      reference_origin.Y()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM),
  // 		      reference_origin.Z()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM),
  // 		      reference_endpoint.X()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM),
  // 		      reference_endpoint.Y()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM),
  // 		      reference_endpoint.Z()+gRandom->Uniform(-maxDeviationInMM, maxDeviationInMM) };
  TVector3 deviationDir[] = { getRandomDir(gRandom), getRandomDir(gRandom) }; // origin, endpoint
  const auto deviationStepInMM = 1.0; // [mm]
  auto deviationRadiusInMM = deviationStepInMM * (gRandom->Integer((int)(maxDeviationInMM/deviationStepInMM))+1);
  double pStart[] = {
    reference_adcPerMeV+gRandom->Uniform(-maxDeviationInADC, maxDeviationInADC),
    reference_origin.X() + deviationRadiusInMM*deviationDir[0].X(),
    reference_origin.Y() + deviationRadiusInMM*deviationDir[0].Y(),
    reference_origin.Z() + deviationRadiusInMM*deviationDir[0].Z(),
    reference_endpoint.X() + deviationRadiusInMM*deviationDir[1].X(),
    reference_endpoint.Y() + deviationRadiusInMM*deviationDir[1].Y(),
    reference_endpoint.Z() + deviationRadiusInMM*deviationDir[1].Z() };

  //// DEBUG
#if defined(DEBUG_PAR0) && DEBUG_PAR0
  std::cout << __FUNCTION__<< ": Initial CHI2=" << myFit(pStart) << std::endl;
  TFile f("chi2_vs_par0_graph.root", "RECREATE");
  f.cd();
  TCanvas c("c", "c", 1200, 1200);
  int np=1000;
  TGraph tg;
  for(auto i=0; i<=np; i++) {
    auto par0 = adcPerMeV-0.5*adcPerMeV*(0.5-1.0*i/np);
    pStart[0]=par0;
    tg.SetPoint(tg.GetN(), par0, fitter.myFit(pStart));
  }
  tg.Draw("AL");
  c.Update();
  c.Modified();
  c.Write();
  f.Close();
  return 0;
#endif
  //// DEBUG
  
  //  fitter.SetFCN(fcn, pStart, 0, true);
  fitter.SetFCN(fcn, pStart, myFit.getRefNpoints(), true);

  // compute better ADC scaling factor
  double ref_integral=0.0; // integral of reference (data) histograms
  for(auto &it: referenceHistosInMM) { ref_integral += it->Integral(); }
  double fit_integral=0.0; // integral of initial fitted histograms
  double chi2=myFit(pStart);
  std::cout << __FUNCTION__ << ": CHI2 evaluated at INITIAL starting point ="<<chi2<<std::endl; 
  for(auto &it: myFit.getFitHistogramsInMM()) { fit_integral += it->Integral(); }
  double corr_factor=ref_integral/fit_integral; // correction factor for ADC scale
  std::cout << __FUNCTION__ << ": Multiplying original ADC scaling factor by "<<corr_factor<<std::endl;
  pStart[0]*=corr_factor;
  fitter.Config().ParSettings(0).SetValue(pStart[0]);
  chi2=myFit(pStart);
  std::cout << __FUNCTION__ << ": CHI2 evaluated at CORRECTED starting point ="<<chi2<<std::endl;

  // set limits on the fitted ADC scale
  // to be within factor of 2 from initial guess value
  fitter.Config().ParSettings(0).SetLimits(0.5*pStart[0], 2.0*pStart[0]);
  std::cout<<"Setting par["<<0<<"]  limits=["
	   <<fitter.Config().ParSettings(0).LowerLimit()<<", "
	   <<fitter.Config().ParSettings(0).UpperLimit()<<"]"<<std::endl;    

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
      std::cout << __FUNCTION__ << ": ERROR: Invalid coordinate index!" << std::endl;
      exit(1);
    };
    fitter.Config().ParSettings(i).SetLimits(parmin, parmax);
    std::cout<<"Setting par["<<i<<"]  limits=["
	     <<fitter.Config().ParSettings(i).LowerLimit()<<", "
	     <<fitter.Config().ParSettings(i).UpperLimit()<<"]"<<std::endl;
  }

  // set step sizes
  //  fitter.Config().ParSettings(0).SetStepSize(1.0); // step for ADC scaling factor [ADC/MeV]
  fitter.Config().ParSettings(0).SetStepSize(100); // step for ADC scaling factor [ADC/MeV]
  for (int i = 1; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).SetStepSize(0.1); // step for spatial dimensions [mm]
  }

  // set numerical tolerance
  fitter.Config().MinimizerOptions().SetTolerance( 1e-4 ); // TEST

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
  fitter.Config().MinimizerOptions().SetTolerance( 1e-4 ); // 1e-3 // increased to speed up calculations
  fitter.Config().MinimizerOptions().SetPrecision( 1e-6 ); // 1e-4 //increased to speed up calculations

  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 1 (Minuit2/Fumili2) --> Fit ok."<<std::endl;
  }

  /*
  ////////// STEP 1 - SIMPLEX - free scale, fixed vertex, endpoint
  //
  std::cout<<"Releasing par["<<0<<"]"<<std::endl;
  fitter.Config().ParSettings(0).Release();
  for (int i = 1; i < myFit.getNparams(); ++i) {
    std::cout<<"Fixing par["<<i<<"]="<<fitter.Config().ParSettings(i).Value()<<std::endl;
    fitter.Config().ParSettings(i).Fix();
  }
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  //  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Migrad"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Minimize"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // default
  //  fitter.Config().MinimizerOptions().SetStrategy(2); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 2 - MIGRAD - free scale, fixed vertex, endpoint
  //
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // default
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 3 - MINIMIZE - free scale, fixed vertex, endpoint
  //
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 4 - SIMPLEX - fixed scale, vertex, free endpoint
  //
  std::cout<<"Fixing par["<<0<<"]="<<fitter.Config().ParSettings(0).Value()<<std::endl;
  fitter.Config().ParSettings(0).Fix();
  for (int i = 4; i < myFit.getNparams(); ++i) {
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
    fitter.Config().ParSettings(i).Release();
  }
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 5 - SIMPLEX - fixed scale, endpoint, free vertex
  //
  std::cout<<"Fixing par["<<0<<"]="<<fitter.Config().ParSettings(0).Value()<<std::endl;
  fitter.Config().ParSettings(0).Fix();
  for (int i = 1; i < 4; ++i) {
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
    fitter.Config().ParSettings(i).Release();
  }
  for (int i = 4; i < myFit.getNparams(); ++i) {
    std::cout<<"Fixing par["<<i<<"]="<<fitter.Config().ParSettings(i).Value()<<std::endl;
    fitter.Config().ParSettings(i).Fix();
  }
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 5 - SIMPLEX - fixed scale, free endpoint, vertex
  //
  std::cout<<"Fixing par["<<0<<"]="<<fitter.Config().ParSettings(0).Value()<<std::endl;
  fitter.Config().ParSettings(0).Fix();
  for (int i = 1; i < myFit.getNparams(); ++i) {
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
    fitter.Config().ParSettings(i).Release();
  }
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }

  ////////// STEP 6 - SIMPLEX - fixed scale, free vertex, endpoint
  //
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // default
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": Fit failed."<<std::endl;
    //    return 1;
  }
*/
  /*
  ////////// NEW STEP 1 - SCAN - free scale, vertex and endpoints
  //
  for (int i = 0; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).Release(); // free scale, vertex, endpoints
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
  }
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Scan"); // TEST
  //  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Migrad"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Minimize"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(1000); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // default
  //  fitter.Config().MinimizerOptions().SetStrategy(2); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 1 --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 1 --> Fit ok."<<std::endl;
  }

  ////////// NEW STEP 7 - SIMPLEX - fix scale, free endpoint, vertex
  //
  fitter.Config().ParSettings(0).Fix(); // release ADC scale
  std::cout<<"Fixing par["<<0<<"]="<<fitter.Config().ParSettings(0).Value()<<std::endl;
  for (int i = 1; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).Release(); // free vertex, endpoints
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
  }  
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(100); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  //  fitter.Config().MinimizerOptions().SetStrategy(2); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 7 --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 7 --> Fit ok."<<std::endl;
  }

  ////////// NEW STEP 8 - SIMPLEX - free scale, fixed endpoint, vertex
  //
  fitter.Config().ParSettings(0).Release(); // release ADC scale
  std::cout<<"Releasing par["<<0<<"]"<<std::endl;
  for (int i = 1; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).Fix(); // fixed vertex, endpoints
    std::cout<<"Fixing par["<<i<<"]="<<fitter.Config().ParSettings(i).Value()<<std::endl;
  }
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(100); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 8 --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 8 --> Fit ok."<<std::endl;
  }

  ////////// NEW STEP 9 - SIMPLEX - free scale, fixed endpoint, vertex
  //
  for (int i = 0; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).Release(); // free scale, vertex, endpoints
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
  }
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Simplex"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(100); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(1); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 9 --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 9 --> Fit ok."<<std::endl;
  }

  ////////// NEW STEP 10 - MINIMIZE=SIMPLEX+MIGRAD - free scale, endpoint, vertex
  //
  for (int i = 0; i < myFit.getNparams(); ++i) {
    fitter.Config().ParSettings(i).Release(); // free scale, vertex, endpoints
    std::cout<<"Releasing par["<<i<<"]"<<std::endl;
  }
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit"); // default
  //  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2"); // TEST
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Minimize"); // TEST
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000); // TEST
  fitter.Config().MinimizerOptions().SetMaxIterations(100); // TEST
  fitter.Config().MinimizerOptions().SetStrategy(2); // TEST
  // check fit results
  ok = fitter.FitFCN();
  if (!ok) {
    std::cout<<__FUNCTION__<<": STEP 10 --> Fit failed."<<std::endl;
    //    return 1;
  } else {
    std::cout<<__FUNCTION__<<": STEP 10 --> Fit ok."<<std::endl;
  }
  */
  // get fit parameters
  const ROOT::Fit::FitResult & result = fitter.Result();
  const double * parFit = result.GetParams();
  const double * parErr = result.GetErrors();
  
  result.Print(std::cout);

  std::cout << "Final FCN value " << result.MinFcnValue()
	    << " (Status=" << result.Status() << ", Ncalls=" << result.NCalls() << ", Ndf=" << result.Ndf() << ")" << std::endl;
  std::cout << "\nFitted SCALE [ADC/MeV] = " << parFit[0] << " +/- " << parErr[0] << std::endl
	    <<   "  True SCALE [ADC/MeV] = " << reference_adcPerMeV << std::endl;
  std::cout << "\nFitted X_START [mm] = " << parFit[1] << " +/- " << parErr[1] << std::endl
	    <<   "  True X_START [mm] = " << reference_origin.X() << std::endl;
  std::cout << "\nFitted Y_START [mm] = " << parFit[2] << " +/- " << parErr[2] << std::endl
	    <<   "  True Y_START [mm] = " << reference_origin.Y() << std::endl;
  std::cout << "\nFitted Z_START [mm] = " << parFit[3] << " +/- " << parErr[3] << std::endl
	    <<   "  True Z_START [mm] = " << reference_origin.Z() << std::endl;
  std::cout << "\nFitted X_END [mm] = " << parFit[4] << " +/- " << parErr[4] << std::endl
	    <<   "  True X_END [mm] = " << reference_endpoint.X() << std::endl;
  std::cout << "\nFitted Y_END [mm] = " << parFit[5] << " +/- " << parErr[5] << std::endl
	    <<   "  True Y_END [mm] = " << reference_endpoint.Y() << std::endl;
  std::cout << "\nFitted Z_END [mm] = " << parFit[6] << " +/- " << parErr[6] << std::endl
	    <<   "  True Z_END [mm] = " << reference_endpoint.Z() << std::endl;

  // create TrackSegment3D collections with TRUE and RECO information
  TrackSegment3D reference_seg;
  reference_seg.setGeometry(geo);
  reference_seg.setStartEnd(reference_origin, reference_endpoint);
  reference_seg.setPID(pid);
  TrackSegment3D fit_seg;
  fit_seg.setGeometry(geo);
  fit_seg.setStartEnd(TVector3(parFit[1], parFit[2], parFit[3]), TVector3(parFit[4], parFit[5], parFit[6]));
  fit_seg.setPID(pid);

  fit_debug_data.chi2 = result.Chi2();
  fit_debug_data.ndf = 1.*result.Ndf();
  fit_debug_data.status = 1.*result.Status();
  fit_debug_data.ncalls = 1.*result.NCalls();
  fit_debug_data.scaleTrue = reference_adcPerMeV;
  fit_debug_data.lengthTrue = reference_seg.getLength();
  fit_debug_data.phiTrue = reference_seg.getTangent().Phi();
  fit_debug_data.cosThetaTrue = reference_seg.getTangent().CosTheta();
  fit_debug_data.xVtxTrue = reference_seg.getStart().X();
  fit_debug_data.yVtxTrue = reference_seg.getStart().Y();
  fit_debug_data.zVtxTrue = reference_seg.getStart().Z();
  fit_debug_data.xEndTrue = reference_seg.getEnd().X();
  fit_debug_data.yEndTrue = reference_seg.getEnd().Y();
  fit_debug_data.zEndTrue = reference_seg.getEnd().Z();
  fit_debug_data.scaleReco = parFit[0];
  fit_debug_data.lengthReco = fit_seg.getLength();
  fit_debug_data.phiReco = fit_seg.getTangent().Phi();
  fit_debug_data.cosThetaReco = fit_seg.getTangent().CosTheta();
  fit_debug_data.xVtxReco = fit_seg.getStart().X();
  fit_debug_data.yVtxReco = fit_seg.getStart().Y();
  fit_debug_data.zVtxReco = fit_seg.getStart().Z();
  fit_debug_data.xEndReco = fit_seg.getEnd().X();
  fit_debug_data.yEndReco = fit_seg.getEnd().Y();
  fit_debug_data.zEndReco = fit_seg.getEnd().Z();
  fit_debug_data.initScaleDeviation = fabs(pStart[0]-fit_debug_data.scaleTrue);
  fit_debug_data.initVtxDeviation = sqrt( pow(pStart[1]-fit_debug_data.xVtxTrue, 2) +
					  pow(pStart[2]-fit_debug_data.yVtxTrue, 2) +
					  pow(pStart[3]-fit_debug_data.zVtxTrue, 2) );
  fit_debug_data.initEndDeviation = sqrt( pow(pStart[4]-fit_debug_data.xEndTrue, 2) +
					  pow(pStart[5]-fit_debug_data.yEndTrue, 2) +
					  pow(pStart[6]-fit_debug_data.zEndTrue, 2) );
  
  return result.MinFcnValue();
}

// _______________________________________
//
int loop(const long maxIter=1, const int runId=1234) {

  gRandom->SetSeed(0);
  
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){ // FIX
    R__LOAD_LIBRARY(libTPCUtilities.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }

  /////////// input parameters
  //
  const auto pid=CARBON_12; // particle ID
  const auto E=1.0; // particle kinetic energy [MeV]
  // const auto pid=ALPHA; // particle ID
  // const auto E=3.0; // particle kinetic energy [MeV]
  const auto sigmaXY=2.0; // mm
  const auto sigmaZ=2.0; // mm
  const auto peakingTime=0.0; // ns
  const auto pressure = 250.0; // mbar
  const auto temperature = 273.15+20; // K
  const std::string geometryFileName = "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat";

  /////////// initialize TPC geometry, electronic parameters and gas conditions
  //
  auto aGeometry = std::make_shared<GeometryTPC>(geometryFileName.c_str(), false);
  aGeometry->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes

  /////////// set true vertex
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
  FitDebugData fit_debug_data;
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

  /////////// pereform fits of randomly generated events
  //
  for(auto iter=0; iter<maxIter; iter++) {
    std::cout << "#####################" << std::endl
	      << "#### TEST ITER=" << iter << std::endl
	      << "#####################" << std::endl;

    auto chi2 = fit_1prong(fit_debug_data, origin, pid, E,
			   aGeometry,
			   pressure, temperature,
			   sigmaXY, sigmaZ, peakingTime);

    // create TrackSegment3D collections with TRUE and RECO information
    Track3D reference_track3d;
    TrackSegment3D reference_seg;
    reference_seg.setGeometry(aGeometry);
    reference_seg.setStartEnd(TVector3(fit_debug_data.xVtxTrue,fit_debug_data.yVtxTrue,fit_debug_data.zVtxTrue),
			      TVector3(fit_debug_data.xEndTrue,fit_debug_data.yEndTrue,fit_debug_data.zEndTrue));
    reference_seg.setPID(pid);
    reference_track3d.addSegment(reference_seg);
    *trackPtr[0]=reference_track3d;
    Track3D fit_track3d;
    TrackSegment3D fit_seg;
    fit_seg.setGeometry(aGeometry);
    fit_seg.setStartEnd(TVector3(fit_debug_data.xVtxReco,fit_debug_data.yVtxReco,fit_debug_data.zVtxReco),
			TVector3(fit_debug_data.xEndReco,fit_debug_data.yEndReco,fit_debug_data.zEndReco));
    fit_seg.setPID(pid);
    fit_track3d.addSegment(fit_seg);
    *trackPtr[1]=fit_track3d;
    
    outputROOTFile->cd();
    fit_debug_data.eventId = iter;
    fit_debug_data.runId = runId;
    fit_debug_data.energy = E;
    fit_debug_data.pid = pid;
    fit_debug_data.chi2 = chi2;
    tree->Fill();
    
    for(auto ifile=0; ifile<2; ifile++) {
      filePtr[ifile]->cd();
      eventInfoPtr[ifile]->SetRunId(runId); // fake run ID (below 1E6)
      eventInfoPtr[ifile]->SetEventId(iter);
      treePtr[ifile]->Fill();
    }
  }

  ////////// close output ROOT files
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
