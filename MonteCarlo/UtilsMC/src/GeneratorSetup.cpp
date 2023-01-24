#include "GeneratorSetup.h"
#include "stdexcept"
#include "TwoProngReaction.h"

GeneratorSetup::GeneratorSetup(const boost::filesystem::path &configFilePath) {
    pugi::xml_document configTop;
    auto result = configTop.load_file(configFilePath.c_str());
    if (!result)
        throw std::runtime_error("Unable to parse \'" + configFilePath.string() + "\' file!");
    topNode = configTop.child("EventGeneratorConfig");
    Info();
}

GeneratorSetup::GeneratorSetup(pugi::xml_node configNode) : topNode{configNode} { Info(); }


void GeneratorSetup::BuildReactionLibrary(ReactionLibrary &lib) {
    auto reactions = topNode.child("Reactions");
    for (auto r: reactions.children()) {
        auto reactionType = std::string(r.attribute("type").as_string());
        if (reactionType == "TwoProngReaction") {
            //parse particles:
            auto target = ParseParticle(r.child("Target"));
            auto firstProd = ParseParticle(r.child("FirstProduct"));
            auto secondProd = ParseParticle(r.child("SecondProduct"));
            //parse angular distributions:
            auto thetaNode = r.child("Theta");
            if (!thetaNode)
                throw std::runtime_error("No theta settings in " + reactionType + "!");
            auto phiNode = r.child("Phi");
            if (!phiNode)
                throw std::runtime_error("No phi settings in " + reactionType + "!");
            auto thetaProv = BuildProvider<AngleProvider>(thetaNode);
            auto phiProv = BuildProvider<AngleProvider>(phiNode);
            //parse BR and reaction type:
            auto branchingRatio = ParseBranchingRatio(r);
            auto reaction = ParseReactionType(r);

            auto twoProng = std::unique_ptr<Reaction>(
                    new TwoProngReaction(std::move(thetaProv), std::move(phiProv), target, firstProd, secondProd));
            lib.RegisterReaction(std::move(twoProng), branchingRatio, reaction);
        } else
            throw std::runtime_error("Unknown reaction type: " + reactionType);
    }
}

pid_type GeneratorSetup::ParseParticle(pugi::xml_node descr) {
    auto particleName = std::string(descr.attribute("type").as_string());
    auto particle = enumDict::GetPidType(particleName);
    if (particle == pid_type::UNKNOWN)
        throw std::runtime_error("Unknown or missing particle: \'" + particleName + "\'!");
    return particle;
}

reaction_type GeneratorSetup::ParseReactionType(pugi::xml_node descr) {
    auto reactionName = std::string(descr.attribute("tag").as_string());
    auto reaction = enumDict::GetReactionType(reactionName);
    if (reaction == reaction_type::UNKNOWN)
        throw std::runtime_error("Unknown or missing reaction type: \'" + reactionName + "\'!");
    return reaction;
}

double GeneratorSetup::ParseBranchingRatio(pugi::xml_node descr) {
    if (!descr.attribute("branchingRatio"))
        throw std::runtime_error("Branching ratio not provided in " + std::string(descr.name()) + "!");
    auto branchingRatio = descr.attribute("branchingRatio").as_double();
    if (branchingRatio < 0)
        throw std::runtime_error("Branching ratio cannot be negative!");
    return branchingRatio;
}

std::unique_ptr<ZProvider> GeneratorSetup::BuildZProvider() {
    if (!topNode.child("Vertex").child("VertexLongitudinal"))
        throw std::runtime_error("No VertexLongitudinal configuration found!");
    return BuildProvider<ZProvider>(topNode.child("Vertex").child("VertexLongitudinal"));
}

std::unique_ptr<XYProvider> GeneratorSetup::BuildXYProvider() {
    if (!topNode.child("Vertex").child("VertexTransverse"))
        throw std::runtime_error("No VertexTransverse configuration found!");
    return BuildProvider<XYProvider>(topNode.child("Vertex").child("VertexTransverse"));
}

std::unique_ptr<EProvider> GeneratorSetup::BuildEProvider() {
    if (!topNode.child("Vertex").child("GammaEnergy"))
        throw std::runtime_error("No GammaEnergy configuration found!");
    return BuildProvider<EProvider>(topNode.child("Beam").child("GammaEnergy"));
}

void GeneratorSetup::ReadBeamProperties(ROOT::Math::XYZPoint &beamPosition, double &angleZ) {

    const auto &beamNode = topNode.child("Beam");
    if (!beamNode)
        throw std::runtime_error("No Beam configuration found!");
    auto angle = beamNode.child("BeamAngleZ").attribute("val").as_double();

    if (!beamNode.child("BeamPosition") || !beamNode.attribute("x") || !beamNode.attribute("y") ||
        !beamNode.attribute("z"))
        throw std::runtime_error("Missing or wrong BeamPosition configuration!");

    auto xPos = beamNode.child("BeamPosition").attribute("x").as_double();
    auto yPos = beamNode.child("BeamPosition").attribute("y").as_double();
    auto zPos = beamNode.child("BeamPosition").attribute("z").as_double();
    beamPosition.SetCoordinates(xPos, yPos, zPos);
    angleZ = angle;
}

//TODO:To be checked, why sometimes factory registration gets optimized-out
void GeneratorSetup::Info() {
    std::cout << "List of available providers:\n";
    for (const auto &p: ProviderFactory::GetRegiseredIdentifiers()) {
        auto prov = ProviderFactory::Create<Provider>(p);
        std::cout << '\t' << prov->GetName() << std::endl;
    }
}

template<typename ProviderType>
std::unique_ptr<ProviderType> GeneratorSetup::BuildProvider(const pugi::xml_node &descr) {
    auto type = std::string(descr.attribute("type").as_string());
    auto prov = ProviderFactory::Create<ProviderType>(type);
    if (!prov)
        throw std::runtime_error(
                "Unable to build " + type + "! Check if naming is correct, or is it the right type of the provider.");
    Provider::paramMapType params;
    for (auto arg: descr.children()) {
        params[std::string(arg.name())] = arg.attribute("val").as_double();
    }
    prov->SetParams(params);
    return prov;
}

