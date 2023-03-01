#include "SimEvent.h"
#include "TTree.h"
#include "TFile.h"
#include "TCanvas.h"
#include <iostream>
#include "TH1D.h"
#include "TH3F.h"
#include "TH2F.h"
#include <string>
#include "TString.h"
#include "TStyle.h"
#include "TLegend.h"

#include "IonRangeCalculator.h"


int main(int argc, char** argv)
{
    gStyle->SetOptStat(0);
    gStyle->SetNumberContours(999);
    int nEventsToDraw=0;
    std::string fname;
    if(argc<2)
    {
        std::cout<<"Usage: "<<argv[0]<<" <filename> <number of events to draw> (optional)"<<std::endl;
        std::cout<<"If <number of events to draw> is not provided all events in file will be drawn"<<std::endl;
        return 1;
    }
    else if(argc>=2)
    {
        fname=argv[1];
    }


    TFile *f=new TFile(fname.c_str());
    TTree *t;
    f->GetObject("TPCData",t);
    if(argc>2)
        nEventsToDraw=atoi(argv[2]);
    else
        nEventsToDraw=t->GetEntries();

    SimEvent* event=new SimEvent();
    t->SetBranchAddress("SimEvent",&event);
    int nEntries=t->GetEntries();
    TCanvas *c=new TCanvas("c","",1024,768);
    c->Print("bragg.pdf[");
    double braggUpperRange=60;
    double depXYrange = 0.2;
    TH1D* hall=new TH1D("hall","A",200,0,braggUpperRange);
    auto hXZ = new TH2D("hXZ","energy deposit, XZ-plane;x[mm];z[mm]",200,-300,300,200,-depXYrange,depXYrange);
    auto hYZ = new TH2D("hYZ","energy deposit, YZ-plane;y[mm];z[mm]",200,-depXYrange,depXYrange,200,-depXYrange,depXYrange);
    auto hXY = new TH2D("hXY","energy deposit, XY-plane;x[mm];y[mm]",200,-300,300,200,-depXYrange,depXYrange);
    auto hRange = new TH1D("hRange","Range of particles;Range [mm]",200,0,braggUpperRange);
    for(int i=0;i<t->GetEntries();i++)
    {
        if(i==nEntries)
            break;
        t->GetEntry(i);
        for(const auto& track: event->GetTracks())
        {
            int a=track.GetPrimaryParticle().GetA();
            int z=track.GetPrimaryParticle().GetZ();
            double e=track.GetPrimaryParticle().GetKineticEnergy();
            //double range=track.GetLength();
            //TH1D* h=new TH1D("h",Form("E=%.2fMeV A=%d Z=%d event=%d",e,a,z,i),100,0,1.2*range);
            if(i<nEventsToDraw);
                TH1D* h=new TH1D("h",Form("E=%.2fMeV A=%d Z=%d event=%d",e,a,z,i),100,0,braggUpperRange);
            auto start=track.GetStart();
            hRange->Fill(track.GetRange());
            for(const auto& hit : track.GetHits())
            {
                auto pos=hit.GetPosition();
                auto d=pos-start;
                auto eDep=hit.GetEnergy();
                hXZ->Fill(pos.X(),pos.Z(),eDep);
                hYZ->Fill(pos.Y(),pos.Z(),eDep);
                hXY->Fill(pos.X(),pos.Y(),eDep);
                if(i<nEventsToDraw) {
                    h->Fill(d.Mag(),  eDep);
                }
                hall->Fill(d.Mag(),  eDep);
            }
            if(i<nEventsToDraw) {
                h->Scale(1 / h->GetBinWidth(1));
                h->GetXaxis()->SetTitle("Distance [mm]");
                h->GetYaxis()->SetTitle("Energy deposit [MeV/mm]");
                h->Draw("hist");
                c->Print("bragg.pdf");
            }
            delete h;
        }

        /*
        //alternative using iterators instead of range based for loops (C++11) and no auto:
        for(SimTracksIterator trackIter=event->TracksBegin(); trackIter!=event->TracksEnd();trackIter++)
        {
            int a=trackIter->GetA();
            int z=trackIter->GetZ();
            double e=trackIter->GetEnergy();
            double range=trackIter->GetLength();
            TH1D* h=new TH1D("h",Form("E=%.2fMeV A=%d Z=%d event=%d",e,a,z,i),100,0,1.2*range);
            TVector3 start=trackIter->GetStart();
            for(SimHitsIterator hitIter=trackIter->HitsBegin(); hitIter!=trackIter->HitsEnd();hitIter++)
            {
                TVector3 pos=hitIter->GetPosition();
                TVector3 d=pos-start;
                h->Fill(d.Mag(),hitIter->GetEnergy());
            }
            h->Draw("hist");
            c->Print("bragg.pdf");
            delete h;
        }
        */

    }

    auto rCal = std::make_unique<IonRangeCalculator>();
    auto g = rCal->getIonBraggCurveMeVPerMM(pid_type::ALPHA,3,1000);

    c->SetGridx();
    c->SetGridy();

    hall->Scale(1./t->GetEntries());
    std::cout<<"Total deposit: "<<hall->Integral()<<" "<<rCal->getIonBraggCurveIntegralMeV(pid_type::ALPHA, 3.0,1000)<<std::endl;
    hall->Scale(1/hall->GetBinWidth(1));
    hall->GetXaxis()->SetTitle("Distance [mm]");
    hall->GetYaxis()->SetTitle("Energy deposit [MeV/mm]");
    hall->SetFillColorAlpha(kBlue-6,0.4);
    hall->SetTitle("E_{k} = 3 MeV, p = 250 mbar");
    hall->Draw("hist");
    g.SetLineColor(kRed);
    g.SetLineWidth(2);
    g.Draw("SAME L");

    auto l = new TLegend(0.7,0.7,0.9,0.9);
    l->AddEntry(&g,"SRIM","l");
    l->AddEntry(hall,"Geant4","f");
    l->Draw();


    c->Print("bragg.pdf");
    g.Draw("AL");
    c->Print("bragg.pdf");
    hXY->Draw("colz");
    c->Print("bragg.pdf");
    hXZ->Draw("colz");
    c->Print("bragg.pdf");
    hYZ->Draw("colz");
    c->Print("bragg.pdf");
    hRange->Draw();
    c->Print("bragg.pdf");


    c->Print("bragg.pdf");

    c->Print("bragg.pdf]");
}
