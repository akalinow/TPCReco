#include "EProvider.h"

EProviderSingle::EProviderSingle()
{
    paramVals["singleE"]=11.5;
}

double EProviderSingle::GetEnergy()
{
    return paramVals["singleE"];
}

void EProviderSingle::ValidateParamValues()
{
    if(paramVals["singleE"]<0 )
        throw std::invalid_argument("EProviderSingle::ValidateParamValues: energy cannot be smaller than 0!");
}

EProviderGaus::EProviderGaus()
{
    paramVals["meanE"]=11.5;
    paramVals["sigmaE"]=0.3;
}

double EProviderGaus::GetEnergy()
{
    auto r=randGen->Gaus(paramVals["meanE"],paramVals["sigmaE"]);
    if(r<0) r=0;
    return r;
}

void EProviderGaus::ValidateParamValues()
{
    if(paramVals["sigmaE"]<0 )
        throw std::invalid_argument("EProviderGaus::ValidateParamValues: sigma cannot be smaller than 0!");
    if(paramVals["meanE"]<0 )
        throw std::invalid_argument("EProviderGaus::ValidateParamValues: mean cannot be smaller than 0!");
}
