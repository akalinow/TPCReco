#ifndef TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_
#define TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_

#include <array>
#include <memory>
#include <optional>
#include <vector>

class GeometryTPC;

/// State machine of the manual ("clicked") track reconstruction.
///
/// GUI independent port of the MarkersManager logic of tpcGUI:
///  - "Add segment" arms the tool,
///  - a click on one of the U/V/W strip vs time projections sets the time (x)
///    and the strip coordinate (y) in that projection,
///  - a click on a second projection uses the time of the first click and the new
///    strip coordinate; the coordinate in the third projection is calculated from
///    the detector geometry,
///  - the first point defines the common vertex, the following points the segment ends,
///  - "Fit segments" returns the segment end points in the format expected by
///    TrackBuilder::getSegment2DCollectionFromGUI().
class ManualRecoTool {

public:

  struct Point { double x, y; };

  /// 2D segment in one strip direction. x2,y2 are NaN while the segment is incomplete.
  struct Segment { double x1, y1, x2, y2; int color; };

  /// vertical helper line (x position, ROOT colour index)
  struct VerticalLine { double x; int color; };

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometry) { myGeometry = aGeometry; }

  /// Clear markers and segments; buttons as after loading an event.
  void reset(bool isRecoModeOn);

  /// "Add segment" button
  void addSegment();

  /// Click at (x,y) on the projection of strip direction iDir (0-2). Returns false when ignored.
  bool click(int iDir, double x, double y);

  /// "Fit segments" button: returns the segments end points (for each segment, for each
  /// direction: x1, y1, x2, y2) and resets the tool.
  std::vector<double> takeSegmentsForFit();

  const std::array<std::vector<Segment>, 3> & getSegments() const { return mySegments; }
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

private:

  void resetMarkers(bool force);
  void updateSegments(int iDir);
  bool isLastSegmentComplete(int iDir) const;
  int findMissingMarkerDir() const;
  double getMissingYCoordinate(int missingDir) const;

  std::shared_ptr<GeometryTPC> myGeometry;

  std::array<std::optional<Point>, 3> myMarkers;
  std::optional<Point> myFirstMarker;
  std::array<std::vector<Segment>, 3> mySegments;
  std::vector<VerticalLine> myTimeLines;

  bool acceptPoints{false};
  bool addEnabled{false}, fitEnabled{false}, clearEnabled{false};
};

#endif // TPCRECO_WEBSERVERCLIENT_MANUAL_RECO_TOOL_H_
