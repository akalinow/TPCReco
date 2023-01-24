#include "GeneratorSetup.h"
#include "stdexcept"
#include "TwoProngReaction.h"
#include <boost/property_tree/json_parser.hpp>


namespace pt = boost::property_tree;

GeneratorSetup::GeneratorSetup(const boost::filesystem::path &configFilePath) {
    pt::read_json(configFilePath.string(), topNode);
}

GeneratorSetup::GeneratorSetup(const pt::ptree &configNode) : topNode{configNode} {}


void GeneratorSetup::BuildReactionLibrary(ReactionLibrary &lib) {
    //auto reactions = topNode.get_child("Reactions");
    for (auto &r: topNode.get_child("Reactions")) {
        auto reactionType = r.second.get<std::string>("type");
        if (reactionType == "TwoProngReaction") {
            //parse particles:
            auto target = ParseParticle(r.second.get<std::string>("target"));
            auto firstProd = ParseParticle(r.second.get<std::string>("target"));
            auto secondProd = ParseParticle(r.second.get<std::string>("target"));
            //parse angular distributions:
            auto thetaProv = BuildProvider<AngleProvider>(r.second.get_child("Theta"));
            auto phiProv = BuildProvider<AngleProvider>(r.second.get_child("Phi"));
            //parse BR and reaction type:
            auto branchingRatio = r.second.get<double>("branchingRatio");
            auto reaction = ParseReactionType(r.second.get<std::string>("tag"));

            auto twoProng = std::unique_ptr<Reaction>(
                    new TwoProngReaction(std::move(thetaProv), std::move(phiProv), target, firstProd, secondProd));
            lib.RegisterReaction(std::move(twoProng), branchingRatio, reaction);
        } else
            throw std::runtime_error("Unknown reaction type: " + reactionType);
    }
}

pid_type GeneratorSetup::ParseParticle(const std::string &partName) {
    auto particle = enumDict::GetPidType(partName);
    if (particle == pid_type::UNKNOWN)
        throw std::runtime_error("Unknown or missing particle: \'" + partName + "\'!");
    return particle;
}

reaction_type GeneratorSetup::ParseReactionType(const std::string &reactionName) {
    auto reaction = enumDict::GetReactionType(reactionName);
    if (reaction == reaction_type::UNKNOWN)
        throw std::runtime_error("Unknown or missing reaction type: \'" + reactionName + "\'!");
    return reaction;
}


std::unique_ptr<ZProvider> GeneratorSetup::BuildZProvider() {
    return BuildProvider<ZProvider>(topNode.get_child("Vertex.VertexLongitudinal"));
}

std::unique_ptr<XYProvider> GeneratorSetup::BuildXYProvider() {
    return BuildProvider<XYProvider>(topNode.get_child("Vertex.VertexTransverse"));
}

std::unique_ptr<EProvider> GeneratorSetup::BuildEProvider() {
    return BuildProvider<EProvider>(topNode.get_child("Beam.GammaEnergy"));
}

//void GeneratorSetup::ReadBeamProperties(ROOT::Math::XYZPoint &beamPosition, double &angleZ) {
//
//    const auto &beamNode = topNode.child("Beam");
//    if (!beamNode)
//        throw std::runtime_error("No Beam configuration found!");
//    auto angle = beamNode.child("BeamAngleZ").attribute("val").as_double();
//
//    if (!beamNode.child("BeamPosition") || !beamNode.child("BeamPosition").attribute("x") ||
//        !beamNode.child("BeamPosition").attribute("y") ||
//        !beamNode.child("BeamPosition").attribute("z"))
//        throw std::runtime_error("Missing or wrong BeamPosition configuration!");
//
//    auto xPos = beamNode.child("BeamPosition").attribute("x").as_double();
//    auto yPos = beamNode.child("BeamPosition").attribute("y").as_double();
//    auto zPos = beamNode.child("BeamPosition").attribute("z").as_double();
//    beamPosition.SetCoordinates(xPos, yPos, zPos);
//    angleZ = angle;
//}
//
//TODO:To be checked, why sometimes factory registration gets optimized-out
void GeneratorSetup::Info() {
    std::cout << "List of available providers:\n";
    for (const auto &p: ProviderFactory::GetRegiseredIdentifiers()) {
        auto prov = ProviderFactory::Create<Provider>(p);
        std::cout << '\t' << prov->GetName() << std::endl;
    }
}

GeneratorSetup::BeamGeometry GeneratorSetup::ReadBeamGeometry() {
    BeamGeometry g{};
    g.phiNom=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesNominal.phi");
    g.thetaNom=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesNominal.theta");
    g.psiNom=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesNominal.psi");

    g.phiAct=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesActual.phi");
    g.thetaAct=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesActual.theta");
    g.psiAct=topNode.get<double>("Beam.BeamGeometry.BeamEulerAnglesActual.psi");

    auto px=topNode.get<double>("Beam.BeamGeometry.BeamPosition.x");
    auto py=topNode.get<double>("Beam.BeamGeometry.BeamPosition.y");
    auto pz=topNode.get<double>("Beam.BeamGeometry.BeamPosition.z");
    g.beamPos=ROOT::Math::XYZPoint{px,py,pz};
    return g;
}

template<typename ProviderType>
std::unique_ptr<ProviderType> GeneratorSetup::BuildProvider(const boost::property_tree::ptree &node) {
    auto type = node.get<std::string>("distribution");
    auto prov = ProviderFactory::Create<ProviderType>(type);
    if (!prov)
        throw std::runtime_error(
                "Unable to build " + type + "! Check if naming is correct, or is it the right type of the provider.");
    Provider::paramMapType params;
    for (auto &arg: node.get_child("parameters")) {
        params[arg.first] = arg.second.get<double>("");
    }
    prov->SetParams(params);
    return prov;
}
//
