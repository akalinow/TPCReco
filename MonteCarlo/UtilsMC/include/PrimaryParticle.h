#ifndef TPCSOFT_PRIMARYPARTICLE_H
#define TPCSOFT_PRIMARYPARTICLE_H

#include "CommonDefinitions.h"
#include "Math/Vector4D.h"
#include "Math/Vector3D.h"
#include "IonProperties.h"
#include <vector>

class PrimaryParticle {
public:
    PrimaryParticle() = default;

    PrimaryParticle(pid_type id,
                    ROOT::Math::PxPyPzEVector fourMom)
            : pID{id},
              fourMomentum{std::move(fourMom)} {}

    pid_type GetID() { return pID; }

    ROOT::Math::XYZVector GetMomentum() const { return fourMomentum.Vect(); }

    ROOT::Math::PxPyPzEVector GetFourMomentum() const { return fourMomentum; }

    double GetKineticEnergy() const { return fourMomentum.E()-fourMomentum.M(); }

    //Wrappers around IonProperties, might be useful
    inline int GetA() { return IonProperties::GetInstance()->GetA(pID); }

    inline int GetZ() { return IonProperties::GetInstance()->GetZ(pID); }

    inline double GetMass() { return fourMomentum.M(); }

    void SetID(const pid_type &id) { pID = id; }

    void SetFourMomentumLAB(const ROOT::Math::PxPyPzEVector &mom) { fourMomentum = mom; }


private:
    pid_type pID{pid_type::UNKNOWN};
    ROOT::Math::PxPyPzEVector fourMomentum;
};

typedef std::vector<PrimaryParticle> PrimaryParticles;

#endif //TPCSOFT_PRIMARYPARTICLE_H
