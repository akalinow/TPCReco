#include "TrackTruncator.h"
#include "TPCReco/UtilsMath.h"


fwk::VModule::EResultFlag TrackTruncator::Init(boost::property_tree::ptree config) {
    includeElectronicsRange = config.get<bool>("IncludeElectronicsRange");
    BuildPlanes();
    return eSuccess;
}

fwk::VModule::EResultFlag TrackTruncator::Process(ModuleExchangeSpace &event) {
    auto &currentSimEvent = event.simEvt;
    for (auto &t: currentSimEvent.GetTracks()) {

        //we start with assumption that the track is fully contained
        t.SetFullyContained(true);

        auto offset = t.GetStart();
        auto tangent = t.GetStop() - offset;
        auto startInside = geometry->IsInsideActiveVolume(offset);
        auto stopInside = geometry->IsInsideActiveVolume(offset + tangent);
        if (includeElectronicsRange) {
            startInside = startInside && geometry->IsInsideElectronicsRange(offset);
            stopInside = stopInside && geometry->IsInsideElectronicsRange(offset + tangent);
        }

        // both inside => nothing to do
        if (startInside && stopInside)
            continue;
        //we are here, so track is not fully contained:
        t.SetFullyContained(false);

        auto intersections = FindIntersections(offset, tangent);
        auto nIntersections = intersections.size();

        //no intersections, neither end inside => track is fully out
        if (nIntersections == 0 && !startInside && !stopInside) {
            t.SetOutOfActiveVolume(true); //it is false by default
            continue;
        }

        // one intersection => truncate track from one end
        if (nIntersections == 1) {
            //truncate end if start is inside active volume:
            if (startInside) {
                t.SetTruncatedStop(intersections.front());
            } else {
                t.SetTruncatedStart(intersections.front());
            }
            continue;
        }

        // sort list in ascending order according to distance from track's start-point
        std::sort(intersections.begin(), intersections.end(),
                  [&offset](const TVector3 &a, const TVector3 &b) {
                      return (a - offset).Mag2() < (b - offset).Mag2();
                  });

        t.SetTruncatedStart(intersections.front());
        t.SetTruncatedStop(intersections.back());


    }

    return eSuccess;
}

fwk::VModule::EResultFlag TrackTruncator::Finish() {
    return eSuccess;
}

void TrackTruncator::BuildPlanes() {
    double xmin, xmax, ymin, ymax, zmin, zmax;
    std::tie(xmin, xmax, ymin, ymax, zmin, zmax) = geometry->rangeXYZ();
    if (includeElectronicsRange) {
        bool err;
        zmin = geometry->Timecell2pos(0, err);
        zmax = geometry->Timecell2pos(geometry->GetAgetNtimecells() - 1, err);
    }
    //first, we build two horizontal planes limiting Z-range
    planes.push_back({{xmin, ymin, zmin}, {xmax - xmin, 0, 0}, {0, ymax - ymin, 0}, true});
    planes.push_back({{xmin, ymin, zmax}, {xmax - xmin, 0, 0}, {0, ymax - ymin, 0}, true});

    auto gr = geometry->GetActiveAreaConvexHull();

    for (auto edge = 0; edge < gr.GetN() - 1; edge++) {
        auto x = gr.GetX()[edge];
        auto y = gr.GetY()[edge];

        auto xNext = gr.GetX()[edge + 1];
        auto yNext = gr.GetY()[edge + 1];
        planes.push_back({{x, y, zmin}, {xNext - x, yNext - y, 0}, {0, 0, zmax - zmin}, false});
    }
    nWalls = planes.size();
}

std::vector<TVector3> TrackTruncator::FindIntersections(const TVector3 &offset, const TVector3 &tangent) {
    std::vector<TVector3> result;
    for (const auto &plane: planes) {
        TVector3 p;
        double t1 = -9999, t2 = -9999;
        if (!Utils::intersectionEdgePlane(offset, tangent, plane.basePoint,
                                          plane.spanVec1, plane.spanVec2, p, t1, t2) ||
            !((plane.isHorizontal && geometry->IsInsideActiveArea(TVector2(p.X(), p.Y()))) ||
              (!plane.isHorizontal && t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1))) {
            continue; // next wall
        }
        result.push_back(p);

    }
    return result;
}
