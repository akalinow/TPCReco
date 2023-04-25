#include "TrackTruncator.h"
#include "UtilsMath.h"


fwk::VModule::EResultFlag TrackTruncator::Init(boost::property_tree::ptree config) {
    includeElectronicsRange = config.get<bool>("IncludeElectronicsRange");
    BuildPlanes();
    bool err;
    offsetU = geometry->Strip2posUVW(definitions::DIR_U,1,err);
    offsetV = geometry->Strip2posUVW(definitions::DIR_V,1,err);
    offsetW = geometry->Strip2posUVW(definitions::DIR_W,1,err);

    if(geometry->IsStripDirReversed(definitions::DIR_U))
        dirU=-1;

    if(geometry->IsStripDirReversed(definitions::DIR_V))
        dirV=-1;

    if(geometry->IsStripDirReversed(definitions::DIR_W))
        dirW=-1;

    stripPitch = geometry->GetStripPitch();
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
    //second loop over tracks - assign truncated positions to UVWT coordinates
    for (auto &t: currentSimEvent.GetTracks()) {
        auto start = t.GetTruncatedStart();
        auto stop = t.GetTruncatedStop();
        bool err;
        auto startU = dirU*(geometry->Cartesian2posUVW(start.X(), start.Y(), definitions::DIR_U, err) - offsetU)/stripPitch+1;
        auto startV = dirV*(geometry->Cartesian2posUVW(start.X(), start.Y(), definitions::DIR_V, err) - offsetV)/stripPitch+1;
        auto startW = dirW*(geometry->Cartesian2posUVW(start.X(), start.Y(), definitions::DIR_W, err) - offsetW)/stripPitch+1;
        auto startT = geometry->Pos2timecell(start.Z(),err);

        auto stopU = dirU*(geometry->Cartesian2posUVW(stop.X(), stop.Y(), definitions::DIR_U, err) - offsetU)/stripPitch+1;
        auto stopV = dirV*(geometry->Cartesian2posUVW(stop.X(), stop.Y(), definitions::DIR_V, err) - offsetV)/stripPitch+1;
        auto stopW = dirW*(geometry->Cartesian2posUVW(stop.X(), stop.Y(), definitions::DIR_W, err) - offsetW)/stripPitch+1;
        auto stopT = geometry->Pos2timecell(stop.Z(),err);

        t.SetTruncatedStartUVWT({startU,startV,startW,startT});
        t.SetTruncatedStopUVWT({stopU,stopV,stopW,stopT});

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
