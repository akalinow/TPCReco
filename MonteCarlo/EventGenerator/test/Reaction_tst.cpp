#include "TPCReco/Reaction.h"
#include "TPCReco/ReactionTwoProng.h"
#include "TPCReco/AngleProvider.h"
#include <string>
#include "gtest/gtest.h"

class AngleProviderDummy : public AngleProvider {
public:
    explicit AngleProviderDummy(double v) : val{v} {}
    double GetAngle() override { return val; }
    std::string GetName() override {return "AngleProviderDummy"; }
private:
    void ValidateParamValues() override {}
    double val=0;
};

class ReactionFixture : public Reaction, public testing::Test {
    PrimaryParticles GeneratePrimaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation) override { return {}; }
};

TEST_F(ReactionFixture, TestKinematics) {
    GetKinematics(1, 1);
    //betaCM=c*p_tot/E_tot plus root convention for beta sign
    auto expectedBeta = ROOT::Math::PxPyPzEVector::BetaVector{0, 0, -0.5};
    EXPECT_EQ(betaCM, expectedBeta);
}

TEST_F(ReactionFixture, TestStochiometry){
    //wrong stoichiometry:
    EXPECT_THROW(CheckStoichiometry({OXYGEN_16},{CARBON_12,PROTON}), std::runtime_error);

    //correct stoichiometry:
    EXPECT_NO_THROW(CheckStoichiometry({OXYGEN_16},{CARBON_12,ALPHA}));
}

TEST(ReactionTwoProng, TestAngles){
    auto theta = std::unique_ptr<AngleProvider>(new AngleProviderDummy(0));
    auto phi = std::unique_ptr<AngleProvider>(new AngleProviderDummy(0));
    auto r=new ReactionTwoProng(std::move(theta),std::move(phi),OXYGEN_16,ALPHA,CARBON_12);
    auto prim = r->GeneratePrimaries(0, ROOT::Math::Rotation3D());
    //event should be empty with zero gamma momentum:
    EXPECT_EQ(prim.size(),0);

    //Particles produced along beam axis should have unchanged theta:
    prim= r->GeneratePrimaries(10, ROOT::Math::Rotation3D());
    EXPECT_DOUBLE_EQ(prim[0].GetFourMomentum().CosTheta(),1);
    EXPECT_DOUBLE_EQ(prim[1].GetFourMomentum().CosTheta(),-1);
}