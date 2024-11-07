#ifndef TPCRECO_ANALYSIS_CUTS_H_
#define TPCRECO_ANALYSIS_CUTS_H_
#include "TPCReco/CommonDefinitions.h"
#include <TGraph.h>
#include <TH2Poly.h>
#include <TVector3.h>
#include <algorithm>
#include <map>
#include <memory>
#include <cassert>
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
// NOTE: Initialization via constructor is necessary for ROOT macros
class VertexPosition {
public:
  VertexPosition(double beamOffset=0., double beamSlope=0., double beamDiameter=0.)
    : beamOffset(beamOffset), beamSlope(beamSlope), beamDiameter(beamDiameter) { }
  template <class Track> bool operator()(Track *track) {
    auto vertexPos = track->getSegments().front().getStart();
    return std::abs(vertexPos.Y() - (beamOffset + beamSlope * vertexPos.X())) <=
           0.5 * beamDiameter;
  }

private:
  double beamOffset = 0.; // mm
  double beamSlope = 0.;
  double beamDiameter = 0.; // mm
};
using Cut2 = VertexPosition;

// cut: XY plane : minimal distance to the border of UVW active area
// - less strict than simple XY rectangular cut, allows to gain some statistics
// NOTE: Initialization via constructor is necessary for ROOT macros
class DistanceToBorder {
public:
  template <class Geometry>
  DistanceToBorder(Geometry *geometry, double margin_mm) {
    auto *hull = new TGraph(geometry->GetActiveAreaConvexHull(margin_mm));
    bin = std::make_shared<TH2PolyBin>(hull, 1);
  }
  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
#ifdef __CINT__
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this](const auto &segment) {
#else
    return std::all_of(segments.cbegin(), segments.cend(),                   // FIX for ROOT macros
                       [this](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif
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
// NOTE: Initialization via constructor is necessary for ROOT macros
class RectangularCut {
public:
  RectangularCut(double minX, double maxX, double minY, double maxY)
      : minX(minX), maxX(maxX), minY(minY), maxY(maxY) { }
  template <class Track> bool operator()(Track *track) {
    const auto &segments = track->getSegments();
#ifdef __CINT__
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this](const auto &segment) {
#else
    return std::all_of(segments.cbegin(), segments.cend(),                   // FIX for ROOT macros
                       [this](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif
                         auto start = segment.getStart();
                         auto stop = segment.getEnd();
                         return start.X() < maxX && start.X() > minX &&
                                start.Y() < maxY && start.Y() > minY &&
                                stop.X() < maxX && stop.X() > minX &&
                                stop.Y() < maxY && stop.Y() > minY;
                       });
  }

private:
  double minX; // mm
  double maxX; // mm
  double minY; // mm
  double maxY; // mm
};
using Cut3a = RectangularCut;

// cut: global Z-span per event, verifies that:
// - vertical projection length is below physical drift cage length
// - tracks do not overlap with pedestal exclusion zone, begin of history
// buffer
// - tracks not too close to end of history buffer
// NOTE: Initialization via constructor is necessary for ROOT macros
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
  double lowerMargin; // mm
  double upperMargin; // mm
  double lengthLimit; // mm
};
using Cut4 = GlobalZSpan;

// cut #5 : Z-span wrt vertex per track per event, verifies that:
// - vertical distance of endpoint to vertex is less than half of drift cage
// height corrected for maximal vertical beam spread
// - ensures that 2,3-prong events hit neither the GEM plane nor the cathode
// plane NOTE: does not protect against 1-prong events (eg. background)
// originating from the GEM plane or the cathode plane
// NOTE: Initialization via constructor is necessary for ROOT macros
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
#ifdef __CINT__
    return std::all_of(std::cbegin(segments), std::cend(segments),
                       [this, vertexZ](const auto &segment) {
#else
    return std::all_of(segments.cbegin(), segments.cend(),                            // FIX for ROOT macros
                       [this, vertexZ](const decltype(*segments.cbegin()) &segment) { // FIX for ROOT macros
#endif
                         return std::abs(segment.getEnd().Z() - vertexZ) <=
                                0.5 * (driftCageLength - beamDiameter);
                       });
  }

private:
  double driftCageLength; // mm
  double beamDiameter; // mm
};
using Cut5 = VertexZSpan;

// cut #6 : Additional quality cuts for 2-prong events used by Artur for plots
// from automatic reconstruction that employs lustering + dE/dx method:
// - loss < 15
// - charge > 1000
// - length > 30 mm
// - eventType = 3
// - hypothesisLoss < 0.3
// NOTE: For manual reconstruction disable these dE/dx fit quality checks
// NOTE: Those cuts are currently impossible to apply to results from manual
// data reconstruction and to fake data generated by toy MC. If we are going
// to use results from automatic reconstruction for demonstration of cross
// section measurement then those cuts must be taken into account as well
// while correcting the rates!
// NOTE: Initialization via constructor is necessary for ROOT macros
class ReconstructionQuality2Prong {
public:
  ReconstructionQuality2Prong(pid_type firstPID, pid_type secondPID, double chi2,
			      double hypothesisChi2, double length, double charge)
      : firstPID(firstPID), secondPID(secondPID), chi2(chi2),
        hypothesisChi2(hypothesisChi2), length(length), charge(charge) { }
  template <class Track> bool operator()(Track *track) {
    if (track->getSegments().size() != 2) {
      return true;
    }
    auto segments = track->getSegments();
    std::sort(segments.begin(), segments.end(),
#ifdef __CINT__
              [](const auto &a, const auto &b) {
#else
	      [](const decltype(*std::begin(segments)) &a, const decltype(*std::begin(segments)) &b) { // FIX for ROOT macros
#endif
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
    std::cout<<" loss: "<<track->getLoss()
              <<" hypothesisLoss: "<<track->getHypothesisFitLoss()
              <<" charge: "<<track->getIntegratedCharge(track->getLength())
              << std::endl;

    return segments.front().getPID() == firstPID &&
           segments.back().getPID() == secondPID && track->getLoss() <= chi2 &&
           track->getHypothesisFitLoss() <= hypothesisChi2 &&
      track->getLength() >= length; //// DEBUG //&&
      //           track->getIntegratedCharge(track->getLength()) >= charge;
      ///// DEBUG
  }

private:
  pid_type firstPID;
  pid_type secondPID;
  double chi2;
  double hypothesisChi2;
  double length; // mm
  double charge; // ADC units, after pedestal subtraction
};
using Cut6 = ReconstructionQuality2Prong;

// cut #7 : Additional selection cuts for 2-prong events to identify O-16 photo-dissociation candidates
// - tracks are sorted by length (longest first)
// - uncorrected length [mm] of the longest track must be within certain [min, max] range
// - uncorrected length [mm] of the shortest track must be within certain [min, max] range
// - optionally PIDs of the shortest and the longest track can be verified as well (e.g. for automatic reconstruction)
// NOTE: Initialization via constructor is necessary for ROOT macros
class ReconstructionLengthCorrelation2Prong {
public:
  ReconstructionLengthCorrelation2Prong(bool enablePIDcheck, pid_type firstPID,
					double firstLengthMin, double firstLengthMax,
					pid_type secondPID, double secondLengthMin,
					double secondLengthMax)
      : enablePIDcheck(enablePIDcheck), firstPID(firstPID),
        firstLengthMin(firstLengthMin), firstLengthMax(firstLengthMax),
        secondPID(secondPID), secondLengthMin(secondLengthMin),
        secondLengthMax(secondLengthMax) {
    assert(firstLengthMin<firstLengthMax && secondLengthMin<secondLengthMax);
  }
  template <class Track> bool operator()(Track *track) {
    if (track->getSegments().size() != 2) {
      return true;
    }
    auto segments = track->getSegments();
    std::sort(segments.begin(), segments.end(),
#ifdef __CINT__
              [](const auto &a, const auto &b) {
#else
	      [](const decltype(*std::begin(segments)) &a, const decltype(*std::begin(segments)) &b) { // FIX for ROOT macros
#endif
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

private:
  bool enablePIDcheck;
  pid_type firstPID; // not used unless enablePIDcheck=true
  double firstLengthMin; // mm
  double firstLengthMax; // mm
  pid_type secondPID; // not used unless enablePIDcheck=true
  double secondLengthMin; // mm
  double secondLengthMax; // mm
};

using Cut7 = ReconstructionLengthCorrelation2Prong;

// cut #8 : Additional selection cuts to identify multiploarity (e.g. E1,E2 polar angle distributions) of the leading particle track
// in the LAB reference frame:
// - tracks are sorted by length (longest first)
// - for the leading track {X0,Y0} pair is calculated, where: X0=cos(theta_BEAM), Y0=uncorrected length [mm]
// - such {X0,Y0} point must fulfill the condition:
//     Y_MIN(X0) <= Y0 <= Y_MAX(X0)
//   where the lower and upper limits of the inclined 2D band are defined, respectively, as:
//     Y_MIN(X) = SLOPE[mm] * X + OFFSET_MIN[mm]
//     Y_MAX(X) = SLOPE[mm] * X + OFFSET_MAX[mm]
// - optionally PID of the longest track can be verified as well (e.g. for automatic reconstruction).
// NOTE: Since cos(theta_BEAM) is unitless and has the range of [-1, 1] the leftmost and rightmost limits of accepted particle lengths
//       are [OFFSET_MIN-SLOPE, OFFSET_MAX-SLOPE] and [OFFSET_MIN+SLOPE, OFFSET_MAX+SLOPE], respectively.
// NOTE: Initialization via constructor is necessary for ROOT macros
//
class ReconstructionMultipolarityOfLeadingTrack {
public:
  ReconstructionMultipolarityOfLeadingTrack(bool enablePIDcheck, pid_type PID,
					    double bandOffsetMin, double bandOffsetMax, double bandSlope,
					    TVector3 beamDir_DET)
    : enablePIDcheck(enablePIDcheck), PID(PID),
      bandOffsetMin(bandOffsetMin), bandOffsetMax(bandOffsetMax), bandSlope(bandSlope),
      beamDir_DET(beamDir_DET.Unit()) {
    assert(bandOffsetMin<bandOffsetMax);
  }
  template <class Track> bool operator()(Track *track) {
    if (!track->getSegments().size()) {
      return false;
    }
    auto segments = track->getSegments();
    std::sort(segments.begin(), segments.end(),
#ifdef __CINT__
              [](const auto &a, const auto &b) {
#else
	      [](const decltype(*std::begin(segments)) &a, const decltype(*std::begin(segments)) &b) { // FIX for ROOT macros
#endif
                return a.getLength() > b.getLength();
              });
    auto cosTheta_BEAM = segments.front().getTangent() * beamDir_DET; // cos of polar angle wrt beam axis
    auto length = segments.front().getLength();
    return (!enablePIDcheck ||
	    (enablePIDcheck &&
	     segments.front().getPID() == PID )) &&
	    length >= bandSlope * cosTheta_BEAM + bandOffsetMin &&
	    length <= bandSlope * cosTheta_BEAM + bandOffsetMax;
  }

private:
  bool enablePIDcheck;
  pid_type PID; // not used unless enablePIDcheck=true
  double bandOffsetMin; // mm - offset of the allowed lower band (length at cos(theta_BEAM)=0)
  double bandOffsetMax; // mm - offset of the allowed upper band (length at cos(theta_BEAM)=0)
  double bandSlope; // mm / unit of cos(theta_BEAM) - slope of the allowed band
  TVector3 beamDir_DET; // actual beam unit vector in DET coordinates (i.e. direction of photons)
};

using Cut8 = ReconstructionMultipolarityOfLeadingTrack;

// ReconstructionNumberOfTracks:
// Cut that accepts only events with exact number of tracks (constructor with 1 argument)
// or events with number of tracks within specific [MIN, MAX] range (constructor with 2 arguments).
//
class ReconstructionNumberOfTracks {
public:
  ReconstructionNumberOfTracks(int minTrackNumber, int maxTrackNumber)
    : minTrackNumber(minTrackNumber), maxTrackNumber(maxTrackNumber) {
    assert(minTrackNumber<=maxTrackNumber);
  }
  ReconstructionNumberOfTracks(int exactTrackNumber)
    : minTrackNumber(exactTrackNumber), maxTrackNumber(exactTrackNumber) { }
  template <class Track> bool operator()(Track *track) {
    return track->getSegments().size() >= minTrackNumber &&
           track->getSegments().size() <= maxTrackNumber;
  }

private:
  int minTrackNumber;
  int maxTrackNumber;
};

using Cut9 = ReconstructionNumberOfTracks;

} // namespace cuts
} // namespace tpcreco

#endif // TPCRECO_ANALYSIS_CUTS_H_
