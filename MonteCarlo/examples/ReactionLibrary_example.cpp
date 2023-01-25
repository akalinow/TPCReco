#include "ReactionLibrary.h"
#include "TwoProngReaction.h"
#include "AngleProvider.h"
#include "GeneratorSetup.h"
#include "EventGenerator.h"
#include "ObjectFactory.h"
#include "Math/EulerAngles.h"
#include "Math/LorentzRotation.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"


void generateEventsAndMakePlots(int nEvents, const std::string& configFileName){

    //definition of plots:
    TH2D* hXY=new TH2D("hxy", "",1000,-20,20,1000,-20,20);
    auto hDot=new TH1D("hdot", "",1000,-1.1,1.1);

    auto g = EventGenerator(configFileName);

    for(int i=0;i<nEvents;i++) {
        auto e = g.GenerateEvent();
        if (i % 1000 == 0)
            std::cout << "Generated event " << i << std::endl;
        hXY->Fill(e.GetVertexPosition().Y(), e.GetVertexPosition().Z());
        auto tracks=e.GetTracks();
        auto v1=tracks[0].GetPrimaryParticle().GetFourMomentum().Vect();
        auto v2=tracks[1].GetPrimaryParticle().GetFourMomentum().Vect();
        hDot->Fill(v1.Dot(v2)/(v1.Mag()*v2.Mag()));
        }



    auto c = new TCanvas("c","",1024,768);

    c->Print("plots.pdf[");

    hXY->Draw("colz");
    c->Print("plots.pdf");

    hDot->Draw();
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