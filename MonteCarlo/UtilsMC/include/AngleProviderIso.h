#ifndef TPCSOFT_ANGLEPROVIDERISO_H
#define TPCSOFT_ANGLEPROVIDERISO_H
#include "AngleProvider.h"


class AngleProviderIso : public AngleProvider{
public:
    AngleProviderIso();
    AngleProviderIso(double min, double max);
    double GetAngle() override;
private:
    double aMin;
    double aMax;
};

#endif //TPCSOFT_ANGLEPROVIDERISO_H
