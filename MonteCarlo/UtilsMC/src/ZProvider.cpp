#include "ZProvider.h"

ZProviderUniform::ZProviderUniform()
{
    paramVals["minZ"]=-100;
    paramVals["maxZ"]=100;
}

double ZProviderUniform::GetZ()
{
    return randGen->Uniform(paramVals["minZ"],paramVals["maxZ"]);
}

void ZProviderUniform::ValidateParamValues()
{
    if(paramVals["maxZ"]<paramVals["minZ"] )
        throw std::invalid_argument("ZProviderUniform::ValidateParamValues: maxZ cannot be smaller than minZ!");
}

ZProviderSingle::ZProviderSingle()
{
    paramVals["singleZ"]=0;
}

double ZProviderSingle::GetZ()
{
    return paramVals["singleZ"];
}
