#include "TrackTruncator.h"

fwk::VModule::EResultFlag TrackTruncator::Init(boost::property_tree::ptree config) {
    geometry = std::make_shared<GeometryTPC>(config.get<std::string>("GeometryConfig").c_str());
    return eSuccess;
}

fwk::VModule::EResultFlag TrackTruncator::Process(ModuleExchangeSpace &event) {
    return eSuccess;
}

fwk::VModule::EResultFlag TrackTruncator::Finish() {
    return eSuccess;
}
