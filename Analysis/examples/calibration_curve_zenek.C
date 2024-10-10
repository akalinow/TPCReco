//////////////////////////
//
// root
// root [0] .L calibration_curve_zenek.C
// root [1] calibration_curve_zenek()
//
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <cmath>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TLegend.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLorentzVector.h>
#include <TVector3.h>
#include <TAttMarker.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/IonRangeCalculator.h"

void calibration_curve_zenek(double scale=1.03499, double offset_mm=1.6402,
			     double length_min_mm=20, double length_max_mm=90,
			     double pressure_mbar=250, double temperature_K=273.15+20,
			     const char *beforeFitName=NULL, // optional ROOT file with saved TCanvas with fit results
			     const char *afterFitName=NULL   // optional ROOT file with saved TCanvas with fit results
			     ) {

  if (!gROOT->GetClass("TrackSegment3D")){
     R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
     R__LOAD_LIBRARY(libTPCUtilities.so);
  }

  // initialize range calculator for current (p,T) conditions
  auto rangeCalc=new IonRangeCalculator(CO2, pressure_mbar, temperature_K);
  rangeCalc->setDebug(false);

  // initialize vector of track lengths for current (p,T) conditions
  auto npoints=1000L;
  std::vector<double> length_vec;
  for(auto ipoint=0L; ipoint<npoints; ipoint++) {
    length_vec.push_back(length_min_mm+(length_max_mm-length_min_mm)*ipoint/(npoints-1)); // mm
  }

  auto gr_Ecorr_vs_Euncorr=new TGraph();
  auto gr_Ecorr_vs_range=new TGraph();
  auto gr_Euncorr_vs_range=new TGraph();
  
  // fill graphs
  for(auto &length : length_vec) {
    
    // uncorrected kinetic energy of ALPHA particle 
    auto pid = ALPHA;
    auto T_alpha_LAB = rangeCalc->getIonEnergyMeV(pid, length);
    
    // corrected kinetic energy of ALPHA particle 
    auto length_rescaled =
      std::max(0.0, length + offset_mm
	       * rangeCalc->getGasRangeReferencePressure(pid)/rangeCalc->getGasPressure()
	       * rangeCalc->getGasTemperature()/rangeCalc->getGasRangeReferencePressure(pid));
    auto T_alpha_LAB_corr =  scale * rangeCalc->getIonEnergyMeV(pid, length_rescaled);

    std::cout << "L[mm]=" << length << "  p[mbar]=" << pressure_mbar << "  T[K]=" << temperature_K
	      << " :  E_LAB_uncorr[MeV]=" << T_alpha_LAB << "  E_LAB_corr[MeV]=" << T_alpha_LAB_corr
	      << "  (diff=" << (T_alpha_LAB_corr-T_alpha_LAB)/T_alpha_LAB*100 << " \%)" << std::endl;

    gr_Euncorr_vs_range->SetPoint(gr_Euncorr_vs_range->GetN(), length, T_alpha_LAB);
    gr_Ecorr_vs_range->SetPoint(gr_Ecorr_vs_range->GetN(), length, T_alpha_LAB_corr);
    gr_Ecorr_vs_Euncorr->SetPoint(gr_Ecorr_vs_Euncorr->GetN(), T_alpha_LAB, T_alpha_LAB_corr);
  }

  /////////// Process fit results (if any)
  auto gr_data_Ecorr_vs_Euncorr=new TGraph();
  if(beforeFitName!=NULL && afterFitName!=NULL) {
    
    TFile *_file0 = TFile::Open(beforeFitName);
    TFile *_file1 = TFile::Open(afterFitName);
  
    std::vector<TCanvas*> before={
      (TCanvas*)_file0->Get("point_7")->Clone(),    // 9.56 MeV
      (TCanvas*)_file0->Get("point_8")->Clone(),    // 9.85 MeV
      (TCanvas*)_file0->Get("point_1")->Clone(),    // 11.5 MeV
      (TCanvas*)_file0->Get("point_2")->Clone(),    // 12.3 MeV
      (TCanvas*)_file0->Get("point_3")->Clone(),    // 13.1 MeV
      (TCanvas*)_file0->Get("point_4")->Clone()     // 13.5 MeV
    };
  
    std::vector<TCanvas*> after={
      (TCanvas*)_file1->Get("point_7")->Clone(),    // 9.56 MeV
      (TCanvas*)_file1->Get("point_8")->Clone(),    // 9.85 MeV
      (TCanvas*)_file1->Get("point_1")->Clone(),    // 11.5 MeV
      (TCanvas*)_file1->Get("point_2")->Clone(),    // 12.3 MeV
      (TCanvas*)_file1->Get("point_3")->Clone(),    // 13.1 MeV
      (TCanvas*)_file1->Get("point_4")->Clone()     // 13.5 MeV
    };
  
    if(before.size()==after.size()) {

      TH1D* h=NULL; 
      TF1* f=NULL;
      TCanvas* canvas=NULL;
      
      for(auto ipoint=0; ipoint<after.size(); ipoint++) {
	
	canvas=after.at(ipoint);
	h=(TH1D*)canvas->GetListOfPrimitives()->FindObject("hExcitationEnergy");
	f=(TF1*)h->GetFunction("f_energyShapeGauss");
	auto Ex_corr = f->GetParameter(1); // MeV, corrected Excitation Energy in CMS
	
	canvas=before.at(ipoint);
	h=(TH1D*)canvas->GetListOfPrimitives()->FindObject("hExcitationEnergy");

	// // look for TLine
	// auto list=canvas->GetListOfPrimitives();
	// auto l=(TLine*)list->FindObject("TLine");
	// if(!l) continue;
	// std::cout << "TLine Ex1=" << l->GetX1() << " Ex2=" << l->GetX2() << std::endl;
	// auto Ex_corr = l->GetX1(); // MeV, expected corrected Ex peak position
	
	f=(TF1*)h->GetFunction("f_energyShapeGauss");
	auto Ex_uncorr = f->GetParameter(1); // MeV, uncorrected Excitation Energy in CMS
	
	// convert Ex_CMS to T_alpha_CMS (unrelativistic)
	auto mass_alpha = rangeCalc->getIonMassMeV(ALPHA);
	auto mass_C12 = rangeCalc->getIonMassMeV(CARBON_12);
	auto mass_O16 = rangeCalc->getIonMassMeV(OXYGEN_16);
	auto Qvalue = mass_O16 - mass_alpha - mass_C12; // negative, MeV
	auto T_alpha_CMS_uncorr = (Ex_uncorr+Qvalue) / (1.0 + mass_alpha/mass_C12); // MeV
	auto T_alpha_CMS_corr = (Ex_corr+Qvalue) / (1.0 + mass_alpha/mass_C12); // MeV
	
	// nominal boost
	auto Egamma_CMS_corr = Ex_corr*(1.0 - 0.5*Ex_corr/(mass_O16+Ex_corr)); // MeV
	auto Egamma_LAB_corr = 0.5*( pow(mass_O16 + Ex_corr, 2) / mass_O16 - mass_O16 ); // MeV
	auto beta_CMS_wrt_LAB = Egamma_LAB_corr / (Egamma_LAB_corr+mass_O16)*TVector3(0,0,1); // along Z-axis in BEAM coordinate system
	
	// boost P4 from CMS back to LAB
	auto p_alpha_CMS_uncorr = sqrt(T_alpha_CMS_uncorr*(T_alpha_CMS_uncorr+2*mass_alpha)); // uncorrected momentum in CMS
	TLorentzVector P4_alpha_CMS_uncorr; // BEAM coordinates
	P4_alpha_CMS_uncorr.SetVectM(p_alpha_CMS_uncorr*TVector3(0,0,1), mass_alpha); // uncorrected 4-momentum in CMS corresponding to theta_BEAM=0, BEAM coordinates
	auto p_alpha_CMS_corr = sqrt(T_alpha_CMS_corr*(T_alpha_CMS_corr+2*mass_alpha)); // corrected momentum in CMS
	TLorentzVector P4_alpha_CMS_corr; // BEAM coordinates
	P4_alpha_CMS_corr.SetVectM(p_alpha_CMS_corr*TVector3(0,0,1), mass_alpha); // corrected 4-momentum in CMS corresponding to theta_BEAM=0, BEAM coordinates
	
	auto P4_alpha_LAB_uncorr(P4_alpha_CMS_uncorr); // uncorrected 4-momentum in LAB corresponding to theta_BEAM=0, BEAM coordinates
	P4_alpha_LAB_uncorr.Boost(1.0*beta_CMS_wrt_LAB); // CMS to LAB, see ROOT documentation for sign convention
	auto P4_alpha_LAB_corr(P4_alpha_CMS_corr); // corrected 4-momentum in LAB corresponding to theta_BEAM=0, BEAM coordinates
	P4_alpha_LAB_corr.Boost(1.0*beta_CMS_wrt_LAB); // CMS to LAB, see ROOT documentation for sign convention
	auto T_alpha_LAB_uncorr= P4_alpha_LAB_uncorr.E()-mass_alpha; // MeV, uncorrected kinetic energy in LAB
	auto T_alpha_LAB_corr= P4_alpha_LAB_corr.E()-mass_alpha; // MeV, corrected kinetic energy in LAB
	
	std::cout << "Fitted  Ex_uncorr[MeV]=" << Ex_uncorr << "  Ex_corr[MeV]=" << Ex_corr
		  << " :  E_LAB_uncorr(theta=0)[MeV]=" << T_alpha_LAB_uncorr
		  << "  E_LAB_corr(theta=0)[MeV]=" << T_alpha_LAB_corr
		  << "  (diff=" << (T_alpha_LAB_corr-T_alpha_LAB_uncorr)/T_alpha_LAB_uncorr*100 << " \%)" << std::endl;

	gr_data_Ecorr_vs_Euncorr->SetPoint(gr_data_Ecorr_vs_Euncorr->GetN(), T_alpha_LAB_uncorr, T_alpha_LAB_corr);
      }
    }
  }
  ///////////

  gStyle->SetPadLeftMargin(0.135);
  gStyle->SetPadRightMargin(0.02);
  gStyle->SetPadBottomMargin(0.135);
  gStyle->SetPadTopMargin(0.02);
  gStyle->SetTitleXOffset(1.25);
  gStyle->SetTitleYOffset(1.25);
  gStyle->SetTitleXSize(0.05);
  gStyle->SetTitleYSize(0.05);

  auto c=new TCanvas("c", "c", 800, 800);
  double Emin = std::min( gr_Ecorr_vs_Euncorr->GetXaxis()->GetXmin(), gr_Ecorr_vs_Euncorr->GetYaxis()->GetXmin() );
  double Emax = std::max( gr_Ecorr_vs_Euncorr->GetXaxis()->GetXmax(), gr_Ecorr_vs_Euncorr->GetYaxis()->GetXmax() );

  // first plot
  c->Clear();
  c->SetGridx(true);
  c->SetGridy(true);

  gr_Ecorr_vs_range->Draw("AL");
  gr_Ecorr_vs_range->GetYaxis()->SetRangeUser( Emin, Emax );
  gr_Ecorr_vs_range->GetXaxis()->SetTitle("#alpha-track length  [mm]");
  gr_Ecorr_vs_range->GetYaxis()->SetTitle("#alpha-particle E_{KIN} in LAB  [MeV]");
  gr_Ecorr_vs_range->SetLineColor(kRed);
  gr_Ecorr_vs_range->SetLineStyle(kSolid);
  gr_Ecorr_vs_range->SetLineWidth(2);
  gr_Euncorr_vs_range->Draw("L SAME");
  gr_Euncorr_vs_range->SetLineColor(kBlue);
  gr_Euncorr_vs_range->SetLineStyle(kSolid);
  gr_Euncorr_vs_range->SetLineWidth(2);
  c->Modified();
  c->Update();
  c->Print("calibration_curve1.root");
  c->Print("calibration_curve1.png");

  // second plot
  c->Clear();
  gr_Ecorr_vs_Euncorr->Draw("AP");
  gr_Ecorr_vs_Euncorr->GetXaxis()->SetRangeUser( Emin, Emax );
  gr_Ecorr_vs_Euncorr->GetYaxis()->SetRangeUser( Emin, Emax );
  gr_Ecorr_vs_Euncorr->Draw("L SAME");
  gr_Ecorr_vs_Euncorr->GetXaxis()->SetTitle("Raw #alpha-particle E_{KIN} in LAB  [MeV]");
  gr_Ecorr_vs_Euncorr->GetYaxis()->SetTitle("Corrected #alpha-particle E_{KIN} in LAB  [MeV]");
  gr_Ecorr_vs_Euncorr->SetLineColor(kRed);
  gr_Ecorr_vs_Euncorr->SetLineStyle(kSolid);
  gr_Ecorr_vs_Euncorr->SetLineWidth(2);
  gPad->Modified();
  gPad->Update();
  
  if(gr_data_Ecorr_vs_Euncorr->GetN()>0) {
    gr_data_Ecorr_vs_Euncorr->SetMarkerColor(kBlack);
    gr_data_Ecorr_vs_Euncorr->SetMarkerSize(1.5);
    //    gr_data_Ecorr_vs_Euncorr->SetMarkerStyle(kCircle);
    //    gr_data_Ecorr_vs_Euncorr->SetMarkerStyle(kFullDotLarge);
    gr_data_Ecorr_vs_Euncorr->SetMarkerStyle(kFullSquare);
    gr_data_Ecorr_vs_Euncorr->Draw("P SAME");
  }
  
  c->Modified();
  c->Update();
  c->Print("calibration_curve2.root");
  c->Print("calibration_curve2.png");

}
