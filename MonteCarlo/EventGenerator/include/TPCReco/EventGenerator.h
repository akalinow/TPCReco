#ifndef TPCSOFT_EVENTGENERATOR_H
#define TPCSOFT_EVENTGENERATOR_H

#include "TPCReco/PrimaryParticle.h"
#include "ReactionLibrary.h"
#include "ZProvider.h"
#include "XYProvider.h"
#include "GeneratorSetup.h"
#include "boost/property_tree/ptree.hpp"
#include "Math/Rotation3D.h"
#include "TPCReco/SimEvent.h"

class EventGenerator {
public:
    EventGenerator() = delete;

    EventGenerator(const boost::property_tree::ptree &configNode);

    EventGenerator(const boost::filesystem::path &configFilePath);

    SimEvent GenerateEvent();



private:
    void Init();
    ROOT::Math::XYZPoint GenerateVertexPosition();
    ReactionLibrary lib{};
    std::unique_ptr<ZProvider> zProv;
    std::unique_ptr<XYProvider> xyProv;
    std::unique_ptr<EProvider> eProv;
    GeneratorSetup setup;
    ROOT::Math::Rotation3D beamToDetRotation;
    ROOT::Math::XYZVector beamPosition;
};


#endif //TPCSOFT_EVENTGENERATOR_H
