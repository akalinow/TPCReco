#ifndef TPCSOFT_ANGLEPROVIDERISO_H
#define TPCSOFT_ANGLEPROVIDERISO_H
#include "AngleProvider.h"


class AngleProviderIso : public AngleProvider{
public:
    AngleProviderIso();
    double GetAngle() override;
protected:
    void ValidateParamValues() override;
};

#endif //TPCSOFT_ANGLEPROVIDERISO_H
