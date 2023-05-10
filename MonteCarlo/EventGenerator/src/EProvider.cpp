#include "TPCReco/EProvider.h"

EProviderSingle::EProviderSingle() {
    paramVals["singleE"] = 11.5;
}

double EProviderSingle::GetEnergy() {
    return paramVals["singleE"];
}

void EProviderSingle::ValidateParamValues() {
    CheckCondition(paramVals["singleE"] >= 0, "energy cannot be smaller than 0!");
}

EProviderGaus::EProviderGaus() {
    paramVals["meanE"] = 11.5;
    paramVals["sigmaE"] = 0.3;
}

double EProviderGaus::GetEnergy() {
    auto r = randGen->Gaus(paramVals["meanE"], paramVals["sigmaE"]);
    if (r < 0) r = 0;
    return r;
}

void EProviderGaus::ValidateParamValues() {
    CheckCondition(paramVals["sigmaE"] >= 0, "sigma cannot be smaller than 0!");
    CheckCondition(paramVals["meanE"] >= 0, "mean cannot be smaller than 0!");
}
