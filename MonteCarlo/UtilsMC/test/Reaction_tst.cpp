#include <Reaction.h>
#include "AngleProvider.h"
#include <string>
#include "gtest/gtest.h"

class AngleProviderDummy : public AngleProvider {
    double GetAngle() override { return 0; }
};

class ReactionFixture : public Reaction, public testing::Test {
    PrimaryParticles GeneratePrmaries(double gammaMom, ROOT::Math::Rotation3D &beamToDetRotation) override { return {}; }
};

TEST_F(ReactionFixture, TestKinematics) {
    GetKinematics(1, 1);
    //betaCM=c*p_tot/E_tot plus root convention for beta sign
    auto expectedBeta = ROOT::Math::PxPyPzEVector::BetaVector{0, 0, -0.5};
    EXPECT_EQ(betaCM, expectedBeta);
}
