#include "AngleProvider.h"

#include "TCanvas.h"
#include "TH1D.h"
#include "Math/Math.h"
#include "TStyle.h"
#include <iostream>
#include <memory>

int main()
{

    //I divided everything into two for loops to get rid of ugly root log messages from output with provider parameters
    //Generate list of registered providers, at the same time plot sample angular distributions
    std::vector<std::unique_ptr<TH1D>> hVec;
    std::cout<<"List of registered angle providers with their default parameters:"<<std::endl;
    for(const auto& p: ProviderFactory::GetRegiseredIdentifiers()){
        auto prov=ProviderFactory::Create<AngleProvider>(p);
        prov->PrintParams();
        if(!prov) continue;
        auto h=std::make_unique<TH1D>(p.c_str(),p.c_str(),1000,
                          -ROOT::Math::Pi(),ROOT::Math::Pi());
        for(int i=0;i<100000;i++)
            h->Fill(prov->GetAngle());
        hVec.push_back(std::move(h));
    }

    auto c = new TCanvas("c","",1024,768);
    c->Print("Angle_distr.pdf[");
    //plot histos:
    for(const auto &h : hVec)
    {
        h->SetMinimum(0);
        h->Draw();
        c->Print("Angle_distr.pdf");
    }
    c->Print("Angle_distr.pdf]");

}