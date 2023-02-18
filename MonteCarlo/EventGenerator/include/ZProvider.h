#ifndef TPCSOFT_ZPROVIDER_H
#define TPCSOFT_ZPROVIDER_H

#include "Provider.h"

class ZProvider : public Provider {
public:
    ~ZProvider() override = default;
    virtual double GetZ() = 0;
};

class ZProviderUniform : public ZProvider {
public:
    ZProviderUniform();
    double GetZ() override;
    void ValidateParamValues() override;
REGISTER_PROVIDER(ZProviderUniform)
};

class ZProviderSingle : public ZProvider {
public:
    ZProviderSingle();
    double GetZ() override;

    void ValidateParamValues() override {} //nothing to validate
REGISTER_PROVIDER(ZProviderSingle)
};

#endif //TPCSOFT_ZPROVIDER_H
