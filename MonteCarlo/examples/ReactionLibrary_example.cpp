
#include "EventGenerator.h"
#include "Math/EulerAngles.h"
#include "Math/LorentzRotation.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"


void generateEventsAndMakePlots(int nEvents, const std::string& configFileName){
    gRandom->SetSeed(0);
    gStyle->SetOptStat(0);
    //definition of plots:
    TH2D* hXY=new TH2D("hxy", "XY",200,0,0,200,0,0);
    auto hDot=new TH1D("hdot", "cos angle between tracks",1000,0,0);
    auto hDotDeg=new TH1D("hdotdeglab", "angle between tracks LAB [deg]",1000,0,0);
    auto hDotDegCM=new TH1D("hdotdegcm", "angle between tracks CM [deg]",1000,170,190);

    auto hThetaLab=new TH1D("thetalab", "cos theta LAB",1000,0,0);
    auto hThetaCM=new TH1D("thetacm", "cos theta CM",1000,0,0);
    auto hAlphaTheta=new TH2D("AlphaTheta", "angle between tracks vs cos theta",300,0,0,300,0,0);

    auto g = EventGenerator(configFileName);

    for(int i=0;i<nEvents;i++) {
        auto e = g.GenerateEvent();
        if (i % 1000 == 0)
            std::cout << "Generated event " << i << std::endl;
        hXY->Fill(e.GetVertexPosition().Y(), e.GetVertexPosition().Z());
        auto tracks=e.GetTracks();
        if(tracks.empty()) continue; //skip empty events
        auto p41=tracks[0].GetPrimaryParticle().GetFourMomentum();
        auto p42=tracks[1].GetPrimaryParticle().GetFourMomentum();
        auto v1=p41.Vect();
        auto v2=p42.Vect();

        auto beta=1/((p41+p42).M())*(v1+v2);

        p41.Boost(-beta);
        p42.Boost(-beta);

        auto cosalpha=v1.Dot(v2)/(v1.Mag()*v2.Mag());
        hDot->Fill(cosalpha);
        hDotDeg->Fill(TMath::ACos(cosalpha)*180./ROOT::Math::Pi());
        hThetaLab->Fill(-v1.X()/v1.Mag());
        hThetaCM->Fill(-p41.X()/p41.Vect().Mag());
        hAlphaTheta->Fill(-v1.X()/v1.Mag(),TMath::ACos(cosalpha)*180./ROOT::Math::Pi());

        cosalpha=p41.Vect().Dot(p42.Vect())/(p41.Vect().Mag()*p42.Vect().Mag());
        hDotDegCM->Fill(TMath::ACos(cosalpha)*180./ROOT::Math::Pi());
        }



    auto c = new TCanvas("c","",1024,768);

    c->Print("plots.pdf[");

    hXY->Draw("colz");
    c->Print("plots.pdf");

    hDot->Draw();
    c->Print("plots.pdf");

    hDotDeg->Draw();
    c->Print("plots.pdf");

    hDotDegCM->Draw();
    c->Print("plots.pdf");

    hThetaLab->Draw();
    c->Print("plots.pdf");

    hThetaCM->Draw();
    c->Print("plots.pdf");

    hAlphaTheta->Draw("colz");
    c->Print("plots.pdf");


    c->Print("plots.pdf]");

}

int main(int argc, char **argv) {
    if(argc<3){
        std::cout<<"usage: "+std::string(argv[0])+" <config file> <number of events to generate>\n";
        return -1;
    }
    auto nEvents=std::atoi(argv[2]);

    std::string rootFileName("myfile.root");

    generateEventsAndMakePlots(nEvents,argv[1]);


    return 0;
}