#include <gtest/gtest.h>
#include "TVector2.h"
#include "../DataFormats/include/GeometryTPC.h"

TEST(DataFormatsTests, MatchCrossPointTest) {
    std::array<int, 3> strip_nums = {2,1,0};
    double radius = 5.0;
    TVector2 point;
    bool doPointsMatch = Geometry("/TPCReco/resources/geometry_mini_eTPC.dat", 1).MatchCrossPoint(strip_nums, radius, point);
    EXPECT_EQ(true, doPointsMatch);
}

