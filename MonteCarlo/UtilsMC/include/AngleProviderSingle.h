#ifndef TPCSOFT_ANGLEPROVIDERSINGLE_H
#define TPCSOFT_ANGLEPROVIDERSINGLE_H

#include "AngleProvider.h"

class AngleProviderSingle : public AngleProvider{
public:
    AngleProviderSingle()
    {
        paramVals["singleAngle"]=0;
    }
    double GetAngle() override { return paramVals["singleAngle"]; };
protected:
    void ValidateParamValues() override {}
};

#endif //TPCSOFT_ANGLEPROVIDERSINGLE_H
