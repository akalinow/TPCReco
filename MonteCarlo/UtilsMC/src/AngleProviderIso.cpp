#include "AngleProviderIso.h"
#include "Math/Math.h"
#include "stdexcept"

AngleProviderIso::AngleProviderIso()
    :aMin{-ROOT::Math::Pi()}, aMax{ROOT::Math::Pi()}
{}

double AngleProviderIso::GetAngle()
{
    return randGen->Uniform(aMin,aMax);
}

AngleProviderIso::AngleProviderIso(double min, double max)
{
    if(min<-ROOT::Math::Pi())
        throw std::invalid_argument(
            "AngleProviderIso::AngleProviderIso: min angle is smaller than -pi!");
    if(max>ROOT::Math::Pi())
        throw std::invalid_argument(
            "AngleProviderIso::AngleProviderIso: max angle is larger than pi!");
    if(min>max)
        throw std::invalid_argument(
            "min angle is larger than max angle!");
    aMin=min;
    aMax=max;
}
