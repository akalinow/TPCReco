#include "Reaction.h"
#include "Math/Boost.h"
#include "Math/AxisAngle.h"

Reaction::Reaction() : ionProp{IonProperties::GetInstance()} {}

void Reaction::GetKinematics(const double gammaMom, const double &targetMass) {
    ROOT::Math::PxPyPzEVector tot4Mom{0, 0, gammaMom, targetMass + gammaMom};
    betaCM = tot4Mom.BoostToCM();
    auto boost = ROOT::Math::Boost(betaCM);
    totalEnergy = boost(tot4Mom).E();
}

void Reaction::CheckStoichiometry(const std::vector<pid_type> &substrates, const std::vector<pid_type> &products) {
    int totalAProducts=0;
    int totalASubstrates=0;
    int totalZProducts=0;
    int totalZSubstrates=0;
    auto prop=IonProperties::GetInstance();
    for(const auto &p:products){
        totalAProducts+=prop->GetA(p);
        totalZProducts+=prop->GetZ(p);
    }
    for(const auto &p:substrates){
        totalASubstrates+=prop->GetA(p);
        totalZSubstrates+=prop->GetZ(p);
    }
    if(totalAProducts!=totalASubstrates||totalZProducts!=totalZSubstrates){
        std::string msg="Number of protons and neutrons for substrates and products of the reaction are not the same! Reaction: ";
        msg+=enumDict::GetPidName(substrates[0]);
        if(substrates.size()>1)
            for(size_t i=1;i<substrates.size();i++)
                msg+="+"+enumDict::GetPidName(substrates[i]);
        msg+="->"+enumDict::GetPidName(products[0]);
        if(products.size()>1)
            for(size_t i=1;i<products.size();i++)
                msg+="+"+enumDict::GetPidName(products[i]);
        msg+=" is not possible!";
        throw std::runtime_error(msg);
    }
}
