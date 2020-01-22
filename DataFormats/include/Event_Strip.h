#pragma once
#include <map>
#include "Geometry_Strip.h"

class Event_Strip : public Geometry_Strip {
private:
    std::map<int, double> chargeByTimeCell;

public:
    Event_Strip() = default;
    ~Event_Strip() = default;

    double& operator[](int index);
    decltype(chargeByTimeCell)& charge() { return chargeByTimeCell; };
};