#ifndef TPCSOFT_TWOPRONGREACTION_H
#define TPCSOFT_TWOPRONGREACTION_H

#include "Reaction.h"
#include "AngleProvider.h"

class TwoProngReaction : public Reaction {
public:
    TwoProngReaction(
            std::unique_ptr<AngleProvider> theta,
            std::unique_ptr<AngleProvider> phi,
            pid_type targetIon,
            pid_type prod1Ion,
            pid_type prod2Ion);

    PrimaryParticles
    GeneratePrmaries(double gammaMom, ROOT::Math::Rotation3D &beamToDetRotation) override;

private:
    std::unique_ptr<AngleProvider> thetaProv;
    std::unique_ptr<AngleProvider> phiProv;
    pid_type target;
    pid_type prod1Pid;
    pid_type prod2Pid;
    double targetMass;
    double prod1Mass;
    double prod2Mass;
};

#endif //TPCSOFT_TWOPRONGREACTION_H
