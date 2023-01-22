#ifndef TPCSOFT_REACTION_H
#define TPCSOFT_REACTION_H

#include "PrimaryParticle.h"
#include "IonProperties.h"
#include "Math/Vector4D.h"
#include "Math/Point3D.h"
#include "Math/Boost.h"

class Reaction {
public:
    Reaction();
    virtual ~Reaction() = default;
    virtual PrimaryParticles
    GeneratePrmaries(const ROOT::Math::XYZVector &gammaMom, const ROOT::Math::XYZPoint &vertexPos) = 0;
protected:
    std::shared_ptr<IonProperties> ionProp;
    void GetKinematics(const ROOT::Math::XYZVector &gammaMom, const double &targetMass);

    //Generate four momentum of a decay product. Use additional vector to define rotation axis for theta angle
    static ROOT::Math::PxPyPzMVector
    GenerateFourMomentum(double mom, double mass, double theta, double phi, const ROOT::Math::XYZVector &primaryDirection,
                         const ROOT::Math::XYZVector &thetaPlaneNormal);
    double totalEnergy{};
    ROOT::Math::PxPyPzMVector::BetaVector betaCM;

};

#endif //TPCSOFT_REACTION_H
