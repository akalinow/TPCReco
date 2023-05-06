#ifndef TPCRECO_ANALYSIS_CUTS_H_
#define TPCRECO_ANALYSIS_CUTS_H_
#include "TPCReco/CommonDefinitions.h"
#include <TGraph.h>
#include <TH2Poly.h>
#include <TVector3.h>
#include <algorithm>
#include <map>
#include <memory>
/////// DEBUG
#include <iostream> // for DEBUG std::cout
/////// DEBUG
namespace tpcreco {
namespace cuts {

// cut: reject empty events
struct NonEmpty {
  template <class Track> bool operator()(Track *track) {
    return (track != nullptr) && (track->getSegments().size() != 0);
  }
};
using Cut1 = NonEmpty;

// cut: XY plane : vertex position per event, corrected for beam tilt
struct VertexPosition {
#ifndef __CINT__                                                        // FIX for ROOT macros
  VertexPosition(double offset=0., double slope=0., double diameter=0.) // FIX for ROOT macros
    : beamOffset(offset), beamSlope(slope), beamDiameter(diameter) { }  // FIX for ROOT macros
#endif                                                                  // FIX for ROOT macros
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
class DistanceToBorder {
public:
  template <class Geometry>
  DistanceToBorder(Geometry *geometry, double margin_mm) {
    auto *hull = new TGraph(geometry->GetActiveAreaConvexHull(margin_mm));
    bin = std::make_shared<TH2PolyBin>(hull, 1);
  }
  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
#ifdef __CINT__                                                              // FIX for ROOT macros
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this](const auto &segment) {
#else                                                                        // FIX for ROOT macros
    return std::all_of(segments.cbegin(), segments.cend(),                   // FIX for ROOT macros
                       [this](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif                                                                       // FIX for ROOT macros
                         return this->isInside(segment.getStart()) &&
                                this->isInside(segment.getEnd());
                       });
  }

private:
  bool isInside(TVector3 v) const { return bin->IsInside(v.X(), v.Y()); }
  std::shared_ptr<TH2PolyBin> bin;
};
using Cut3 = DistanceToBorder;

// cut: XY plane : rectangular cut per track
struct RectangularCut {
#ifndef __CINT__                                                         // FIX for ROOT macros
  RectangularCut(double _minX, double _maxX, double _minY, double _maxY) // FIX for ROOT macros
      : minX(_minX), maxX(_maxX), minY(_minY), maxY(_maxY) { }           // FIX for ROOT macros
#endif                                                                   // FIX for ROOT macros
  const double minX;
  const double maxX;
  const double minY;
  const double maxY;
  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
#ifdef __CINT__                                                              // FIX for ROOT macros
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this](const auto &segment) {
#else                                                                        // FIX for ROOT macros
    return std::all_of(segments.cbegin(), segments.cend(),                   // FIX for ROOT macros
                       [this](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif                                                                       // FIX for ROOT macros
                         auto start = segment.getStart();
                         auto stop = segment.getEnd();
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
// - tracks do not overlap with pedestal exclusion zone, begin of history
// buffer
// - tracks not too close to end of history buffer
class GlobalZSpan {
public:
  template <class Geometry>
  GlobalZSpan(const Geometry *geometry, double lowerMarginTimeCells,
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
                                      segments[i].getEnd().Z())));
      zmax = std::max(zmax, (std::max(segments[i].getStart().Z(),
                                      segments[i].getEnd().Z())));
    }
    return isInside(zmin, zmax);
  }

private:
  bool isInside(double z1, double z2) const noexcept {
    return isInsideMargins(z1) && isInsideMargins(z2) &&
           isInsideDriftCage(z1, z2);
  }
  bool isInsideDriftCage(double z1, double z2) const noexcept {
    return std::abs(z1 - z2) < lengthLimit;
  }
  bool isInsideMargins(double z) const noexcept {
    return z > lowerMargin && z < upperMargin;
  }
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
  VertexZSpan(const Geometry *geometry, double beamDiameter)
      : driftCageLength(geometry->GetDriftCageZmax() -
                        geometry->GetDriftCageZmin()),
        beamDiameter(beamDiameter) {}

