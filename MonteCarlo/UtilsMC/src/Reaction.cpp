#include "Reaction.h"
#include "Math/Boost.h"
#include "Math/AxisAngle.h"

Reaction::Reaction() : ionProp{IonProperties::GetInstance()} {}

void Reaction::GetKinematics(const double gammaMom, const double &targetMass) {
    ROOT::Math::PxPyPzEVector tot4Mom{0, 0, gammaMom, targetMass + gammaMom};
    betaCM = tot4Mom.BoostToCM();
    auto boost = ROOT::Math::Boost(betaCM);
    totalEnergy = boost(tot4Mom).E();
}