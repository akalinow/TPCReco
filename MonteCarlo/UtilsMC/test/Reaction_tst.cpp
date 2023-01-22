#include <Reaction.h>
#include "AngleProvider.h"
#include <string>
#include "gtest/gtest.h"

class AngleProviderDummy : public AngleProvider {
    double GetAngle() override { return 0; }
};

class ReactionFixture : public Reaction, public testing::Test {
    PrimaryParticles GeneratePrmaries(const ROOT::Math::XYZVector &gammaMom,
                                      const ROOT::Math::XYZPoint &vertexPos) override { return {}; }
};

TEST_F(ReactionFixture, TestKinematics) {
    ROOT::Math::XYZVector mom{0, 0, 1}; //unit momentum
    double mass = 1; //unit mass
    GetKinematics(mom, mass);
    //betaCM=c*p_tot/E_tot plus root convention for beta sign
    auto expectedBeta = ROOT::Math::PxPyPzEVector::BetaVector{0, 0, -0.5};
    EXPECT_EQ(betaCM, expectedBeta);
}

TEST_F(ReactionFixture, TestGenerateFourMomentum) {
    double precisionLimit = 1e-10;
    double mass = 1;
    auto direction = ROOT::Math::XYZVector{1, 0, 0};
    auto thetaPlaneNormal = ROOT::Math::XYZVector{0, 0, 1};
    //Check if GenerateFourMomentum will throw when vectors are colinear
    EXPECT_THROW(GenerateFourMomentum(0, mass, 0, 0, direction, direction), std::runtime_error);
    //it should not throw when vectors are perpendicular:
    EXPECT_NO_THROW(GenerateFourMomentum(0, mass, 0, 0, direction, thetaPlaneNormal));

    EXPECT_TRUE((GenerateFourMomentum(0, mass, 0, 0, direction, thetaPlaneNormal) -
                 ROOT::Math::PxPyPzEVector(0, 0, 0, mass)).R() < precisionLimit);
    EXPECT_TRUE((GenerateFourMomentum(1, mass, 0, 0, direction, thetaPlaneNormal) -
                 ROOT::Math::PxPyPzEVector(1, 0, 0, sqrt(2))).R() < precisionLimit);
    EXPECT_TRUE((GenerateFourMomentum(1, mass, ROOT::Math::Pi(), 0, direction, thetaPlaneNormal) -
                 ROOT::Math::PxPyPzEVector(-1, 0, 0, sqrt(2))).R() < precisionLimit);
    EXPECT_TRUE((GenerateFourMomentum(1, mass, ROOT::Math::Pi() / 2, 0, direction, thetaPlaneNormal) -
                 ROOT::Math::PxPyPzEVector(0, 1, 0, sqrt(2))).R() < precisionLimit);
    EXPECT_TRUE((GenerateFourMomentum(1, mass, ROOT::Math::Pi() / 2, ROOT::Math::Pi(), direction, thetaPlaneNormal) -
                 ROOT::Math::PxPyPzEVector(0, -1, 0, sqrt(2))).R() < precisionLimit);
    EXPECT_TRUE(
            (GenerateFourMomentum(1, mass, ROOT::Math::Pi() / 2, ROOT::Math::Pi() / 2, direction, thetaPlaneNormal) -
             ROOT::Math::PxPyPzEVector(0, 0, 1, sqrt(2))).R() < precisionLimit);
}

