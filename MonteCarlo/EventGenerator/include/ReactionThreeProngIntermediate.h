#ifndef TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H
#define TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H

#include "Reaction.h"
#include "AngleProvider.h"

class ReactionThreeProngIntermediate: public Reaction{
public:
    ReactionThreeProngIntermediate(
            std::unique_ptr<AngleProvider> thetaFirstDecay,
            std::unique_ptr<AngleProvider> phiFirstDecay,
            std::unique_ptr<AngleProvider> thetaSecondDecay,
            std::unique_ptr<AngleProvider> phiSecondDecay,
            pid_type targetIon,
            pid_type prod1Ion,
            pid_type prod2Ion) {}
    PrimaryParticles
    GeneratePrmaries(double v, const ROOT::Math::Rotation3D &beamToDetRotation) override;
};

#endif //TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H
