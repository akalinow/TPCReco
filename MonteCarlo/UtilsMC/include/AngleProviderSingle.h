#ifndef TPCSOFT_ANGLEPROVIDERSINGLE_H
#define TPCSOFT_ANGLEPROVIDERSINGLE_H

#include "AngleProvider.h"

class AngleProviderSingle : public AngleProvider{
public:
    AngleProviderSingle() :angle{0} {}
    explicit AngleProviderSingle(double an) :angle{an} {}
    double GetAngle() override { return angle; };
private:
    double angle;
};

#endif //TPCSOFT_ANGLEPROVIDERSINGLE_H
