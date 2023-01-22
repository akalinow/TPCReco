#include "Reaction.h"
#include "Math/Boost.h"
#include "Math/AxisAngle.h"

Reaction::Reaction(): ionProp{IonProperties::GetInstance()} {}

void Reaction::GetKinematics(const ROOT::Math::XYZVector &gammaMom, const double &targetMass) {
    ROOT::Math::PxPyPzMVector tot4Mom{gammaMom.X(),gammaMom.Y(),gammaMom.Z(),targetMass};
    betaCM=tot4Mom.BoostToCM();
    auto boost = ROOT::Math::Boost(betaCM);
    totalEnergy=boost(tot4Mom).E();
}

ROOT::Math::PxPyPzMVector
Reaction::GenerateFourMomentum(double mom, double mass, double theta, double phi, const ROOT::Math::XYZVector &primaryDirection, const ROOT::Math::XYZVector &thetaPlaneNormal) {
    if(primaryDirection.Dot(thetaPlaneNormal) != 0)
        throw std::runtime_error("Momentum direction and theta axis rotation are not perpendicular! Angles are ill-defined!");
    auto p3= mom * primaryDirection.Unit();
    auto p4=ROOT::Math::PxPyPzMVector(p3.X(),p3.Y(),p3.Z(),mass);
    auto rTheta=ROOT::Math::AxisAngle(thetaPlaneNormal, theta);
    auto rPhi=ROOT::Math::AxisAngle(primaryDirection, phi);
    return rPhi*rTheta*p4;
}