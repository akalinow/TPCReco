#ifndef TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H
#define TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H

#include <utility>

#include "Reaction.h"
#include "AngleProvider.h"
#include "TH1D.h"
#include "TF1.h"

class ReactionThreeProngIntermediate: public Reaction{
public:
    struct IntermediateState{
        IntermediateState(double m, double w, double br) : mass{m},width{w},branchingRatio{br} {}
        double mass;
        double width;
        double branchingRatio;
    };
    ReactionThreeProngIntermediate(
            std::unique_ptr<AngleProvider> thetaFirstDecay,
            std::unique_ptr<AngleProvider> phiFirstDecay,
            std::unique_ptr<AngleProvider> thetaSecondDecay,
            std::unique_ptr<AngleProvider> phiSecondDecay,
            std::vector<IntermediateState> intermediateStates)
                :thetaFirst{std::move(thetaFirstDecay)}, thetaSecond{std::move(thetaSecondDecay)},
                 phiFirst{std::move(phiFirstDecay)}, phiSecond{std::move(phiSecondDecay)},
                 intermediateStates{std::move(intermediateStates)} {
        bwTF1=std::make_unique<TF1>("","TMath::BreitWigner(x, 0, 1)",-10,10);
    }
    PrimaryParticles
    GeneratePrimaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation) override;
    std::pair<double, double> SelectIntermediateState();
private:
    std::unique_ptr<AngleProvider> thetaFirst;
    std::unique_ptr<AngleProvider> thetaSecond;
    std::unique_ptr<AngleProvider> phiFirst;
    std::unique_ptr<AngleProvider> phiSecond;
    std::vector<IntermediateState> intermediateStates;
    bool isBrInitialized{false};
    std::unique_ptr<TH1D> brHelperHisto;
    std::unique_ptr<TF1> bwTF1;
    double BreitWignerRandom();
};

#endif //TPCSOFT_REACTIONTHREEPRONGINTERMEDIATE_H
