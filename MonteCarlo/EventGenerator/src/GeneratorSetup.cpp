#include "GeneratorSetup.h"
#include "stdexcept"
#include "ReactionTwoProng.h"
#include "ReactionThreeProngDemocratic.h"
#include "ReactionThreeProngIntermediate.h"
#include "ReactionParticleGun.h"
#include <boost/property_tree/json_parser.hpp>


namespace pt = boost::property_tree;

GeneratorSetup::GeneratorSetup(const boost::filesystem::path &configFilePath) {
    pt::read_json(configFilePath.string(), topNode);
}

GeneratorSetup::GeneratorSetup(const pt::ptree &configNode) : topNode{configNode} {}


void GeneratorSetup::BuildReactionLibrary(ReactionLibrary &lib) {
    //auto reactions = topNode.get_child("Reactions");
    for (auto &r: topNode.get_child("Reactions")) {
        auto reactionName = r.second.get<std::string>("type");
        //parse BR and reaction type:
        auto branchingRatio = r.second.get<double>("branchingRatio");
        auto reaction = ParseReactionType(r.second.get<std::string>("tag"));
        if (reactionName == "TwoProng") {
            //parse particles:
            auto target = ParseParticle(r.second.get<std::string>("target"));
            auto firstProd = ParseParticle(r.second.get<std::string>("FirstProduct"));
            auto secondProd = ParseParticle(r.second.get<std::string>("SecondProduct"));
            //parse angular distributions:
            auto thetaProv = BuildProvider<AngleProvider>(r.second.get_child("Theta"));
            auto phiProv = BuildProvider<AngleProvider>(r.second.get_child("Phi"));

            auto twoProng = std::unique_ptr<Reaction>(
                    new ReactionTwoProng(std::move(thetaProv), std::move(phiProv), target, firstProd, secondProd));
            lib.RegisterReaction(std::move(twoProng), branchingRatio, reaction);
        } else if (reactionName == "ThreeProngDemocratic") {
            auto threeProngDemocratic = std::unique_ptr<Reaction>(new ReactionThreeProngDemocratic());
            lib.RegisterReaction(std::move(threeProngDemocratic), branchingRatio, reaction);
        } else if (reactionName == "ThreeProngIntermediate") {
            auto thetaProv1 = BuildProvider<AngleProvider>(r.second.get_child("Theta1"));
            auto phiProv1 = BuildProvider<AngleProvider>(r.second.get_child("Phi1"));
            auto thetaProv2 = BuildProvider<AngleProvider>(r.second.get_child("Theta2"));
            auto phiProv2 = BuildProvider<AngleProvider>(r.second.get_child("Phi2"));
            std::vector<ReactionThreeProngIntermediate::IntermediateState> intermediates;
            for (auto &is: r.second.get_child("IntermediateStates")) {
                auto m = is.second.get<double>("mass");
                auto w = is.second.get<double>("width");
                auto br = is.second.get<double>("branchingRatio");
                intermediates.emplace_back(m, w, br);
            }
            auto threeProngIntermediate = std::unique_ptr<Reaction>(
                    new ReactionThreeProngIntermediate(std::move(thetaProv1), std::move(phiProv1),
                                                       std::move(thetaProv2), std::move(phiProv2), intermediates));
            lib.RegisterReaction(std::move(threeProngIntermediate), branchingRatio, reaction);
        } else if (reactionName == "ParticleGun") {
            auto thetaProv = BuildProvider<AngleProvider>(r.second.get_child("Theta"));
            auto phiProv = BuildProvider<AngleProvider>(r.second.get_child("Phi"));
            auto eProv = BuildProvider<EProvider>(r.second.get_child("KineticEnergy"));
            auto particle = ParseParticle(r.second.get<std::string>("Particle"));
            auto particleGun = std::unique_ptr<Reaction>(new ReactionParticleGun(std::move(thetaProv),std::move(phiProv),std::move(eProv),particle));
            lib.RegisterReaction(std::move(particleGun),branchingRatio,reaction);

        } else
            throw std::runtime_error("Unknown reaction type: " + reactionName);
    }
    //Initialize library:
    lib.Init();
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
    g.phiNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.phi");
    g.thetaNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.theta");
    g.psiNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.psi");

    g.phiAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.phi");
    g.thetaAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.theta");
    g.psiAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.psi");

    auto px = topNode.get<double>("Beam.BeamGeometry.BeamPosition.x");
    auto py = topNode.get<double>("Beam.BeamGeometry.BeamPosition.y");
    auto pz = topNode.get<double>("Beam.BeamGeometry.BeamPosition.z");
    g.beamPos = ROOT::Math::XYZPoint{px, py, pz};
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
