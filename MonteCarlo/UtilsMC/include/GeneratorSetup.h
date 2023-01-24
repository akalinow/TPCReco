#ifndef TPCSOFT_GENERATORSETUP_H
#define TPCSOFT_GENERATORSETUP_H

#include "boost/filesystem.hpp"
#include "pugixml.h"
#include "ReactionLibrary.h"
#include "AngleProvider.h"
#include "ZProvider.h"
#include "XYProvider.h"
#include "EProvider.h"
#include "boost/property_tree/ptree.hpp"


class GeneratorSetup {
public:
    GeneratorSetup() = delete;

    GeneratorSetup(const boost::filesystem::path &configFilePath);

    explicit GeneratorSetup(const boost::property_tree::ptree& configNode);

    void BuildReactionLibrary(ReactionLibrary &lib);

    std::unique_ptr<XYProvider> BuildXYProvider();

    std::unique_ptr<ZProvider> BuildZProvider();

    std::unique_ptr<EProvider> BuildEProvider();
//
//    void ReadBeamProperties(ROOT::Math::XYZPoint &beamPosition, double &angleZ);

private:
    boost::property_tree::ptree topNode;

    template<typename ProviderType>
    static std::unique_ptr<ProviderType> BuildProvider(const boost::property_tree::ptree& node);

    static pid_type ParseParticle(const std::string& partName);

    static reaction_type ParseReactionType(const std::string& reactionName);


    static void Info();
};

#endif //TPCSOFT_GENERATORSETUP_H
