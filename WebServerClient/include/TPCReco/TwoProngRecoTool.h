#ifndef TPCRECO_WEBSERVERCLIENT_TWO_PRONG_RECO_TOOL_H_
#define TPCRECO_WEBSERVERCLIENT_TWO_PRONG_RECO_TOOL_H_

#include "TPCReco/ManualRecoTool.h"

/// Manual reconstruction, 2-prong mode (the tpcGUI one): the first clicked point is the common
/// vertex, every following point is the end of a segment starting at the vertex.
class TwoProngRecoTool : public ManualRecoTool {

public:

  Mode getMode() const override { return Mode::TwoProng; }
  std::string getAddLabel() const override { return "Add segment"; }
  std::string getHint() const override;

  std::array<std::vector<Segment>, 3> getSegments() const override { return mySegments; }

protected:

  void clearPoints() override;
  bool addPoint(const Point3P & aPoint) override;
  bool hasPoints() const override;
  bool canFit() const override;
  std::vector<double> makeSegmentsForFit() const override;

private:

  bool isLastSegmentComplete(int iDir) const;

  std::array<std::vector<Segment>, 3> mySegments;
};

#endif // TPCRECO_WEBSERVERCLIENT_TWO_PRONG_RECO_TOOL_H_
