//
// Created by piotrek on 1/19/23.
//

#ifndef TPCSOFT_ANGLEPROVIDERPHI_H
#define TPCSOFT_ANGLEPROVIDERPHI_H

#include "AngleProvider.h"

class TF1;

class AngleProviderPhi : public AngleProvider{
public:
    AngleProviderPhi();
    double GetAngle() override;
    double Phi(double* x, double *par);
protected:
    void ValidateParamValues() override;
private:

    std::unique_ptr<TF1> phiEmissionTF1;
    static unsigned int nInstances;
};

#endif //TPCSOFT_ANGLEPROVIDERPHI_H
