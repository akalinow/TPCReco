#ifndef TPCSOFT_ANGLEPROVIDER_H
#define TPCSOFT_ANGLEPROVIDER_H

#include <memory>
#include <TRandom3.h>

class AngleProvider{
public:
    AngleProvider()
        :randGen{std::make_unique<TRandom3>()} {}
    virtual ~AngleProvider()=default;
    virtual double GetAngle()=0;
protected:
    std::unique_ptr<TRandom> randGen;
};

#endif //TPCSOFT_ANGLEPROVIDER_H
