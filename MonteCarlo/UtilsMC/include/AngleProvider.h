#ifndef TPCSOFT_ANGLEPROVIDER_H
#define TPCSOFT_ANGLEPROVIDER_H

#include <memory>
#include "Provider.h"
#include <TRandom3.h>
#include <string>
#include <map>

class AngleProvider : public Provider{
public:
    ~AngleProvider() override =default;
    virtual double GetAngle()=0;
};

#endif //TPCSOFT_ANGLEPROVIDER_H
