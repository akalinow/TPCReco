#ifndef TPCSOFT_REACTIONPARTICLEGUN_H
#define TPCSOFT_REACTIONPARTICLEGUN_H

#include "Reaction.h"
#include "AngleProvider.h"
#include "EProvider.h"

class ReactionParticleGun : public Reaction {
public:
    ReactionParticleGun(
            std::unique_ptr<AngleProvider> theta,
            std::unique_ptr<AngleProvider> phi,
            std::unique_ptr<EProvider> energy,
            pid_type ionType);

    PrimaryParticles
    GeneratePrimaries(double v, const ROOT::Math::Rotation3D &beamToDetRotation) override;

private:
    double particleMass;
    pid_type particleId;
    std::unique_ptr<AngleProvider> thetaProv;
    std::unique_ptr<AngleProvider> phiProv;
    std::unique_ptr<EProvider> eProv;
};

#endif //TPCSOFT_REACTIONPARTICLEGUN_H
