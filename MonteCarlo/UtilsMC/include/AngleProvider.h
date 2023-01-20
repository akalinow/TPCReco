#ifndef TPCSOFT_ANGLEPROVIDER_H
#define TPCSOFT_ANGLEPROVIDER_H

#include "Provider.h"

class AngleProvider : public Provider{
public:
    ~AngleProvider() override =default;
    virtual double GetAngle()=0;
};

#endif //TPCSOFT_ANGLEPROVIDER_H
