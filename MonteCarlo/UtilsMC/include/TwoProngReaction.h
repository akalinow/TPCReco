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
    GeneratePrmaries(const ROOT::Math::XYZVector &gammaMom, const ROOT::Math::XYZPoint &vertexPos) override;
private:
    std::unique_ptr<AngleProvider> thetaProv;
    std::unique_ptr<AngleProvider> phiProv;
    pid_type target;
    pid_type lightProd;
    pid_type heavyProd;
    double targetMass;
    double lightProdMass;
    double heavyProdMass;
};

#endif //TPCSOFT_TWOPRONGREACTION_H
