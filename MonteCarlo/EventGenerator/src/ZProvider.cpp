#include "TPCReco/ZProvider.h"

ZProviderUniform::ZProviderUniform() {
    paramVals["minZ"] = -100;
    paramVals["maxZ"] = 100;
}

double ZProviderUniform::GetZ() {
    return randGen->Uniform(paramVals["minZ"], paramVals["maxZ"]);
}

void ZProviderUniform::ValidateParamValues() {
    CheckCondition(paramVals["maxZ"] >= paramVals["minZ"], "maxZ cannot be smaller than minZ!");
}

ZProviderSingle::ZProviderSingle() {
    paramVals["singleZ"] = 0;
}

double ZProviderSingle::GetZ() {
    return paramVals["singleZ"];
}
