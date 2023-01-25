#ifndef TPCSOFT_REACTION_H
#define TPCSOFT_REACTION_H

#include "PrimaryParticle.h"
#include "IonProperties.h"
#include "Math/Vector4D.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "Math/Boost.h"
#include "Math/Rotation3D.h"

class Reaction {
public:
    Reaction();

    virtual ~Reaction() = default;

    virtual PrimaryParticles
    GeneratePrmaries(double gammaMom, ROOT::Math::Rotation3D &beamToDetRotation) = 0;

protected:
    std::shared_ptr<IonProperties> ionProp;

    void GetKinematics(double gammaMom, const double &targetMass);

    double totalEnergy{};
    ROOT::Math::PxPyPzEVector::BetaVector betaCM;

};

#endif //TPCSOFT_REACTION_H
