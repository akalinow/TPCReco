#ifndef TPCSOFT_GENERATORSETUP_H
#define TPCSOFT_GENERATORSETUP_H

#include "boost/filesystem.hpp"
#include "pugixml.h"
#include "ReactionLibrary.h"
#include "AngleProvider.h"
#include "ZProvider.h"
#include "XYProvider.h"
#include "EProvider.h"


class GeneratorSetup {
public:
    GeneratorSetup() = delete;

    GeneratorSetup(const boost::filesystem::path &configFilePath);

    explicit GeneratorSetup(pugi::xml_node configNode);

    void BuildReactionLibrary(ReactionLibrary &lib);

    std::unique_ptr<XYProvider> BuildXYProvider();

    std::unique_ptr<ZProvider> BuildZProvider();

    std::unique_ptr<EProvider> BuildEProvider();

    void ReadBeamProperties(ROOT::Math::XYZPoint &beamPosition, double &angleZ);

private:
    pugi::xml_node topNode;

    template<typename ProviderType>
    static std::unique_ptr<ProviderType> BuildProvider(const pugi::xml_node &descr);

    static pid_type ParseParticle(pugi::xml_node descr);

    static reaction_type ParseReactionType(pugi::xml_node descr);

    static double ParseBranchingRatio(pugi::xml_node descr);

    static void Info();
};

#endif //TPCSOFT_GENERATORSETUP_H
