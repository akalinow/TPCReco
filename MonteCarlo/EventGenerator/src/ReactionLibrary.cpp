#include "TPCReco/ReactionLibrary.h"
#include <stdexcept>
#include "Math/Rotation3D.h"

void ReactionLibrary::RegisterReaction(std::unique_ptr<Reaction> reaction, double branchingRatio,
                                       reaction_type reactionType) {
    reactions.push_back({std::move(reaction), branchingRatio, reactionType});
}

void ReactionLibrary::Init() {
    auto nReactions = reactions.size();
    selectionHelperHisto = std::make_unique<TH1D>("", "", nReactions, 0, nReactions);
    for (auto i = 0U; i < reactions.size(); i++) {
        selectionHelperHisto->SetBinContent(i + 1, reactions[i].BR);
    }
    if(selectionHelperHisto->Integral()==0)
        throw std::runtime_error("At least one reaction has to have non-zero branching ratio!");
    initialized = true;
}

std::pair<PrimaryParticles, reaction_type>
ReactionLibrary::Generate(double gammaMom, ROOT::Math::Rotation3D &beamToDetRotation) const {
    if (!initialized)
        throw std::runtime_error("ReactionLibrary is not initialized! Call ReactionLibrary::Init() first!");
    auto reactionId = selectionHelperHisto->FindBin(selectionHelperHisto->GetRandom()) - 1;
    auto primaries = reactions[reactionId].aReaction->GeneratePrimaries(gammaMom, beamToDetRotation);
    auto type = reactions[reactionId].type;
    return {primaries, type};
}

