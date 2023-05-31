#include "TPCReco/Cuts.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <TVector3.h>
#include <vector>
using namespace tpcreco::cuts;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

struct GeometryTPCMock {
  MOCK_METHOD(double, GetDriftCageZmin, (), (const));
  MOCK_METHOD(double, GetDriftCageZmax, (), (const));
  MOCK_METHOD(int, GetAgetNtimecells, (), (const));
  MOCK_METHOD(double, Timecell2pos, (double, bool &), (const));
  MOCK_METHOD(TGraph, GetActiveAreaConvexHull, (double), (const));
};

struct TrackSegment3DMock {
  MOCK_METHOD(TVector3, getStart, (), (const));
  MOCK_METHOD(TVector3, getEnd, (), (const));
  MOCK_METHOD(pid_type, getPID, (), (const));
  MOCK_METHOD(double, getLength, (), (const));
  TrackSegment3DMock() = default;
  TrackSegment3DMock(const TrackSegment3DMock &other) {}
  TrackSegment3DMock &operator=(const TrackSegment3DMock &other) {
    return *this;
  }
};

struct Track3DMock {
  MOCK_METHOD(std::vector<TrackSegment3DMock> &, getSegments, (), (const));
  MOCK_METHOD(double, getChi2, (), (const));
  MOCK_METHOD(double, getHypothesisFitChi2, (), (const));
  MOCK_METHOD(double, getLength, (), (const));
  MOCK_METHOD(double, getIntegratedCharge, (double), (const));
};

class CutsTest : public ::testing::Test {
public:
  std::vector<TrackSegment3DMock> segments;
  Track3DMock track;

  void SetUp() override {
    EXPECT_CALL(track, getSegments()).WillRepeatedly(ReturnRef(segments));
  }
};

class Cut1Test : public CutsTest {};

TEST_F(Cut1Test, EmptyTrack) {
  Cut1 cut;
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut1Test, NonEmptyTrack) {
  segments.push_back({});
  Cut1 cut{};
  EXPECT_TRUE(cut(&track));
}

class Cut2Test : public CutsTest {};

TEST_F(Cut2Test, beamNoTilt) {
  segments.push_back({});
  double beamRadius = 5.;
  double beamOffset = 100;
  Cut2 cut{beamOffset, 0, 2 * beamRadius};
  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{0, 100, 0}));
  EXPECT_TRUE(cut(&track));
  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{0, 102, 0}));
  EXPECT_TRUE(cut(&track));
  // off beam
  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{0, 106, 0}));
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut2Test, beamTilt) {
  segments.push_back({});
  double beamRadius = 5.;
  double beamOffset = 10.;
  double beamSlope = 0.2;
  Cut2 cut{beamOffset, beamSlope, 2 * beamRadius};
  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_FALSE(cut(&track));

  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{1, 10.1, 0}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{1, 15.1, 0}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getStart())
      .Times(1)
      .WillRepeatedly(Return(TVector3{1, 15.4, 0}));
  EXPECT_FALSE(cut(&track));
}

class Cut3Test : public CutsTest {
public:
  double margin_mm;
  TGraph graph;
  GeometryTPCMock geometry;
  void SetUp() override {
    CutsTest::SetUp();
    double x[] = {0, 1, 1, 0, 0};
    double y[] = {0, 0, 1, 1, 0};
    graph = TGraph(5, x, y);
    EXPECT_CALL(geometry, GetActiveAreaConvexHull(_))
        .WillRepeatedly(Return(graph));
  }
};

TEST_F(Cut3Test, InsideConvexHull) {
  Cut3 cut{&geometry, margin_mm};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0.3, 0.3, 0}));
  EXPECT_CALL(segments[0], getEnd())
      .WillRepeatedly(Return(TVector3{0.4, 0.4, 0}));
  EXPECT_TRUE(cut(&track));
}

TEST_F(Cut3Test, OutsideConvexHull) {
  Cut3 cut{&geometry, margin_mm};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{2, 0.3, 0}));
  EXPECT_CALL(segments[0], getEnd())
      .WillRepeatedly(Return(TVector3{0.4, -0.4, 0}));
  EXPECT_FALSE(cut(&track));
}

class Cut3aTest : public CutsTest {
public:
  Cut3a cut{-10, 20, -5, 5};
};

TEST_F(Cut3aTest, AllOutside) {
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{-20, -20, 0}));
  EXPECT_CALL(segments[0], getEnd())
      .WillRepeatedly(Return(TVector3{-20, -20, 0}));
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut3aTest, StopOutside) {
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_CALL(segments[0], getEnd())
      .WillRepeatedly(Return(TVector3{-20, -20, 0}));
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut3aTest, AllInside) {
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_TRUE(cut(&track));
}

