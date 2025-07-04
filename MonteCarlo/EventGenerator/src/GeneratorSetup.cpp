#include "TPCReco/GeneratorSetup.h"
#include "stdexcept"
#include "TPCReco/ReactionTwoProng.h"
#include "TPCReco/ReactionThreeProngDemocratic.h"
#include "TPCReco/ReactionThreeProngIntermediate.h"
#include "TPCReco/ReactionParticleGun.h"
#include <boost/property_tree/json_parser.hpp>
#include "TPCReco/ConfigManager.h"

namespace pt = boost::property_tree;

GeneratorSetup::GeneratorSetup(const boost::filesystem::path &configFilePath) {
    pt::read_json(configFilePath.string(), topNode);
}

GeneratorSetup::GeneratorSetup(const pt::ptree &configNode) : topNode{configNode} {

    int seed = 0;

    if(topNode.count("RandomSeed")){
        seed = ConfigManager::getScalar<unsigned int>(topNode, "RandomSeed");
    }

    std::cout<<KBLU<<"Setting random "<<RST<<seed<<KBLU<<" as seed for random number generator."<<RST<<std::endl;
    gRandom->SetSeed(seed);
}


void GeneratorSetup::BuildReactionLibrary(ReactionLibrary &lib) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    for (auto &r: topNode.get_child("Reactions")) {
        // auto reactionName = r.second.get<std::string>("type"); // without MATH expressions
        auto reactionName = ConfigManager::getScalar<std::string>(r.second, "type");
        //parse BR and reaction type:
        // auto branchingRatio = r.second.get<double>("branchingRatio"); // without MATH expressions
        auto branchingRatio = ConfigManager::getScalar<double>(r.second, "branchingRatio");
        // auto reaction = ParseReactionType(r.second.get<std::string>("tag")); // without MATH expressions
        auto reaction = ParseReactionType(ConfigManager::getScalar<std::string>(r.second, "tag")); // arrbitrary text label
        if (reactionName == "TwoProng") {
            //parse particles:
            // auto target = ParseParticle(r.second.get<std::string>("target")); // without MATH expressions
            // auto firstProd = ParseParticle(r.second.get<std::string>("FirstProduct")); // without MATH expressions
            // auto secondProd = ParseParticle(r.second.get<std::string>("SecondProduct")); // without MATH expressions
            auto target = ParseParticle(ConfigManager::getScalar<std::string>(r.second, "target")); // see CommonDefinitions.h
            auto firstProd = ParseParticle(ConfigManager::getScalar<std::string>(r.second, "FirstProduct")); // see CommonDefinitions.h
            auto secondProd = ParseParticle(ConfigManager::getScalar<std::string>(r.second, "SecondProduct")); // see CommonDefinitions.h
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
	        // auto m = is.second.get<double>("mass"); // without MATH expressions
                // auto w = is.second.get<double>("width"); // without MATH expressions
                // auto br = is.second.get<double>("branchingRatio"); // without MATH expressions
                auto m = ConfigManager::getScalar<double>(is.second, "mass"); // [MeV]
                auto w = ConfigManager::getScalar<double>(is.second, "width"); // [MeV]
                auto br = ConfigManager::getScalar<double>(is.second, "branchingRatio"); // range [0,1]
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
            // auto particle = ParseParticle(r.second.get<std::string>("Particle")); // without MATH expressions
            auto particle = ParseParticle(ConfigManager::getScalar<std::string>(r.second, "Particle")); // see CommonDefinitions.h
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
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    BeamGeometry g{};
    // Euler angles of rotation matrix from DET coordinate system (Z along E-drift) to nominal BEAM coordinate system (Z along photon)
    // g.phiNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.phi"); // withoout MATH expressions
    // g.thetaNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.theta"); // withoout MATH expressions
    // g.psiNom = topNode.get<double>("Beam.BeamGeometry.EulerAnglesNominal.psi"); // withoout MATH expressions
    g.phiNom = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesNominal.phi"); // [rad]
    g.thetaNom = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesNominal.theta"); // [rad]
    g.psiNom = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesNominal.psi"); // [rad]

    // Euler angles (small corrections) of rotation matrix from nominal BEAM to actual BEAM coordinate system:
    // g.phiAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.phi"); // withoout MATH expressions
    // g.thetaAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.theta"); // withoout MATH expressions
    // g.psiAct = topNode.get<double>("Beam.BeamGeometry.EulerAnglesActual.psi"); // withoout MATH expressions
    g.phiAct = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesActual.phi"); // [rad]
    g.thetaAct = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesActual.theta"); // [rad]
    g.psiAct = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.EulerAnglesActual.psi"); // [rad]

    // Origin offset of BEAM coordinate system in DET coordinate system:
    // auto px = topNode.get<double>("Beam.BeamGeometry.BeamPosition.x"); // withoout MATH expressions
    // auto py = topNode.get<double>("Beam.BeamGeometry.BeamPosition.y"); // withoout MATH expressions
    // auto pz = topNode.get<double>("Beam.BeamGeometry.BeamPosition.z"); // withoout MATH expressions
    auto px = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.BeamPosition.x"); // X_DET [mm]
    auto py = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.BeamPosition.y"); // Y_DET [mm]
    auto pz = ConfigManager::getScalar<double>(topNode, "Beam.BeamGeometry.BeamPosition.z"); // Z_DET [mm]
    g.beamPos = ROOT::Math::XYZPoint{px, py, pz};
    return g;
}

template<typename ProviderType>
std::unique_ptr<ProviderType> GeneratorSetup::BuildProvider(const boost::property_tree::ptree &node) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    // auto type = node.get<std::string>("distribution"); // without MATH expressions
    auto type = ConfigManager::getScalar<std::string>(node, "distribution");
    auto prov = ProviderFactory::Create<ProviderType>(type);
    if (!prov)
        throw std::runtime_error(
                "Unable to build " + type + "! Check if naming is correct, or is it the right type of the provider.");
    Provider::paramMapType params;
    for (auto &arg: node.get_child("parameters")) {
        // params[arg.first] = arg.second.get<double>(""); // without MATH expressions
        params[arg.first] = ConfigManager::getScalar<double>(arg.second, "");
    }
    prov->SetParams(params);
    return prov;
}
//
