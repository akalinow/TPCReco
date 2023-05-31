#ifndef TPCSOFT_REACTIONLIBRARY_H
#define TPCSOFT_REACTIONLIBRARY_H

#include <memory>
#include "Reaction.h"
#include "TPCReco/CommonDefinitions.h"
#include "TH1D.h"
#include "Math/Vector3D.h"

class ReactionLibrary {
public:
    ReactionLibrary() = default;

    void RegisterReaction(std::unique_ptr<Reaction> reaction, double branchingRatio, reaction_type reactionType);

    std::pair<PrimaryParticles, reaction_type> Generate(double gammaMom, ROOT::Math::Rotation3D &beamToDetRotation) const;

    void Init();

private:
    struct ReactionEntry {
        std::unique_ptr<Reaction> aReaction;
        double BR;
        reaction_type type;
    };

    std::vector<ReactionEntry> reactions;
    std::unique_ptr<TH1D> selectionHelperHisto;
    bool initialized = false;
};


#endif //TPCSOFT_REACTIONLIBRARY_H