class Cut4Test : public CutsTest {
public:
  GeometryTPCMock geometry;
  double lowerMargin = 25;
  double upperMargin = 5;
  void SetUp() override {
    CutsTest::SetUp();
    EXPECT_CALL(geometry, GetDriftCageZmin()).WillRepeatedly(Return(-50));
    EXPECT_CALL(geometry, GetDriftCageZmax()).WillRepeatedly(Return(50));
    EXPECT_CALL(geometry, GetAgetNtimecells()).WillRepeatedly(Return(512));
    EXPECT_CALL(geometry, Timecell2pos(_, _))
        .WillOnce(Return(-100))
        .WillOnce(Return(100));
  }
};

TEST_F(Cut4Test, SingleSegmentInside) {

  Cut4 cut{&geometry, lowerMargin, upperMargin};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 80}));
  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 90}));
  EXPECT_TRUE(cut(&track));
}

TEST_F(Cut4Test, SingleSegmentOutsideMargin) {
  Cut4 cut{&geometry, lowerMargin, upperMargin};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 80}));
  EXPECT_CALL(segments[0], getEnd())
      .WillRepeatedly(Return(TVector3{0, 0, 110}));
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut4Test, SingleSegmentOutsideDriftCageLimit) {
  Cut4 cut{&geometry, lowerMargin, upperMargin};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, -90}));
  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 90}));
  EXPECT_FALSE(cut(&track));
}

class Cut5Test : public CutsTest {
public:
  GeometryTPCMock geometry;
  double beamDiameter = 10;
  void SetUp() override {
    CutsTest::SetUp();
    EXPECT_CALL(geometry, GetDriftCageZmin()).WillRepeatedly(Return(-20));
    EXPECT_CALL(geometry, GetDriftCageZmax()).WillRepeatedly(Return(20));
  }
};

TEST_F(Cut5Test, VertexInCenter) {
  Cut5 cut{&geometry, beamDiameter};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 12}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 20}));
  EXPECT_FALSE(cut(&track));
}

TEST_F(Cut5Test, VertexOffCenter) {
  Cut5 cut{&geometry, beamDiameter};
  segments.push_back({});
  EXPECT_CALL(segments[0], getStart())
      .WillRepeatedly(Return(TVector3{0, 0, 10}));
  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 0}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 22}));
  EXPECT_TRUE(cut(&track));

  EXPECT_CALL(segments[0], getEnd()).WillRepeatedly(Return(TVector3{0, 0, 27}));
  EXPECT_FALSE(cut(&track));
}

// Mock expectations broken by sorting 
// class Cut6Test : public CutsTest {
// public:
//   pid_type firstPID = ALPHA;
//   pid_type secondPID = CARBON_12;
//   double chi2 = 4;
//   double hypothesisChi2 = 10;
//   double length = 10;
//   double charge = 100;
//   Cut6 cut{firstPID, secondPID, chi2, hypothesisChi2, length, charge};
// };
//
// TEST_F(Cut6Test, SkipNon2ProngEvents) {
//   EXPECT_TRUE(cut(&track));
//   segments.push_back({});
//   EXPECT_TRUE(cut(&track));
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_TRUE(cut(&track));
// }
//
// TEST_F(Cut6Test, passingQuality) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(ALPHA));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(20));
//   EXPECT_CALL(track, getIntegratedCharge(20)).WillRepeatedly(Return(200));
//   EXPECT_TRUE(cut(&track));
// }
//
// TEST_F(Cut6Test, WrongPID) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(UNKNOWN));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(20));
//   EXPECT_CALL(track, getIntegratedCharge(20)).WillRepeatedly(Return(200));
//   EXPECT_FALSE(cut(&track));
// }
//
// TEST_F(Cut6Test, WrongChi2) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(UNKNOWN));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(100));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(20));
//   EXPECT_CALL(track, getIntegratedCharge(20)).WillRepeatedly(Return(200));
//   EXPECT_FALSE(cut(&track));
// }
//
// TEST_F(Cut6Test, WrongHipothesisChi2) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(UNKNOWN));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(100));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(20));
//   EXPECT_CALL(track, getIntegratedCharge(20)).WillRepeatedly(Return(200));
//   EXPECT_FALSE(cut(&track));
// }
//
// TEST_F(Cut6Test, WrongLength) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(UNKNOWN));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(track, getIntegratedCharge(2)).WillRepeatedly(Return(200));
//   EXPECT_FALSE(cut(&track));
// }
//
// TEST_F(Cut6Test, WrongCharge) {
//   segments.push_back({});
//   segments.push_back({});
//   EXPECT_CALL(segments[0], getLength()).WillRepeatedly(Return(2));
//   EXPECT_CALL(segments[1], getLength()).WillRepeatedly(Return(1));
//   EXPECT_CALL(segments[0], getPID()).WillRepeatedly(Return(UNKNOWN));
//   EXPECT_CALL(segments[1], getPID()).WillRepeatedly(Return(CARBON_12));
//   EXPECT_CALL(track, getChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getHypothesisFitChi2()).WillRepeatedly(Return(1));
//   EXPECT_CALL(track, getLength()).WillRepeatedly(Return(20));
//   EXPECT_CALL(track, getIntegratedCharge(20)).WillRepeatedly(Return(1));
//   EXPECT_FALSE(cut(&track));
// }