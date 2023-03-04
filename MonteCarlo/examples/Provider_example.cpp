#include "AngleProvider.h"
#include "XYProvider.h"
#include "EProvider.h"
#include "ZProvider.h"

#include "TCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "Math/Math.h"
#include "TStyle.h"
#include <iostream>
#include <memory>

int main()
{
    /*
        ██     ██  █████  ██████  ███    ██ ██ ███    ██  ██████  ██
        ██     ██ ██   ██ ██   ██ ████   ██ ██ ████   ██ ██       ██
        ██  █  ██ ███████ ██████  ██ ██  ██ ██ ██ ██  ██ ██   ███ ██
        ██ ███ ██ ██   ██ ██   ██ ██  ██ ██ ██ ██  ██ ██ ██    ██
         ███ ███  ██   ██ ██   ██ ██   ████ ██ ██   ████  ██████  ██
        THIS IS NEITHER TYPICAL NOR PROPER USE CASE!!!!
        FOR THE TYPICAL ONE, SEE LAS SECTION OF main()
        Purpose of this code is to present available distribution providers.
     */
    gStyle->SetOptStat(0);
    std::vector<std::unique_ptr<TH1D>> hVec1D;
    std::vector<std::unique_ptr<TH2D>> hVec2D;
    //Generate list of registered providers, and print lists of parameters:
    std::cout<<"List of registered providers with their default parameters:"<<std::endl;
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<Provider>(p);
        prov->PrintParams();
    }

    //Angular distributions (skip all other ones):
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<AngleProvider>(p);
        if(!prov) continue;
        auto h=std::make_unique<TH1D>(p.c_str(),p.c_str(),1000,
                          -ROOT::Math::Pi(),ROOT::Math::Pi());
        TAxis* a = h->GetXaxis();
        a->SetNdivisions(-502);
        a->ChangeLabel(1,-1,-1,-1,-1,-1,"-#pi");
        a->ChangeLabel(-1,-1,-1,-1,-1,-1,"#pi");
        for(int i=0;i<100000;i++)
            h->Fill(prov->GetAngle());
        hVec1D.push_back(std::move(h));
    }

    //XY distributions (skip all other ones):
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<XYProvider>(p);
        if(!prov) continue;
        auto h=std::make_unique<TH2D>(p.c_str(),p.c_str(),
                              200,-20,20,200,-20,20);
        for(int i=0;i<1000000;i++){
            auto r=prov->GetXY();
            h->Fill(r.first,r.second);
        }
        hVec2D.push_back(std::move(h));
    }

    //Z distributions (skip all other ones):
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<ZProvider>(p);
        if(!prov) continue;
        auto h=std::make_unique<TH1D>(p.c_str(),p.c_str(),1000,
                                      -100,100);
        for(int i=0;i<100000;i++)
            h->Fill(prov->GetZ());
        hVec1D.push_back(std::move(h));
    }

    //Energy distributions (skip all other ones):
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<EProvider>(p);
        if(!prov) continue;
        auto h=std::make_unique<TH1D>(p.c_str(),p.c_str(),1000,
                                      5,15);
        for(int i=0;i<100000;i++)
            h->Fill(prov->GetEnergy());
        hVec1D.push_back(std::move(h));
    }

    auto c = new TCanvas("c","",1024,768);
    c->Print("Angle_distr.pdf[");
    //plot histos:
    for(const auto &h : hVec1D)
    {
        h->SetMinimum(0);
        h->Draw();
        c->Print("Angle_distr.pdf");
    }
    for(const auto &h : hVec2D)
    {
        h->Draw("colz");
        c->Print("Angle_distr.pdf");
    }
    c->Print("Angle_distr.pdf]");

    //Typical use case would include obtaining a pointer from the factory
    //while having only a provider name (string). This way, the mechanism is
    //config-friendly, ane providers can be changed without changing the code.
    //Factory method down-casts the pointers to a sub-class selected via its
    // template parameter. This way, we are able to target specific interfaces
    // in the generator - XYProvider will not go to VAngleProvider slot by accident.

    auto p = ProviderFactory::Create<AngleProvider>("AngleProviderE1E2");
    Provider::paramMapType pars = {
            {"sigmaE1", 1},
            {"sigmaE2", 0},
            {"phaseE1E2", 0},
            {"phaseCosSign", 1}
    };
    p->SetParams(pars);
    p->GetAngle();


}