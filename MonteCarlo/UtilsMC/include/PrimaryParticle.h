#ifndef TPCSOFT_PRIMARYPARTICLE_H
#define TPCSOFT_PRIMARYPARTICLE_H

#include "CommonDefinitions.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "IonProperties.h"
#include <vector>

class PrimaryParticle {
public:
    PrimaryParticle() = default;

    PrimaryParticle(pid_type id,
                    ROOT::Math::XYZPoint start,
                    ROOT::Math::XYZVector mom,
                    double e)
            : pID{id},
              emissionPoint{std::move(start)},
              momentum{std::move(mom)},
              eKin{e} {}

    pid_type GetID() { return pID; }

    ROOT::Math::XYZPoint GetEmissionPoint() const { return emissionPoint; }

    ROOT::Math::XYZVector GetMomentum() const { return momentum; }

    double GetEnergy() const { return eKin; }

    //Wrappers around IonProperties, might be useful
    inline int GetA() { return IonProperties::GetInstance()->GetA(pID); }

    inline int GetZ() { return IonProperties::GetInstance()->GetZ(pID); }

    inline double GetMassMeV() { return IonProperties::GetInstance()->GetAtomMass(pID); }

    void SetID(const pid_type &id) { pID = id; }

    void SetEmissionPoint(const ROOT::Math::XYZPoint &start) { emissionPoint = start; }

    void SetMomentum(const ROOT::Math::XYZVector &mom) { momentum = mom; }

    void SetEnergy(const double &e) { eKin = e; }

private:
    pid_type pID{pid_type::UNKNOWN};
    ROOT::Math::XYZPoint emissionPoint;
    ROOT::Math::XYZVector momentum;
    double eKin{};
};

typedef std::vector<PrimaryParticle> PrimaryParticles;

#endif //TPCSOFT_PRIMARYPARTICLE_H
