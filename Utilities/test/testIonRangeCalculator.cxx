//////////////////////////
//
// root
// root [0] .L testIonRangeCalculator
// root [1] testIonRangeCalculator("Trees.root", 250.0, 293.15); // p=250 mbar, T=20 C
//
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <cmath>
#include "TFile.h"
#include "TTree.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TSystem.h"
#include "TPad.h"

#include "CommonDefinitions.h"
#include "IonRangeCalculator.h"
#include "HIGS_trees_dataFormat.h"

void test1(IonRangeCalculator *calc, const char *fname, double *average_E_alpha=NULL, double *average_E_carbon=NULL){ // MeV, MeV

  TFile *f = TFile::Open(fname, "OLD");
  if(!f) {
    std::cout << "Cannot open ROOT file: " << fname << std::endl;
    return;
  }
  f->ls();

  auto *Tree_2prong_events = (TTree*)(f->Get("Tree_2prong_events"));

  std::cout << Tree_2prong_events << std::endl;
  if(!Tree_2prong_events) {
    std::cout << "2-prong TTree pointer is empty" << std::endl;
    return;
  }

  auto event2=new Event_2prong;
  Tree_2prong_events->SetBranchAddress("data", &event2);

  double average_T_alpha_LAB=0.0;
  double average_T_carbon_LAB=0.0;
  long nGoodEvents=0;
  for(auto i=0; i<Tree_2prong_events->GetEntries(); i++) {

    Tree_2prong_events->GetEntry(i);

    auto T_alpha_LAB = calc->getIonEnergyMeV(ALPHA, event2->alpha_length);
    auto T_carbon_LAB = calc->getIonEnergyMeV(CARBON_12, event2->carbon_length);
    auto mass_alpha = calc->getIonMassMeV(ALPHA);
    auto mass_carbon = calc->getIonMassMeV(CARBON_12);

    // reject invalid entries (inf, nan)
    if(std::isfinite(event2->alpha_length) && std::isfinite(event2->carbon_length)) {
      nGoodEvents++;
    } else {

      //// DEBUG
      std::cout << "====== Invalid event=" << event2->eventId << " - BEGIN ======" << std::endl;
      std::cout << "Run=" << event2->runId << ", event=" << event2->eventId << ":" << std::endl;
      std::cout << "ALPHA: mass=" << mass_alpha << " MeV/c^2,  range=" <<  event2->alpha_length << " mm,  E_LAB=" << T_alpha_LAB << " MeV" << std::endl;
      std::cout << "C-12:  mass=" << mass_carbon << " MeV/c^2,  range=" <<  event2->carbon_length << " mm,  E_LAB=" << T_carbon_LAB << " MeV" << std::endl;
      std::cout << "alpha START: "; event2->vertexPos.Print();
      std::cout << "alpha END:   "; event2->alpha_endPos.Print();
      std::cout << "alpha LENGTH=" << event2->alpha_length << " mm" << std::endl;
      std::cout << "carbon START: "; event2->vertexPos.Print();
      std::cout << "carbon END:   "; event2->carbon_endPos.Print();
      std::cout << "carbon LENGTH=" << event2->carbon_length << " mm" << std::endl;
      std::cout << "====== Invalid event=" << event2->eventId << " - END ======" << std::endl;
      //// DEBUG

      continue; // skip this event
    }

    average_T_alpha_LAB += T_alpha_LAB;
    average_T_carbon_LAB += T_carbon_LAB;

  }
  if(nGoodEvents>0) {
    average_T_alpha_LAB /= nGoodEvents;
    average_T_carbon_LAB /= nGoodEvents;
  }
  std::cout << "ALPHA: average E_LAB=" << average_T_alpha_LAB << " MeV" << std::endl;
  std::cout << "C-12:  average E_LAB=" << average_T_carbon_LAB << " MeV" << std::endl;

  if(average_E_alpha) *average_E_alpha=average_T_alpha_LAB; // MeV
  if(average_E_carbon) *average_E_carbon=average_T_carbon_LAB; // MeV
}

void test2(IonRangeCalculator *calc, double E_alpha=4.0, double E_carbon=1.0) { // MeV, MeV

  double R_alpha=calc->getIonRangeMM(ALPHA, E_alpha); // mm
  double R_carbon=calc->getIonRangeMM(CARBON_12, E_carbon); // mm

  const double nPointsPerMM=2; // density of TGraph points along the track
  const int nPoints_alpha=(int)(R_alpha*nPointsPerMM+0.5);
  const int nPoints_carbon=(int)(R_carbon*nPointsPerMM+0.5);
  TGraph gr_alpha(calc->getIonBraggCurveMeVPerMM(ALPHA, E_alpha, nPoints_alpha));
  TGraph gr_carbon(calc->getIonBraggCurveMeVPerMM(CARBON_12, E_carbon, nPoints_carbon));

  std::cout << "Current GAS: type=" << calc->getGasMixture() << ", p=" << calc->getGasPressure() << " mbar, T=" << calc->getGasTemperature()-273.15 << " C" << std::endl;
  std::cout << "ALPHA: E_kin=" << E_alpha << " MeV,  range=" << R_alpha << " mm,  dE/dx integral=" << calc->getIonBraggCurveIntegralMeV(ALPHA, E_alpha) << " MeV" << std::endl;
  std::cout << "C-12:  E_kin=" << E_carbon << " MeV,  range=" << R_carbon << " mm,  dE/dx integral=" << calc->getIonBraggCurveIntegralMeV(CARBON_12, E_carbon) << " MeV" << std::endl;

  TMultiGraph mg;
  mg.Add(&gr_alpha, "L*");
  mg.Add(&gr_carbon, "L*");
  gr_alpha.SetLineColor(kRed);
  gr_alpha.SetMarkerColor(kRed);
  gr_carbon.SetLineColor(kBlue);
  gr_carbon.SetMarkerColor(kBlue);
  mg.Draw("A");
  gPad->Print("graph.pdf");
}

void testIonRangeCalculator(const char *fname, double pressure=250.0, double temperature=273.15+20) { // mbar, Kelvins

  if (!gROOT->GetClass("TrackSegment3D")){
     R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
     R__LOAD_LIBRARY(libTPCUtilities.so);
  }

  auto myRangeCalculator=new IonRangeCalculator(CO2, pressure, temperature);
  myRangeCalculator->setDebug(false); // true

  double Ea=0.0, Ec=0.0;
  test1(myRangeCalculator, fname, &Ea, &Ec);
  test2(myRangeCalculator, Ea, Ec);
}
