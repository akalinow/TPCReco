#include "TPCReco/CutsFactory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

using namespace tpcreco::cuts;
using ::testing::InSequence;
using ::testing::StrictMock;

struct CutsMock : public CutsInterface {
  MOCK_METHOD(CutType, makeNonEmpty, (), (const, override));
  MOCK_METHOD(CutType, makeVertexPosition, (double, double, double),
              (const, override));
  MOCK_METHOD(CutType, makeDistanceToBorder, (const GeometryTPC *, double),
              (const, override));
  MOCK_METHOD(CutType, makeGlobalZSpan, (const GeometryTPC *, double, double),
              (const, override));
  MOCK_METHOD(CutType, makeVertexZSpan, (const GeometryTPC *, double),
              (const, override));
  MOCK_METHOD(CutType, makeRectangularCut, (double, double, double, double),
              (const, override));
};

class CutsFactoryTest : public ::testing::Test {
public:
  boost::property_tree::ptree tree;
  std::unique_ptr<CutsMock> imp = std::make_unique<CutsMock>();
};

TEST_F(CutsFactoryTest, NonEmpty) {
  std::stringstream ss(R"(
    {
        "type": "NonEmpty"
    }
  )");
  boost::property_tree::read_json(ss, tree);
  EXPECT_CALL(*imp, makeNonEmpty()).Times(1);
  CutsFactory cutsMaker(std::move(imp));
  cutsMaker.create(nullptr, tree);
}

TEST_F(CutsFactoryTest, VertexPosition) {
  std::stringstream ss(R"(
    {
        "type": "VertexPosition",
        "beam_offset_mm": -1.3,
        "beam_slope_rad": 3.0E-3,
        "beam_diameter_mm": 12.0
    }
  )");
  boost::property_tree::read_json(ss, tree);
  EXPECT_CALL(*imp, makeVertexPosition(-1.3, 3.0E-3, 12.0)).Times(1);
  CutsFactory cutsMaker(std::move(imp));
  cutsMaker.create(nullptr, tree);
}

TEST_F(CutsFactoryTest, DistanceToBorder) {
  std::stringstream ss(R"(
    {
        "type": "DistanceToBorder",
        "margin_mm": 5
    }
  )");
  boost::property_tree::read_json(ss, tree);
  EXPECT_CALL(*imp, makeDistanceToBorder(nullptr, 5)).Times(1);
  CutsFactory cutsMaker(std::move(imp));
  cutsMaker.create(nullptr, tree);
}

TEST_F(CutsFactoryTest, GlobalZSpan) {
  std::stringstream ss(R"(
    {
        "type": "GlobalZSpan",
        "lower_margin_timecells": 25,
        "upper_margin_timecells": 5
    }
  )");
  boost::property_tree::read_json(ss, tree);
  EXPECT_CALL(*imp, makeGlobalZSpan(nullptr, 25, 5)).Times(1);
  CutsFactory cutsMaker(std::move(imp));
  cutsMaker.create(nullptr, tree);
}

TEST_F(CutsFactoryTest, VertexZSpan) {
  std::stringstream ss(R"(
    {
        "type": "VertexZSpan",
        "beam_diameter_mm": 12.0
    }
  )");
  boost::property_tree::read_json(ss, tree);
  EXPECT_CALL(*imp, makeVertexZSpan(nullptr, 12)).Times(1);
  CutsFactory cutsMaker(std::move(imp));
  cutsMaker.create(nullptr, tree);
}

TEST_F(CutsFactoryTest, multiple) {
  InSequence sequnce;
  std::stringstream ss(R"(
    {
    "cuts": [
        {
            "type": "NonEmpty"
        },
        {
            "type": "VertexPosition",
            "beam_offset_mm": -1.3,
            "beam_slope_rad": 3.0E-3,
            "beam_diameter_mm": 12.0
        },
        {
            "type": "DistanceToBorder",
            "margin_mm": 5
        },
        {
            "type": "GlobalZSpan",
            "lower_margin_timecells": 25,
            "upper_margin_timecells": 5
        },
        {
            "type": "VertexZSpan",
            "beam_diameter_mm": 12.0
        }
    ]
}
  )");
  boost::property_tree::read_json(ss, tree);

  EXPECT_CALL(*imp, makeNonEmpty()).Times(1);
  EXPECT_CALL(*imp, makeVertexPosition(-1.3, 3.0E-3, 12.0)).Times(1);
  EXPECT_CALL(*imp, makeDistanceToBorder(nullptr, 5)).Times(1);
  EXPECT_CALL(*imp, makeGlobalZSpan(nullptr, 25, 5)).Times(1);
  EXPECT_CALL(*imp, makeVertexZSpan(nullptr, 12)).Times(1);

  CutsFactory cutsMaker(std::move(imp));
  for (const auto &cutConfig : tree.find("cuts")->second) {
    cutsMaker.create(nullptr, cutConfig.second);
  }
}