#ifndef TPCSOFT_TRACKTRUNCATOR_H
#define TPCSOFT_TRACKTRUNCATOR_H

#include "VModule.h"
#include "GeometryTPC.h"

class TrackTruncator : public fwk::VModule {
private:
    struct Plane3D {
        TVector3 basePoint;
        TVector3 spanVec1;
        TVector3 spanVec2;
        bool isHorizontal;
    };
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:

    void BuildPlanes();

    std::vector<TVector3> FindIntersections(const TVector3 &offset, const TVector3 &tangent);


    std::vector<Plane3D> planes;
    bool includeElectronicsRange{false};
    unsigned int nWalls;

REGISTER_MODULE(TrackTruncator)
};


#endif //TPCSOFT_TRACKTRUNCATOR_H
