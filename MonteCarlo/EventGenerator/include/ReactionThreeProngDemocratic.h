#ifndef TPCSOFT_REACTIONTHREEPRONGDEMOCRATIC_H
#define TPCSOFT_REACTIONTHREEPRONGDEMOCRATIC_H

#include "Reaction.h"

class ReactionThreeProngDemocratic : public Reaction {
public:
    ReactionThreeProngDemocratic() = default;

    PrimaryParticles
    GeneratePrmaries(double v, const ROOT::Math::Rotation3D &beamToDetRotation) override;
};

#endif //TPCSOFT_REACTIONTHREEPRONGDEMOCRATIC_H
