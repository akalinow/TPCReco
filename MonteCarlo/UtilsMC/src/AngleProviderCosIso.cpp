#include "AngleProviderCosIso.h"
#include "TMath.h"

AngleProviderCosIso::AngleProviderCosIso()
{
    paramVals["minCos"]=-1;
    paramVals["maxCos"]=1;
}

double AngleProviderCosIso::GetAngle()
{
    auto aCosMin=paramVals["minCos"];
    auto aCosMax=paramVals["maxCos"];
    auto cos=randGen->Uniform(aCosMin,aCosMax);
    return TMath::ACos(cos);
}

void AngleProviderCosIso::ValidateParamValues()
{
    auto min=paramVals["cosMin"];
    auto max=paramVals["cosMAx"];
    if(min<-1)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: min cos is smaller than -1");
    if(max>1)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: max cos is larger than 1");
    if(min>max)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: min cos is larger than max cos!");
}



