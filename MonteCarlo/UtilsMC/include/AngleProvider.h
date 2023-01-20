#ifndef TPCSOFT_ANGLEPROVIDER_H
#define TPCSOFT_ANGLEPROVIDER_H

#include "Provider.h"

class TF1;

class AngleProvider : public Provider{
public:
    ~AngleProvider() override =default;
    virtual double GetAngle()=0;
};

class AngleProviderCosIso : public AngleProvider{
public:
    AngleProviderCosIso();
    double GetAngle() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(AngleProviderCosIso)
};

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

class AngleProviderIso : public AngleProvider{
public:
    AngleProviderIso();
    double GetAngle() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(AngleProviderIso)
};

class AngleProviderPhi : public AngleProvider{
public:
    AngleProviderPhi();
    double GetAngle() override;
    double Phi(double* x, double *par);
protected:
    void ValidateParamValues() override;
private:

    std::unique_ptr<TF1> phiEmissionTF1;
REGISTER_PROVIDER(AngleProviderPhi)
};

class AngleProviderSingle : public AngleProvider{
public:
    AngleProviderSingle()
    {
        paramVals["singleAngle"]=0;
    }
    double GetAngle() override { return paramVals["singleAngle"]; };
protected:
    void ValidateParamValues() override {}
REGISTER_PROVIDER(AngleProviderSingle)
};

#endif //TPCSOFT_ANGLEPROVIDER_H
