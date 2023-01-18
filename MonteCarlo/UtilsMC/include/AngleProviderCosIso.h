#ifndef TPCSOFT_ANGLEPROVIDERCOSISO_H
#define TPCSOFT_ANGLEPROVIDERCOSISO_H
#include "AngleProvider.h"


class AngleProviderCosIso : public AngleProvider{
public:
    AngleProviderCosIso();
    AngleProviderCosIso(double min, double max);
    double GetAngle() override;
private:
    double aCosMin;
    double aCosMax;
};

#endif //TPCSOFT_ANGLEPROVIDERCOSISO_H
