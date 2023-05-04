#ifndef TPCSOFT_PRIMARYPARTICLE_H
#define TPCSOFT_PRIMARYPARTICLE_H

#include "TPCReco/CommonDefinitions.h"
#include "TLorentzVector.h"
#include "IonProperties.h"
#include <vector>

class PrimaryParticle {
public:
    PrimaryParticle() = default;

    PrimaryParticle(pid_type id,
                    const TLorentzVector &fourMom)
            : pID{id},
              fourMomentum{fourMom} {}

    pid_type GetID() const { return pID; }

    TVector3 GetMomentum() const { return fourMomentum.Vect(); }

    TLorentzVector GetFourMomentum() const { return fourMomentum; }

    double GetKineticEnergy() const { return fourMomentum.E() - fourMomentum.M(); }

    //Wrappers around IonProperties, might be useful
    inline int GetA() const { return IonProperties::GetInstance()->GetA(pID); }

    inline int GetZ() const { return IonProperties::GetInstance()->GetZ(pID); }

    inline double GetMass() const { return fourMomentum.M(); }

    void SetID(const pid_type &id) { pID = id; }

    void SetFourMomentum(const TLorentzVector &mom) { fourMomentum = mom; }


private:
    pid_type pID{pid_type::UNKNOWN};
    TLorentzVector fourMomentum;
};

typedef std::vector<PrimaryParticle> PrimaryParticles;

#endif //TPCSOFT_PRIMARYPARTICLE_H
