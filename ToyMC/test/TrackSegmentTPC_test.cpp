#include <iostream>
#include <thread>
#include <chrono>

#include <TApplication.h>
#include <TH2D.h>
#include <TVector2.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLine.h>
#include <TStyle.h>
#include <TColor.h>
#include <TROOT.h>

#include "TrackSegmentTPC.h"

#define ERROR -1

int main(int argc, char *argv[]) {

#if defined (__CINT__) || defined(__MAKECINT__)
    std::cout << "Calling gROOT->Reset" << std::endl << std::flush;
    gROOT->Reset();
#endif

  TApplication app ("app",&argc,argv); // allows to open graphics windows
  /*
  if(argc<2) {
    std::cout << "Usage: \n\n"
	      << argv[0] << " TPC_geometry_file\n\n";
    return ERROR;
  }
  const std::string geom_fname(argv[1]);

  std::cout << "main: argc=" << argc << std::endl;
  for(int i=0; i<argc; i++) {
    std::cout << "main: argv[" << i << "]=" << argv[i] << std::endl << std::flush;
  }
  */
  const std::string geom_fname("resources/geometry_mini_eTPC.dat");
  
  // generate 2D pseudo-data X-Z [mm]x[mm]

  const unsigned int NbinsX = 50;
  const unsigned int NbinsZ = 50;
  const double xmin = -100.0;  // mm
  const double xmax =  100.0;  // mm
  const double zmin = -100.0; // mm
  const double zmax =  100.0; // mm  
  auto *h2 = new TH2D("h_xz", "Pseudo data;X [mm];Z [mm];Charge/bin [arb.u.]",
		      NbinsX, xmin, xmax,
		      NbinsZ, zmin, zmax);

  const double length = 60.0;  // mm  - nominal track lenght
  const double theta  = 30.0;  // deg - angle wrt X-axis
  const double spread = 3.0;   // mm  - gaussian spread around track segment

  const double Deg2Rad = pi/180.;
  const TVector2 vec=length*TVector2( TMath::Cos(theta*Deg2Rad), TMath::Sin(theta*Deg2Rad) );

  const double trk_x1 = xmin+30.0;        // mm
  const double trk_z1 = zmin+50.0;        // mm
  const double trk_x2 = trk_x1 + vec.X(); // mm
  const double trk_z2 = trk_z1 + vec.Y(); // mm
  
  // generating pseudo-hits
  const unsigned long Npoints = 10000;
  auto *rnd = new TRandom3(0);

  unsigned long cnt=0;
  for(unsigned long i=0; i<Npoints; i++) {

    TVector2 pos = TVector2(trk_x1 + rnd->Gaus(0., spread), trk_z1 + rnd->Gaus(0., spread))
      + rnd->Uniform(0., 1.)*vec;
    h2->Fill( pos.X(), pos.Y(), 1.0 );

    if( pos.X()>=xmin && pos.X()<xmax && pos.Y()>=zmin && pos.Y()<zmax ) cnt++;
  }

  // setting initial start/end points of 2D track segment candidate
  double fit_x1 = trk_x1+rnd->Uniform(-5*spread, 5*spread);
  double fit_z1 = trk_z1+rnd->Uniform(-5*spread, 5*spread);
  double fit_x2 = trk_x2+rnd->Uniform(-5*spread, 5*spread);
  double fit_z2 = trk_z2+rnd->Uniform(-5*spread, 5*spread);
  unsigned int fit_n = 5;
  
  // initialize TPC geometry
  auto *g = new GeometryTPC(geom_fname.c_str());
  if(!g) {
    std::cerr << "ERROR: Can't initialize TPC geometry!!!\n";
    return ERROR;
  }
  g->SetDebug(false);

  // initialize 2D track segment candidate
  auto *t = new TrackSegment2D( fit_x1, fit_z1, fit_x2, fit_z2, fit_n );
  if(!t) {
    std::cerr << "ERROR: Can't initialize 2D track segment!!!\n";
    return ERROR;
  }
  t->SetDebug(true);

  std::cout << "Cluster TH2D integral: " << h2->Integral() << "\n"
	    << "Cluster TH2D entries (all): " << h2->GetEntries() << "\n"
  	    << "Cluster TH2D entries (within limits): " << cnt << "\n\n";

  // test getter methods
  std::cout << "TRK candidate w/o CLUSTER data:\n";
  t->Print();

  t->SetCluster(h2);

  std::cout << "\n" << "TRK candidate with CLUSTER data:\n";
  t->Print();
  
  std::cout << "\n" << "TRK candidate to CLUSTER matching:\n";
  std::cout << "Method-0 CHI2: " << t->GetChi2(spread, 0) << "\n";
  std::cout << "Method-1 CHI2: " << t->GetChi2(spread, 1) << "\n";
  std::cout << "Method-2 CHI2: " << t->GetChi2(spread, 2) << "\n\n";

  const double ratio=(zmax-zmin)/(xmax-xmin);
  const int sx = (int)((float)500*(1+0.1+0.2));
  const int sy = (int)((float)500*ratio*(1+0.1+0.05));
  std::cout << "sx: "<<sx << " sy: " << sy << std::endl;
  std::cout<<"h2: "<<h2<<std::endl;

  TCanvas *c1 = new TCanvas("c1","c", 2, 78, sx, sy);
  h2->Draw("COLZ");
 
  // superimpose track segment candidate 
  TLine *seg=new TLine( t->GetStartPoint().X(), t->GetStartPoint().Y(),
		       t->GetEndPoint().X(), t->GetEndPoint().Y() );
  seg->SetLineWidth(3);
  seg->SetLineColor(kRed);
  seg->Draw();

  c1->Print("track.pdf");

  std::cout<<"Here"<<std::endl;
  return 0;
}
