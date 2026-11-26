#ifndef TPCRECO_RECONSTRUCTION_CHARGE_PROFILE_BUILDER_H_
#define TPCRECO_RECONSTRUCTION_CHARGE_PROFILE_BUILDER_H_

#include <vector>

#include <TH1F.h>

class Track3D;
class TrackSegment3D;

/// Charge profile along the track for the current track parameters, independent of the dE/dx fit.
///
/// Every segment gives the charge per mm along it, from its start (the vertex) to its end, from
/// the rec hits close to its U/V/W projections. The segment profiles are aligned by their end
/// points: a segment of length L is shifted by Lmax - L, so all segment ends are at x = Lmax (the
/// length of the longest segment) and the Bragg peaks line up. The track profile is the stack
/// (sum) of the segment profiles, in charge/mm (not normalised).
///
/// Filled for the initial track (after the 3D track is built in the automatic reconstruction, after
/// the fit of the clicked segments in the manual one) and filled again after the dE/dx fit has
/// improved the track.
class ChargeProfileBuilder {

public:

  struct Settings {
    double radiusCut{4.0};        // [mm] maximum hit distance from a segment 2D projection
    double binWidth{2.0};         // [mm]
    double minProjFraction{0.25}; // projections shorter than this fraction of the segment are skipped
    double minProjLength{1.0};    // [mm] ... and shorter than this
    double endMargin{0.2};        // hits up to this fraction of the segment length beyond its end (none behind the vertex)
  };

  ChargeProfileBuilder() = default;
  explicit ChargeProfileBuilder(const Settings & aSettings) : mySettings(aSettings) {}

  /// profiles of the segments (charge/mm), aligned by the segment end points, common binning
  std::vector<TH1F> getSegmentProfiles(const Track3D & aTrack) const;

  /// profile of one segment (charge/mm) from its start (the vertex), range [-0.2, 1.2]*length
  TH1F getSegmentProfile(const TrackSegment3D & aSegment) const;

  /// stacked profile of the track: sum of the segment profiles
  TH1F getTrackProfile(const Track3D & aTrack) const;

  /// sets the track profile (Track3D::setChargeProfile)
  void fill(Track3D & aTrack) const;

private:

  /// x range of the profiles: [-0.2, 1.2]*Lmax
  TH1F makeHisto(double maxLength) const;
  void addSegment(TH1F & histo, const TrackSegment3D & aSegment, double shift) const;

  Settings mySettings;
};

#endif // TPCRECO_RECONSTRUCTION_CHARGE_PROFILE_BUILDER_H_
