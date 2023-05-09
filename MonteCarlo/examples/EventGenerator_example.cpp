
#include "TPCReco/EventGenerator.h"
#include "Math/EulerAngles.h"
#include "Math/LorentzRotation.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TFile.h"


void generateEventsAndMakePlots(EventGenerator &g, int nEvents) {
    gRandom->SetSeed(0);
    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(999);
    //definition of plots:
    TH2D *hXY = new TH2D("hxy", ";y [mm];z [mm]", 200, 0, 0, 200, 0, 0);
    auto hCosAngleTwoTracks = new TH1D("hdot", "cos angle between tracks;cos(#alpha)", 1000, 0, 0);
    auto hAngleTwoTracks = new TH1D("hdotdeglab", "angle between tracks LAB [deg];#alpha", 1000, 0, 0);
    auto hAngleTwoTracksCM = new TH1D("hdotdegcm", "angle between tracks CM [deg];#alpha", 1000, 170, 190);

    auto hThetaLab = new TH1D("thetalab", "cos theta LAB;cos(#theta)", 1000, 0, 0);
    auto hThetaCM = new TH1D("thetacm", "cos theta CM;cos(#theta)", 1000, 0, 0);
    auto hMomLAB = new TH1D("momlab", "alpha momentum in LAB;p [MeV/#it{c}]", 1000, 0, 0);
    auto hMomCM = new TH1D("momcm", "alpha momentum in CM;p [MeV/#it{c}]", 1000, 0, 0);
    auto hAlphaTheta = new TH2D("AlphaTheta", "angle between tracks vs cos theta;cos(#theta);#alpha", 300, 0, 0, 300, 0,
                                0);
    TH2D *hThetaP = new TH2D("thetap", "LAB;cos(#theta);p_{#alpha} [MeV/#it{c}]", 200, 0, 0, 200, 0, 0);
    TH2D *hThetaPcm = new TH2D("thetapcm", "CM;cos(#theta);p_{#alpha} [MeV/#it{c}]", 200, 0, 0, 200, 0, 0);

    auto hTheta3P = new TH1D("theta3p", "cos theta LAB, 3 prong, all prongs;cos(#theta)", 1000, 0, 0);

    auto hInvMass3P = new TH1D("invmass3p", "invariant mass, alpha pairs, 3-prong;m_{inv} [MeV]", 1000, 0, 0);

    auto hKineticE = new TH1D("kineticE", "kinetic energy;E_{k} [MeV]", 1000, 0, 0);
    auto hKineticEinvMass = new TH2D("kineticEinvMass", "inv mass vs kinetic energy of third alpha;E_{K} [MeV];m_{inv} [MeV]", 1000, 0, 0,1000,0,0);

    for (int i = 0; i < nEvents; i++) {
        auto e = g.GenerateEvent();
        if (i % 100000 == 0)
            std::cout << "Generated event " << i / 1000 << "k" << std::endl;
        //we analyze only C12_ALPHA reactions
        auto rt=e.GetReactionType();
        hXY->Fill(e.GetTrueVertexPosition().Y(), e.GetTrueVertexPosition().Z());
        auto tracks = e.GetTracks();
        if (tracks.empty()) continue; //skip empty events

        if (rt == reaction_type::C12_ALPHA) {

            //only two-prong, so we have two tracks in each event
            auto p41 = tracks[0].GetPrimaryParticle().GetFourMomentum();
            auto p42 = tracks[1].GetPrimaryParticle().GetFourMomentum();
            auto v1 = p41.Vect();
            auto v2 = p42.Vect();

            auto beta = 1 / ((p41 + p42).M()) * (v1 + v2);

            hMomLAB->Fill(p41.P());

            hThetaP->Fill(-p41.X() / p41.P(), p41.P());
            p41.Boost(-beta);
            p42.Boost(-beta);
            hThetaPcm->Fill(-p41.X() / p41.P(), p41.P());

            auto cosalpha = v1.Dot(v2) / (v1.Mag() * v2.Mag());

            hCosAngleTwoTracks->Fill(cosalpha);
            hAngleTwoTracks->Fill(TMath::ACos(cosalpha) * 180. / ROOT::Math::Pi());
            hThetaLab->Fill(-v1.X() / v1.Mag());
            hThetaCM->Fill(-p41.X() / p41.Vect().Mag());
            hAlphaTheta->Fill(-v1.X() / v1.Mag(), TMath::ACos(cosalpha) * 180. / ROOT::Math::Pi());

            cosalpha = p41.Vect().Dot(p42.Vect()) / (p41.P() * p42.P());
            hAngleTwoTracksCM->Fill(TMath::ACos(cosalpha) * 180. / ROOT::Math::Pi());
            hMomCM->Fill(p41.P());
        }
        else if(rt==reaction_type::THREE_ALPHA_DEMOCRATIC)
        {
            auto fourMom=tracks[0].GetPrimaryParticle().GetFourMomentum();
            hTheta3P->Fill(-fourMom.X()/fourMom.P());
            for(auto &t: tracks){
                auto fourMom=t.GetPrimaryParticle().GetFourMomentum();
                hMomLAB->Fill(fourMom.P());
                hTheta3P->Fill(-fourMom.X()/fourMom.P());
            }

            auto p1=tracks[0].GetPrimaryParticle().GetFourMomentum();
            auto p2=tracks[1].GetPrimaryParticle().GetFourMomentum();
            auto p3=tracks[2].GetPrimaryParticle().GetFourMomentum();

            hInvMass3P->Fill((p1+p2).M());
            hInvMass3P->Fill((p2+p3).M());
            //std::cout<<p1.Mag()<<" "<<p2.Mag()<<" "<<p3.Mag()<<" "<<std::endl;
            hInvMass3P->Fill((p3+p1).M());

            hKineticEinvMass->Fill(p1.E()-p1.M(),(p2+p3).M());
            hKineticEinvMass->Fill(p2.E()-p2.M(),(p1+p3).M());
            hKineticEinvMass->Fill(p3.E()-p3.M(),(p1+p2).M());

        }

        for(auto &t: tracks){
            auto fourMom=t.GetPrimaryParticle().GetFourMomentum();
            hKineticE->Fill(fourMom.E()-fourMom.M());
        }

    }


    auto c = new TCanvas("c", "", 1024, 768);

    c->Print("plots.pdf[");

    hXY->Draw("colz");
    c->Print("plots.pdf");

    hCosAngleTwoTracks->Draw();
    c->Print("plots.pdf");

    hAngleTwoTracks->Draw();
    c->Print("plots.pdf");

    hAngleTwoTracksCM->Draw();
    c->Print("plots.pdf");

    hThetaLab->Draw();
    c->Print("plots.pdf");

    hThetaCM->Draw();
    c->Print("plots.pdf");

    hAlphaTheta->Draw("colz");
    c->Print("plots.pdf");

    hMomLAB->Draw();
    c->Print("plots.pdf");

    hMomCM->Draw();
    c->Print("plots.pdf");

    hThetaP->Draw("colz");
    c->Print("plots.pdf");

    hThetaPcm->Draw("colz");
    c->Print("plots.pdf");

    hTheta3P->Draw();
    c->Print("plots.pdf");

    hInvMass3P->Draw();
    c->Print("plots.pdf");

    hKineticE->Draw();
    c->Print("plots.pdf");

    hKineticEinvMass->Draw("colz");
    c->Print("plots.pdf");


    c->Print("plots.pdf]");


}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "usage: " + std::string(argv[0]) + " <config file> <number of events to generate>\n";
        return -1;
    }
    auto nEvents = std::atoi(argv[2]);

    auto g = EventGenerator(argv[1]);
    generateEventsAndMakePlots(g, nEvents);


    return 0;
}