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

  /// one clicked 3D point: (time, strip coordinate) in the U, V and W projections
  using Point3P = std::array<Point, 3>;

  /// 2D segment in one strip direction. x2,y2 are NaN while the segment is incomplete.
  struct Segment { double x1, y1, x2, y2; int color; };

  /// clicked point drawn in a projection (ROOT colour index)
  struct ColoredPoint { double x, y; int color; };

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

  /// Clear markers and segments; buttons as after loading an event.
  void reset(bool isRecoModeOn);

  /// "Add segment" / "Add track" button
  void addSegment();

  /// Click at (x,y) on the projection of strip direction iDir (0-2). Returns false when ignored.
  bool click(int iDir, double x, double y);

  /// "Fit segments" button: returns the segments end points (for each segment, for each
  /// direction: x1, y1, x2, y2) and resets the tool.
  std::vector<double> takeSegmentsForFit();

  /// segments to draw, per strip direction
  virtual std::array<std::vector<Segment>, 3> getSegments() const = 0;
  /// clicked points to draw in the projection iDir (besides the segment ends)
  virtual std::vector<ColoredPoint> getClickedPoints(int /*iDir*/) const { return {}; }
  /// calculated vertex in the projection iDir, if any
  virtual std::optional<Point> getVertex(int /*iDir*/) const { return std::nullopt; }

  /// markers of the point being clicked
  const std::array<std::optional<Point>, 3> & getMarkers() const { return myMarkers; }
  /// time of the first click of the current point, if any
  std::optional<double> getFixedTime() const;
  /// lines marking the clicked times on the charge vs time panel
  const std::vector<VerticalLine> & getTimeLines() const { return myTimeLines; }
  /// colour of the line following the mouse on the charge vs time panel
  int getCursorLineColor() const { return 1 + myTimeLines.size(); }

  /// true when points or segments have been clicked but not yet sent to the fit
  bool hasClickedPoints() const;

  bool isAcceptingPoints() const { return acceptPoints; }
  bool isAddEnabled() const { return addEnabled; }
  bool isFitEnabled() const { return fitEnabled; }
  bool isClearEnabled() const { return clearEnabled; }

protected:

  /// mode specific state: clear it
  virtual void clearPoints() = 0;
  /// "Add" button pressed
  virtual void onAdd() {}
  /// a point is complete (known in all three projections). Returns true when the tool should
  /// keep accepting points (the current segment / track needs more points).
  virtual bool addPoint(const Point3P & aPoint) = 0;
  /// clicked points kept by the mode
  virtual bool hasPoints() const = 0;
  /// "Fit segments" possible with the points clicked so far
  virtual bool canFit() const = 0;
  /// segment end points for TrackBuilder::getSegment2DCollectionFromGUI()
  virtual std::vector<double> makeSegmentsForFit() const = 0;

  std::shared_ptr<GeometryTPC> myGeometry;

private:

  void resetMarkers();
  int findMissingMarkerDir() const;
  double getMissingYCoordinate(int missingDir) const;

  std::array<std::optional<Point>, 3> myMarkers;
  std::optional<Point> myFirstMarker;
  std::vector<VerticalLine> myTimeLines;

  bool acceptPoints{false};
  bool addEnabled{false}, fitEnabled{false}, clearEnabled{false};
};

#endif // TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_
