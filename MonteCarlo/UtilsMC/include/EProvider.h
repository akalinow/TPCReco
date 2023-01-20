#ifndef TPCSOFT_EPROVIDER_H
#define TPCSOFT_EPROVIDER_H

#include "Provider.h"

class EProvider : public Provider{
public:
    ~EProvider() override = default;
    virtual double GetEnergy()=0;
};

class EProviderSingle : public EProvider{
public:
    EProviderSingle();
    double GetEnergy() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(EProviderSingle)
};

class EProviderGaus : public EProvider{
public:
    EProviderGaus();
    double GetEnergy() override;
protected:
    void ValidateParamValues() override;

REGISTER_PROVIDER(EProviderGaus)
};

#endif //TPCSOFT_EPROVIDER_H
