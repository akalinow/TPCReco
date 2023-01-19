#include "AngleProviderPhi.h"
#include "TF1.h"
#include "Math/Math.h"

unsigned int AngleProviderPhi::nInstances=0;

AngleProviderPhi::AngleProviderPhi()
{
    paramVals["polDegree"]=0;
    paramVals["polAngle"]=0;
    std::string fName="AngleProviderPhiTF1_"+std::to_string(nInstances++);
    phiEmissionTF1=std::make_unique<TF1>(
            fName.c_str(),this, &AngleProviderPhi::Phi,
            -ROOT::Math::Pi(),ROOT::Math::Pi(),0);
}

double AngleProviderPhi::Phi(double *x, double *par)
{
    return 1+paramVals["polDegree"]*cos(2*(x[0]-paramVals["polAngle"]));
}

void AngleProviderPhi::ValidateParamValues()
{
    if(paramVals["polDegree"]<0 || paramVals["polDegree"]>1)
        throw std::invalid_argument("AngleProviderPhi::ValidateParamValues: polarisation degree out of [0,1] interval!");
    if(paramVals["polAngle"]<0 || paramVals["polDegree"]>2*ROOT::Math::Pi())
        throw std::invalid_argument("AngleProviderPhi::ValidateParamValues: polarisation angle out of [0,2*pi] interval!");

}

double AngleProviderPhi::GetAngle()
{
    return phiEmissionTF1->GetRandom();
}
