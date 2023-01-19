#include "AngleProviderE1E2.h"
#include "AngleProviderCosIso.h"
#include "AngleProviderIso.h"
#include "AngleProviderSingle.h"
#include "AngleProviderPhi.h"

#include "TCanvas.h"
#include "TH1D.h"
#include "Math/Math.h"
#include "TStyle.h"

int main()
{
    gStyle->SetOptStat(0);
    std::vector<AngleProvider*> vp;
    vp.push_back(new AngleProviderE1E2());
    vp.push_back(new AngleProviderIso());
    vp.push_back(new AngleProviderCosIso());
    vp.push_back(new AngleProviderSingle());
    vp.push_back(new AngleProviderPhi());

    vp[4]->SetSingleParam("polDegree",0.5);
    vp[0]->SetSingleParam("phaseE1E2",ROOT::Math::Pi()/2.);

    vp[0]->SetSingleParam("sigmaE1",1);
    vp[0]->SetSingleParam("sigmaE2",1);
    auto c = new TCanvas("c","",1024,768);
    c->Print("Angle_distr.pdf[");
    for(auto p: vp)
    {
        std::unique_ptr<TH1D> h(new TH1D("h","",1000,
                         -ROOT::Math::Pi(),ROOT::Math::Pi()));
        for(int i=0;i<1000000;i++) {
            h->Fill(p->GetAngle());
        }
        h->SetMinimum(0);
        h->Draw();
        c->Print("Angle_distr.pdf");
    }
    c->Print("Angle_distr.pdf]");
    for(auto p: vp) p->PrintParams();

}