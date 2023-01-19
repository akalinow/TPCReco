#include "AngleProviderE1E2.h"
#include "Math/SpecFuncMathMore.h"
#include "TF1.h"
#include "Math/Math.h"

unsigned int AngleProviderE1E2::nInstances=0;

AngleProviderE1E2::AngleProviderE1E2()
{
    paramVals["norm"]=1;
    paramVals["sigmaE1"]=1;
    paramVals["sigmaE2"]=0;
    paramVals["phaseE1E2"]=0;
    std::string fName="AngleProviderE1E2TF1_"+std::to_string(nInstances++);
    thetaEmissionTF1=std::make_unique<TF1>(
            fName.c_str(),this,&AngleProviderE1E2::thetaEmission,
            0.,ROOT::Math::Pi(),0);
}

// 1D angular distribution to be used with TF1::GetRandom
// Non-siotropic angular distrubution of alpha-particle emission angle wrt gamma beam
// with E1 abd E2 components for Oxygen-16 photodisintegration reaction.
// Ref: M.Assuncao et al., PRC 73 055801 (2006).
//
// x[0] = CMS theta_BEAM alpha-particle emission angle wrt gamma beam [rad]
// par[0] = normalisation constant
// par[1] = sigma_E1 - E1 cross section parameter [nb]
// par[2] = sigma_E2 - E2 cross section parameter [nb]
// par[3] = phase_12 - phase angle responsible for mixing E1/E2 [rad]
double AngleProviderE1E2::thetaEmission(double *x, double *par)
{
    auto norm = paramVals["norm"];
    auto s1=paramVals["sigmaE1"];
    auto s2=paramVals["sigmaE1"];
    auto ph12=paramVals["phaseE1E2"];
    double c = cos(x[0]);
    double L[5];
    for(auto i=0; i<=4; i++) L[i]=ROOT::Math::legendre(i, c);
    auto WE1 = L[0] - L[2];
    auto WE2 = L[0] + (5./7.)*L[2] - (12./7.)*L[4];
    auto W12 = 6./sqrt(5.0)*( L[1] - L[3] );
    return norm*( s1*WE1 + s2*WE2 + sqrt(s1*s2)*cos(ph12)*W12 );

}

void AngleProviderE1E2::ValidateParamValues()
{
    if(paramVals["sigmaE1"]<0)
        throw std::invalid_argument("AngleProviderE1E2::ValidateParamValues: sigmaE1 is smaller than 0!");
    if(paramVals["sigmaE2"]<0)
        throw std::invalid_argument("AngleProviderE1E2::ValidateParamValues: sigmaE2 is smaller than 0!");
}

double AngleProviderE1E2::GetAngle()
{
    return thetaEmissionTF1->GetRandom();
}
