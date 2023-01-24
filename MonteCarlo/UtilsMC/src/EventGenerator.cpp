#include "EventGenerator.h"

EventGenerator::EventGenerator(boost::property_tree::ptree configNode) : setup{configNode} {
    Init();
}

EventGenerator::EventGenerator(const boost::filesystem::path &configFilePath) : setup{configFilePath} {

    Init();
}

void EventGenerator::Init() {
    setup.BuildReactionLibrary(lib);
    xyProv = setup.BuildXYProvider();
    zProv=setup.BuildZProvider();
    eProv=setup.BuildEProvider();
//    setup.ReadBeamProperties(beamPosition,beamAngleZ);
}
