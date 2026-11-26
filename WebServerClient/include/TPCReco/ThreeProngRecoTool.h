#ifndef TPCRECO_WEBSERVERCLIENT_THREE_PRONG_RECO_TOOL_H_
#define TPCRECO_WEBSERVERCLIENT_THREE_PRONG_RECO_TOOL_H_

#include "TPCReco/ManualRecoTool.h"

/// Manual reconstruction, 3-prong mode: the user clicks two points on each track (each point on
/// two projections, as in the 2-prong mode), the vertex is not clicked. In every U/V/W projection
/// each track defines a line through its two points; the vertex is the point where these lines
/// cross. It is found by a least squares fit of one 3D vertex (time, X, Y) to the lines of all
/// tracks in all three projections, so its projections are consistent with each other.
/// Each segment goes from the vertex to the clicked point of its track farther from the vertex.
/// The fit needs at least two tracks (three for a 3-prong event).
class ThreeProngRecoTool : public ManualRecoTool {

public:

  /// fitted vertex: time coordinate t [mm], position on the readout plane X, Y [mm], and
  /// RMS distance of the vertex projections from the track lines [mm]
  struct Vertex { double t, X, Y, rms; int nLines; };

  /// strip pitch unit vectors of U, V, W: strip coordinate = X*p[dir][0] + Y*p[dir][1]
  using PitchVectors = std::array<std::array<double, 2>, 3>;

  /// two clicked points of a track
  using TrackPoints = std::array<Point3P, 2>;

  /// least squares vertex of the track lines (at least two tracks); no value when the lines
  /// do not determine a point (e.g. parallel tracks)
  static std::optional<Vertex> fitVertex(const std::vector<TrackPoints> & tracks,
                                         const PitchVectors & pitch);

  Mode getMode() const override { return Mode::ThreeProng; }
  std::string getAddLabel() const override { return "Add track"; }
  std::string getHint() const override;

  std::array<std::vector<Segment>, 3> getSegments() const override;
  std::vector<ColoredPoint> getClickedPoints(int iDir) const override;
  std::optional<Point> getVertex(int iDir) const override;

protected:

  void clearPoints() override;
  void onAdd() override;
  bool addPoint(const Point3P & aPoint) override;
  bool hasPoints() const override;
  bool canFit() const override;
  std::vector<double> makeSegmentsForFit() const override;

private:

  std::vector<TrackPoints> completeTracks() const;
  /// vertex projection on the strip direction iDir
  Point vertexProjection(int iDir) const;
  /// index (0, 1) of the clicked point of a track farther from the vertex
  int farPointIndex(const TrackPoints & aTrack) const;
  void updateVertex();

  std::vector<std::vector<Point3P>> myTracks; // clicked points of each track (up to 2)
  PitchVectors myPitch{};
  std::optional<Vertex> myVertex;
};

#endif // TPCRECO_WEBSERVERCLIENT_THREE_PRONG_RECO_TOOL_H_
