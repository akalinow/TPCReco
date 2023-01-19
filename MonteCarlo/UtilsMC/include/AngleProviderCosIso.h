#ifndef TPCSOFT_ANGLEPROVIDERCOSISO_H
#define TPCSOFT_ANGLEPROVIDERCOSISO_H
#include "AngleProvider.h"


class AngleProviderCosIso : public AngleProvider{
public:
    AngleProviderCosIso();
    double GetAngle() override;
protected:
    void ValidateParamValues() override;
};

#endif //TPCSOFT_ANGLEPROVIDERCOSISO_H