  template <class Track> bool operator()(Track *track) {
    auto &segments = track->getSegments();
    auto vertexZ = segments.front().getStart().Z();
#ifdef __CINT__                                                                       // FIX for ROOT macros
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this, vertexZ](const auto &segment) {
#else                                                                                 // FIX for ROOT macros
    return std::all_of(segments.cbegin(), segments.cend(),                            // FIX for ROOT macros
                       [this, vertexZ](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif                                                                                // FIX for ROOT macros
                         return std::abs(segment.getEnd().Z() - vertexZ) <=
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
// data reconstruction and to fake data generated by toy MC. If we are going
// to use results from automatic reconstruction for demonstration of cross
// section measurement then those cuts must be taken into account as well
// while correcting the rates!
struct ReconstructionQuality2Prong {
#ifndef __CINT__                                                                      // FIX for ROOT macros
  ReconstructionQuality2Prong(pid_type _firstPID, pid_type _secondPID, double _chi2,  // FIX for ROOT macros
			      double _hypothesisChi2, double _length, double _charge) // FIX for ROOT macros
      : firstPID(_firstPID), secondPID(_secondPID), chi2(_chi2),                      // FIX for ROOT macros
        hypothesisChi2(_hypothesisChi2), length(_length), charge(_charge) { }         // FIX for ROOT macros
#endif                                                                                // FIX for ROOT macros
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
    auto segments = track->getSegments();
    std::sort(segments.begin(), segments.end(),
#ifdef __CINT__                                                                                        // FIX for ROOT macros
              [](const auto &a, const auto &b) {
#else                                                                                                  // FIX for ROOT macros
	      [](const decltype(*std::begin(segments)) &a, const decltype(*std::begin(segments)) &b) { // FIX for ROOT macros
#endif                                                                                                 // FIX for ROOT macros
                return a.getLength() > b.getLength();
              });
    /////// DEBUG
    //    std::cout<<__FUNCTION__<<": (track.front=="<<firstPID<<")=" << (segments.front().getPID() == firstPID)
    //	     << ", (track.back=="<<secondPID<<")=" << (segments.back().getPID() == secondPID)
    //	     << ", (track.Chi2<="<<chi2<<")=" << (track->getChi2() <= chi2)
    //	     << ", (track->HypothesisFitChi2<="<<hypothesisChi2<<")=" << (track->getHypothesisFitChi2() <= hypothesisChi2)
    //	     << ", (track.len>="<<length<<")=" << (track->getLength() >= length)
    //	     << ", (track.charge>"<<charge<<")=" << (track->getIntegratedCharge(track->getLength()) >= charge)
    //	     << ", track.charge=" << track->getIntegratedCharge(track->getLength())
    //	     << std::endl;
    /////// DEBUG
    return segments.front().getPID() == firstPID &&
           segments.back().getPID() == secondPID && track->getChi2() <= chi2 &&
           track->getHypothesisFitChi2() <= hypothesisChi2 &&
      track->getLength() >= length; //// DEBUG //&&
      //           track->getIntegratedCharge(track->getLength()) >= charge;
      ///// DEBUG
  }
};
using Cut6 = ReconstructionQuality2Prong;

// cut #7 : Additional selection cuts for 2-prong events to identify O-16 photo-dissociation candidates
// - tracks are sorted by length (longest first)
// - uncorrected length [mm] of the longest track must be within certain [min, max] range
// - uncorrected length [mm] of the shortest track must be within certain [min, max] range
// - optionally PIDs of the shortest and the longest track can be verifiesd as well (e.g. for automatic reconstruction)
struct ReconstructionLengthCorrelation2Prong {
#ifndef __CINT__                                                                         // FIX for ROOT macros
  ReconstructionLengthCorrelation2Prong(bool _enablePIDcheck, pid_type _firstPID,        // FIX for ROOT macros
					double _firstLengthMin, double _firstLengthMax,  // FIX for ROOT macros
					pid_type _secondPID, double _secondLengthMin,    // FIX for ROOT macros
					double _secondLengthMax)                         // FIX for ROOT macros
      : enablePIDcheck(_enablePIDcheck), firstPID(_firstPID),                            // FIX for ROOT macros
        firstLengthMin(_firstLengthMin), firstLengthMax(_firstLengthMax),                // FIX for ROOT macros
        secondPID(_secondPID), secondLengthMin(_secondLengthMin),                        // FIX for ROOT macros
        secondLengthMax(_secondLengthMax) { }                                            // FIX for ROOT macros
#endif                                                                                   // FIX for ROOT macros
  const bool enablePIDcheck;
  const pid_type firstPID; // not used unless enablePIDcheck=true
  const double firstLengthMin; // mm
  const double firstLengthMax; // mm
  const pid_type secondPID; // not used unless enablePIDcheck=true
  const double secondLengthMin; // mm
  const double secondLengthMax; // mm
  template <class Track> bool operator()(Track *track) {
    if (track->getSegments().size() != 2) {
      return true;
    }
    auto segments = track->getSegments();
    std::sort(segments.begin(), segments.end(),
#ifdef __CINT__                                                                                        // FIX for ROOT macros
              [](const auto &a, const auto &b) {
#else                                                                                                  // FIX for ROOT macros
	      [](const decltype(*std::begin(segments)) &a, const decltype(*std::begin(segments)) &b) { // FIX for ROOT macros
#endif                                                                                                 // FIX for ROOT macros
                return a.getLength() > b.getLength();
              });
    return (!enablePIDcheck ||
	    (enablePIDcheck &&
	     segments.front().getPID() == firstPID &&
	     segments.back().getPID() == secondPID)) &&
      segments.front().getLength()>firstLengthMin &&
      segments.front().getLength()<firstLengthMax &&
      segments.back().getLength()>secondLengthMin &&
      segments.back().getLength()<secondLengthMax;
  }
};

using Cut7 = ReconstructionLengthCorrelation2Prong;

} // namespace cuts
} // namespace tpcreco

#endif // TPCRECO_ANALYSIS_CUTS_H_
