#include "AngleProviderCosIso.h"
#include "TMath.h"

AngleProviderCosIso::AngleProviderCosIso()
    : aCosMin{-1}, aCosMax{1} {}

AngleProviderCosIso::AngleProviderCosIso(double min, double max)
{
    if(min<-1)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: min cos is smaller than -1");
    if(max>1)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: max cos is larger than 1");
    if(min>max)
        throw std::invalid_argument(
                "AngleProviderCosIso::AngleProviderCosIso: min cos is larger than max cos!");

    aCosMin=min;
    aCosMax=max;
}

double AngleProviderCosIso::GetAngle()
{
    auto cos=randGen->Uniform(aCosMin,aCosMax);
    return TMath::ACos(cos);
}



