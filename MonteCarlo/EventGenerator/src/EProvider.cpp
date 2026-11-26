#include <cmath>
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

EProviderGausBW::EProviderGausBW() {
    paramVals["meanE"] = 13.06;
    paramVals["sigmaE"] = 0.2;
    paramVals["resonanceE"] = 13.096;
    paramVals["resonanceWidth"] = 0.130;
}

double EProviderGausBW::GetEnergy() {
    const double halfWidth2 = 0.25*paramVals["resonanceWidth"]*paramVals["resonanceWidth"];
    for(;;){ // acceptance ~ resonanceWidth/sigmaE: ~0.3 % for a 0.6 keV wide resonance in a 0.2 MeV beam
        double e = randGen->Gaus(paramVals["meanE"], paramVals["sigmaE"]);
        if(e < 0) continue;
        double de = e - paramVals["resonanceE"];
        if(randGen->Uniform() < halfWidth2/(de*de + halfWidth2)) return e;
    }
}

void EProviderGausBW::ValidateParamValues() {
    CheckCondition(paramVals["sigmaE"] > 0, "sigma must be larger than 0!");
    CheckCondition(paramVals["meanE"] >= 0, "mean cannot be smaller than 0!");
    CheckCondition(paramVals["resonanceE"] > 0, "resonance energy must be larger than 0!");
    CheckCondition(paramVals["resonanceWidth"] > 0, "resonance width must be larger than 0!");
    CheckCondition(std::abs(paramVals["resonanceE"] - paramVals["meanE"]) < 5*paramVals["sigmaE"],
                   "resonance more than 5 sigma away from the beam energy!");
}
