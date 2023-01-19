#ifndef TPCSOFT_ANGLEPROVIDERE1E2_H
#define TPCSOFT_ANGLEPROVIDERE1E2_H

#include "AngleProvider.h"

class TF1;

class AngleProviderE1E2 : public AngleProvider{
public:
    AngleProviderE1E2();
    double GetAngle() override;
    double thetaEmission(double* x, double *par);
protected:
    void ValidateParamValues() override;
private:

    std::unique_ptr<TF1> thetaEmissionTF1;
    static unsigned int nInstances;
};

#endif //TPCSOFT_ANGLEPROVIDERE1E2_H
