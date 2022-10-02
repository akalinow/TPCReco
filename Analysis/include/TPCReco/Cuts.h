#ifndef TPCRECO_ANALYSIS_CUTS_H_
#define TPCRECO_ANALYSIS_CUTS_H_
#include "TPCReco/CommonDefinitions.h"
#include <algorithm>
#include <iostream>
#include <map>
namespace tpcreco {
namespace cuts {

// cut: reject empty events
struct NonEmpty {
  template <class Track> bool operator()(Track *track) {
    return track->getSegments().size() != 0;
  }
};
using Cut1 = NonEmpty;

// cut: XY plane : vertex position per event, corrected for beam tilt
struct VertexPosition {
  const double beamOffset = 0.;
  const double beamSlope = 0.;
  const double beamDiameter = 0.;
  template <class Track> bool operator()(Track *track) {
    auto vertexPos = track->getSegments().front().getStart();
    return std::abs(vertexPos.Y() - (beamOffset + beamSlope * vertexPos.X())) <=
           0.5 * beamDiameter;
  }
};
using Cut2 = VertexPosition;

// cut: XY plane : minimal distance to the border of UVW active area
// - less strict than simple XY rectangular cut, allows to gain some statistics

// class DistanceToBorder {
//   template <class Geometry>
//   DistanceToBorder(Geometry *geometry, margin_mm) : margin_mm(margin_mm) {}
//   template <class Track> bool operator()(Track *track) {
//     auto segments = track->getSegments();
//     return std::all_of(std::cbegin(segments), std::cend(segments),
//                        [](auto segment) { return })
//   }
//
// private:
//   const margin_mm;
// };
// using Cut3 = DistanceToBorder;

// cut: XY plane : rectangular cut per track
struct RectangularCut {
  const double minX;
  const double maxX;
  const double minY;
  const double maxY;
  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this](const auto &segment) {
                         auto start = segment.getStart();
                         auto stop = segment.getStop();
                         return start.X() < maxX && start.X() > minX &&
                                start.Y() < maxY && start.Y() > minY &&
                                stop.X() < maxX && stop.X() > minX &&
                                stop.Y() < maxY && stop.Y() > minY;
                       });
  }
};
using Cut3a = RectangularCut;

// cut: global Z-span per event, verifies that:
// - vertical projection length is below physical drift cage length
// - tracks do not overlap with pedestal exclusion zone, begin of history buffer
// - tracks not too close to end of history buffer
class GlobalZSpan {
public:
  template <class Geometry>
  GlobalZSpan(Geometry *geometry, double lowerMarginTimeCells,
              double upperMarginTimeCells) {
    lengthLimit =
        0.99 * (geometry->GetDriftCageZmax() - geometry->GetDriftCageZmin());
    auto err = false;
    lowerMargin = geometry->Timecell2pos(lowerMarginTimeCells, err);
    if (err) {
      throw std::runtime_error(
          "LowerSafetyMargin conversion to position failed");
    }
    upperMargin = geometry->Timecell2pos(
        geometry->GetAgetNtimecells() - upperMarginTimeCells, err);
    if (err) {
      throw std::runtime_error(
          "UpperSafetyMargin conversion to position failed");
    }
  }

  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
    auto zmin = segments.at(0).getStart().Z();
    auto zmax = zmin;
    for (size_t i = 0; i < segments.size(); ++i) {
      zmin = std::min(zmin, (std::min(segments[i].getStart().Z(),
                                      segments[i].getStop().Z())));
      zmax = std::max(zmax, (std::max(segments[i].getStart().Z(),
                                      segments[i].getStop().Z())));
    }
    return isInside(zmin, zmax);
  }

private:
  bool isInside(double z1, double z2) {
    return isInsideMargins(z1) && isInsideMargins(z2) &&
           isInsideDriftCage(z1, z2);
  }
  bool isInsideDriftCage(double z1, double z2) {
    return std::abs(z1 - z2) < lengthLimit;
  }
  bool isInsideMargins(double z) { return z > lowerMargin && z < upperMargin; }
  double lowerMargin;
  double upperMargin;
  double lengthLimit;
};
using Cut4 = GlobalZSpan;

// cut #5 : Z-span wrt vertex per track per event, verifies that:
// - vertical distance of endpoint to vertex is less than half of drift cage
// height corrected for maximal vertical beam spread
// - ensures that 2,3-prong events hit neither the GEM plane nor the cathode
// plane NOTE: does not protect against 1-prong events (eg. background)
// originating from the GEM plane or the cathode plane
class VertexZSpan {
public:
  template <class Geometry>
  VertexZSpan(Geometry *geometry, double beamDiameter)
      : driftCageLength(geometry->GetDriftCageZmax() -
                        geometry->GetDriftCageZmin()),
        beamDiameter(beamDiameter) {}

  template <class Track> bool operator()(Track *track) {
    auto &segments = track->getSegments();
    auto vertexZ = segments.front().getStart().Z();
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this, vertexZ](const auto &segment) {
                         return std::abs(segment.getStop().Z() - vertexZ) <=
                                0.5 * (driftCageLength - beamDiameter);
                       });
  }

private:
  double driftCageLength;
  double beamDiameter;
};
using Cut5 = VertexZSpan;

// cut #6 : Additional quality cuts for 2-prong events used by Artur for plots
// from automatic reconstruction that employs lustering + dE/dx method:
// - chi2 < 10
// - charge > 1000
// - length > 30 mm
// - eventType = 3
// - hypothesisChi2 < 5
// NOTE: For manual reconstruction disable these dE/dx fit quality checks
// NOTE: Those cuts are currently impossible to apply to results from manual
// data reconstruction and to fake data generated by toy MC. If we are going to
// use results from automatic reconstruction for demonstration of cross section
// measurement then those cuts must be taken into account as well while
// correcting the rates!
struct ReconstructionQuality2Prong {
  const pid_type firstPID;
  const pid_type secondPID;
  const double chi2;
  const double hypothesisChi2;
  const double length;
  const double charge;
  template <class Track> bool operator()(Track *track) {
    if (track->getSegments().size() != 2) {
      return true;
    }
    return track->getSegments().front().getPID() == firstPID &&
           track->getSegments().back().getPID() == secondPID &&
           track->getChi2() <= chi2 &&
           track->getHypothesisFitChi2() <= hypothesisChi2 &&
           track->getLength() >= length &&
           track->getIntegratedCharge(track->getLength()) >= charge;
  }
};
using Cut6 = ReconstructionQuality2Prong;

} // namespace cuts
} // namespace tpcreco

#endif // TPCRECO_ANALYSIS_CUTS_H_