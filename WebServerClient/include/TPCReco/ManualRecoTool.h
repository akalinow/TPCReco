#ifndef TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_
#define TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class GeometryTPC;

/// Manual ("clicked") event reconstruction: common part of the reconstruction modes.
///
/// GUI independent port of the MarkersManager logic of tpcGUI. Points are clicked in the same way
/// in all modes:
///  - "Add segment" / "Add track" arms the tool,
///  - a click on one of the U/V/W strip vs time projections sets the time (x)
///    and the strip coordinate (y) in that projection,
///  - a click on a second projection uses the time of the first click and the new
///    strip coordinate; the coordinate in the third projection is calculated from
///    the detector geometry: the point is then known in all three projections.
/// The clicked points are kept in click order (getPoints()) and can be moved (movePoint()),
/// also after "Fit segments": the segments are then fitted again.
///
/// What the clicked points mean depends on the mode (derived classes):
///  - TwoProngRecoTool:   the first point is the common vertex, the following points are segment ends,
///  - ThreeProngRecoTool: two points on each track; the vertex is the intersection of the track lines.
///
/// "Fit segments" returns the segment end points in the format expected by
/// TrackBuilder::getSegment2DCollectionFromGUI().
class ManualRecoTool {

public:

  enum class Mode { TwoProng, ThreeProng };

  struct Point { double x, y; };

  /// one 3D point: (time, strip coordinate) in the U, V and W projections
  using Point3P = std::array<Point, 3>;

  /// clicked point: coordinates in all projections and the two projections set by the user
  /// (the third one is calculated)
  struct ClickedPoint { Point3P p; std::array<int, 2> clickedDirs; };

  /// 2D segment in one strip direction. x2,y2 are NaN while the segment is incomplete.
  struct Segment { double x1, y1, x2, y2; int color; };

  /// vertical helper line (x position, ROOT colour index)
  struct VerticalLine { double x; int color; };

  /// tool of the given mode
  static std::unique_ptr<ManualRecoTool> create(Mode aMode);
  /// "2prong" / "3prong"
  static std::string modeName(Mode aMode);
  static std::optional<Mode> modeFromName(const std::string & name);

  virtual ~ManualRecoTool() = default;

  virtual Mode getMode() const = 0;

  /// label of the button that arms the tool ("Add segment", "Add track")
  virtual std::string getAddLabel() const = 0;

  /// what the user should do now / result of the clicks so far (one line)
  virtual std::string getHint() const = 0;

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometry) { myGeometry = aGeometry; }

  /// Clear the points; buttons as after loading an event.
  void reset(bool isRecoModeOn);

  /// Points taken from a reconstructed track, shown as if they were clicked (clicked
  /// projections: U and V). They can be moved and more points can be added; "Fit segments" is
  /// enabled after such a change.
  void setPoints(const std::vector<Point3P> & points);

  /// "Add segment" / "Add track" button
  void addSegment();

  /// Click at (x,y) on the projection of strip direction iDir (0-2). Returns false when ignored.
  bool click(int iDir, double x, double y);

  /// Move the clicked point index to (x,y) in the projection iDir: the time of the point is x
  /// in all projections, its strip coordinate in iDir is y; the strip coordinate of the other
  /// projection set by the user is kept, the third one is calculated. Returns false when ignored
  /// (no such point, or a point is being clicked).
  bool movePoint(size_t index, int iDir, double x, double y);

  /// "Fit segments" button: returns the segments end points (for each segment, for each
  /// direction: x1, y1, x2, y2). The points are kept (they can be moved and fitted again).
  std::vector<double> takeSegmentsForFit();

  /// segments to draw, per strip direction
  virtual std::array<std::vector<Segment>, 3> getSegments() const = 0;
  /// ROOT colour index of the clicked point index
  virtual int getPointColor(size_t index) const = 0;
  /// calculated vertex in the projection iDir, if any
  virtual std::optional<Point> getVertex(int /*iDir*/) const { return std::nullopt; }

  /// clicked points, in click order
  const std::vector<ClickedPoint> & getPoints() const { return myPoints; }
  /// markers of the point being clicked
  const std::array<std::optional<Point>, 3> & getMarkers() const { return myMarkers; }
  /// time of the first click of the current point, if any
  std::optional<double> getFixedTime() const;
  /// lines marking the clicked times on the charge vs time panel
  const std::vector<VerticalLine> & getTimeLines() const { return myTimeLines; }
  /// colour of the line following the mouse on the charge vs time panel
  int getCursorLineColor() const { return 1 + myTimeLines.size(); }

  /// true when points have been clicked or moved but not yet sent to the fit
  bool hasClickedPoints() const;
  /// clicked points can be moved now
  bool canMovePoints() const { return isEnabled && !myFirstMarker && !myPoints.empty(); }

  bool isAcceptingPoints() const { return acceptPoints; }
  bool isAddEnabled() const { return isEnabled && !acceptPoints; }
  bool isFitEnabled() const { return isEnabled && !acceptPoints && isChanged && canFit(); }
  bool isClearEnabled() const { return isEnabled; }

protected:

  /// mode specific cached results (e.g. the vertex): recalculate after the points changed
  virtual void pointsChanged() {}
  /// after a point was added: true when the current segment / track needs more points
  virtual bool needsMorePoints() const = 0;
  /// "Fit segments" possible with the points clicked so far
  virtual bool canFit() const = 0;
  /// segment end points for TrackBuilder::getSegment2DCollectionFromGUI()
  virtual std::vector<double> makeSegmentsForFit() const = 0;

  std::shared_ptr<GeometryTPC> myGeometry;

private:

  /// strip coordinate in missingDir of the point with strip coordinates posA, posB in dirA, dirB
  double getMissingYCoordinate(int dirA, double posA, int dirB, double posB, int missingDir) const;

  std::vector<ClickedPoint> myPoints;
  std::array<std::optional<Point>, 3> myMarkers;
  std::optional<Point> myFirstMarker;
  int myFirstDir{-1};
  std::vector<VerticalLine> myTimeLines; // one per point (and one for the point being clicked)

  bool isEnabled{false};    // manual reconstruction mode on
  bool acceptPoints{false}; // armed by "Add segment"
  bool isChanged{false};    // points added or moved since the last fit
};

#endif // TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_
