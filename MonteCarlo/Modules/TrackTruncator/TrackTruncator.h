#ifndef TPCSOFT_TRACKTRUNCATOR_H
#define TPCSOFT_TRACKTRUNCATOR_H

#include "VModule.h"
#include "GeometryTPC.h"

class TrackTruncator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;
private:
    std::shared_ptr<GeometryTPC> geometry;

};


#endif //TPCSOFT_TRACKTRUNCATOR_H
