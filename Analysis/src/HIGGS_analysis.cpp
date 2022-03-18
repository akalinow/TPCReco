#include <vector>
#include <iostream>

#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TVector3.h"


#include "Track3D.h"
#include "HIGGS_analysis.h"

///////////////////////////////
///////////////////////////////
HIGGS_analysis::HIGGS_analysis(){

  bookHistos();
  
}
///////////////////////////////
///////////////////////////////
HIGGS_analysis::~HIGGS_analysis(){

  finalize();
  delete outputFile;
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::bookHistos(){

  std::string outputFileName = "Histos.root";
  outputFile = new TFile(outputFileName.c_str(),"RECREATE");
  
  TH1F *h1 = new TH1F("vertexX","Vertex position; Position [mm]; Event count",100,-150,150);
  histos1D["vertexX"] = h1;
  histos1D["vertexY"] = (TH1F*)h1->Clone("vertexY");
  histos1D["vertexZ"] = (TH1F*)h1->Clone("vertexZ");  
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::fillHistos(Track3D *aTrack){

  TVector3 vertexPos = aTrack->getSegments().front().getStart();
  histos1D["vertexX"]->Fill(vertexPos.X());
  histos1D["vertexY"]->Fill(vertexPos.Y());
  histos1D["vertexZ"]->Fill(vertexPos.Z());

}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::finalize(){

  outputFile->Write();
}
///////////////////////////////
///////////////////////////////
