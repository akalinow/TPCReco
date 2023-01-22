#include "Reaction.h"
#include "Math/Boost.h"
#include "Math/AxisAngle.h"

Reaction::Reaction() : ionProp{IonProperties::GetInstance()} {}

void Reaction::GetKinematics(const ROOT::Math::XYZVector &gammaMom, const double &targetMass) {
    ROOT::Math::PxPyPzEVector tot4Mom{gammaMom.X(), gammaMom.Y(), gammaMom.Z(), targetMass + gammaMom.R()};
    betaCM = tot4Mom.BoostToCM();
    auto boost = ROOT::Math::Boost(betaCM);
    totalEnergy = boost(tot4Mom).E();
}

ROOT::Math::PxPyPzEVector
Reaction::GenerateFourMomentum(double mom, double mass, double theta, double phi,
                               const ROOT::Math::XYZVector &primaryDirection,
                               const ROOT::Math::XYZVector &thetaPlaneNormal) {
    if (primaryDirection.Dot(thetaPlaneNormal) != 0)
        throw std::runtime_error(
                "Momentum direction and theta axis rotation are not perpendicular! Angles are ill-defined!");
    auto p3 = mom * primaryDirection.Unit();
    auto p4 = ROOT::Math::PxPyPzEVector(p3.X(), p3.Y(), p3.Z(), sqrt(mass * mass + p3.Mag2()));
    auto rTheta = ROOT::Math::AxisAngle(thetaPlaneNormal, theta);
    auto rPhi = ROOT::Math::AxisAngle(primaryDirection, phi);
    return rPhi * rTheta * p4;
}