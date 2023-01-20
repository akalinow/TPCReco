#ifndef TPCSOFT_ANGLEPROVIDERE1E2_H
#define TPCSOFT_ANGLEPROVIDERE1E2_H

#include "AngleProvider.h"

class TF1;

class AngleProviderE1E2 : public AngleProvider{
public:
    AngleProviderE1E2();
    double GetAngle() override;
protected:
    void ValidateParamValues() override;
private:
    double Theta(double* x, double *par);
    std::unique_ptr<TF1> thetaEmissionTF1;

    REGISTER_PROVIDER(AngleProviderE1E2)
};

#endif //TPCSOFT_ANGLEPROVIDERE1E2_H
