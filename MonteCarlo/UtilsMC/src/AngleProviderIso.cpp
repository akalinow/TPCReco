#include "AngleProviderIso.h"
#include "Math/Math.h"
#include "stdexcept"


double AngleProviderIso::GetAngle()
{
    return randGen->Uniform(paramVals["minAngle"],paramVals["maxAngle"]);
}

void AngleProviderIso::ValidateParamValues()
{
    auto aMin=paramVals["minAngle"];
    auto aMax=paramVals["maxAngle"];
    if(aMin<-ROOT::Math::Pi())
        throw std::invalid_argument(
                "AngleProviderIso::AngleProviderIso: min angle is smaller than -pi!");
    if(aMax>ROOT::Math::Pi())
        throw std::invalid_argument(
                "AngleProviderIso::AngleProviderIso: max angle is larger than pi!");
    if(aMin>aMax)
        throw std::invalid_argument(
                "AngleProviderIso::AngleProviderIso: min angle is larger than max angle!");
}

AngleProviderIso::AngleProviderIso()
{
    paramVals["minAngle"]=-ROOT::Math::Pi();
    paramVals["maxAngle"]=ROOT::Math::Pi();
}
