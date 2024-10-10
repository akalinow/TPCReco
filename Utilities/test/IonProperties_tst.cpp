#include "TPCReco/IonProperties.h"
#include "gtest/gtest.h"

TEST(IonProperties, GetZ) {
    auto prop=IonProperties::GetInstance();
    auto protonZ=prop->GetZ(pid_type::PROTON);
    auto alphaZ=prop->GetZ(pid_type::ALPHA);
    EXPECT_EQ(protonZ,1);
    EXPECT_EQ(alphaZ,2);
}

TEST(IonProperties, GetA) {
    auto prop=IonProperties::GetInstance();
    auto protonA=prop->GetA(pid_type::PROTON);
    auto alphaA=prop->GetA(pid_type::ALPHA);
    EXPECT_EQ(protonA,1);
    EXPECT_EQ(alphaA,4);
}

TEST(IonProperties, GetMass) {
    auto prop=IonProperties::GetInstance();
    auto protonMass= prop->GetAtomMass(pid_type::PROTON);
    auto alphaMass= prop->GetAtomMass(pid_type::ALPHA);
    EXPECT_DOUBLE_EQ(protonMass,1.00782503190*IonProperties::atomicMassUnitMeV);
    EXPECT_DOUBLE_EQ(alphaMass,4.00260325413*IonProperties::atomicMassUnitMeV);
}

TEST(IonProperties, AtomicMassUnit) {
    auto prop=IonProperties::GetInstance();
    auto u=prop->atomicMassUnitMeV;
    EXPECT_DOUBLE_EQ(u, 931.49410242);

}

TEST(IonProperties, Exceptions){
    auto prop=IonProperties::GetInstance();
    EXPECT_THROW(
            prop->GetAtomMass(pid_type::THREE_ALPHA),
            std::out_of_range);
    EXPECT_THROW(
            prop->GetZ(pid_type::C12_ALPHA),
            std::out_of_range);
}