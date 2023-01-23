#include "XYProvider.h"
#include "TF2.h"

XYProviderGaussTail::XYProviderGaussTail() {
    paramVals["meanX"] = 0;
    paramVals["meanY"] = 0;
    paramVals["sigma"] = 1;
    paramVals["flatR"] = 5;
    std::string fName = "AngleProviderPhiTF1_" + std::to_string(nInstances++);
    profileTF2 = std::make_unique<TF2>(
            fName.c_str(), this, &XYProviderGaussTail::Profile,
            -100, 100, -100, 100, 0);
    profileTF2->SetNpx(1000);
    profileTF2->SetNpy(1000);
}

XYProvider::xyVal XYProviderGaussTail::GetXY() {
    double x, y;
    profileTF2->GetRandom2(x, y);
    return {x, y};
}

double XYProviderGaussTail::Profile(double *x, double *par) {
    auto mx = paramVals["meanX"];
    auto my = paramVals["meanY"];
    auto fR = paramVals["flatR"];
    auto s = paramVals["sigma"];
    auto r = sqrt((x[0] - mx) * (x[0] - mx) + (x[1] - my) * (x[1] - my));
    if (r < fR) return 1;
    return exp(-0.5 * (r - fR) * (r - fR) / s / s);
}

void XYProviderGaussTail::ValidateParamValues() {
    CheckCondition(paramVals["sigma"] >= 0, "sigma cannot be smaller than 0!");
    CheckCondition(paramVals["flatR"] >= 0, "radius cannot be smaller than 0!");
}

XYProviderSingle::XYProviderSingle() {
    paramVals["singleX"] = 0;
    paramVals["singleY"] = 0;
}

XYProvider::xyVal XYProviderSingle::GetXY() {
    return {paramVals["singleX"], paramVals["singleY"]};
}
