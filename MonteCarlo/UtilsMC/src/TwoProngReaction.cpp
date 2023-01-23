#include "TwoProngReaction.h"
#include "Math/Boost.h"

using namespace std::string_literals;

TwoProngReaction::TwoProngReaction(std::unique_ptr<AngleProvider> theta, std::unique_ptr<AngleProvider> phi,
                                   pid_type targetIon, pid_type prod1Ion, pid_type prod2Ion)
        : thetaProv{std::move(theta)}, phiProv{std::move(phi)}, target{targetIon} {
    targetMass = ionProp->GetAtomMass(target);
    auto prod1Mass = ionProp->GetAtomMass(prod1Ion);
    auto prod2Mass = ionProp->GetAtomMass(prod2Ion);
    if (prod1Mass > prod2Mass)
    {
        heavyProd = prod1Ion;
        lightProd = prod2Ion;
        heavyProdMass = prod1Mass;
        lightProdMass = prod2Mass;
    }
    else
    {
        heavyProd = prod2Ion;
        lightProd = prod1Ion;
        heavyProdMass = prod2Mass;
        lightProdMass = prod1Mass;
    }
}

PrimaryParticles
TwoProngReaction::GeneratePrmaries(const ROOT::Math::XYZVector &gammaMom) {
    GetKinematics(gammaMom, targetMass);
    auto Qvalue = totalEnergy - lightProdMass - heavyProdMass;
    //return empty vector if we do not have enough energy
    if (Qvalue < 0)
    {
        auto msg = "Beam energy is too low to create "s;
        msg += enumDict::GetPidName(lightProd) + "+" + enumDict::GetPidName(heavyProd);
        msg += " pair! Creating empty event";
        std::cout << msg << std::endl;
        return {};
    }
    auto p_CM = 0.5 * sqrt((pow(totalEnergy, 2) - pow(lightProdMass - heavyProdMass, 2)) *
                           (pow(totalEnergy, 2) - pow(lightProdMass + heavyProdMass, 2))) / totalEnergy;
    auto thetaCM = thetaProv->GetAngle();
    auto phiCM = phiProv->GetAngle();
    //rotate theta around -Z axis
    auto p4LightCM = GenerateFourMomentum(p_CM, lightProdMass, thetaCM, phiCM, gammaMom, {0, 0, -1});
    auto p4HeavyCM = GenerateFourMomentum(p_CM, heavyProdMass, ROOT::Math::Pi() - thetaCM, -phiCM, gammaMom,
                                          {0, 0, -1});
    auto bst = ROOT::Math::Boost(-betaCM);
    auto p4LightLAB = bst(p4LightCM);
    auto p4HeavyLAB = bst(p4HeavyCM);
    PrimaryParticles result;
    result.emplace_back(lightProd, p4LightLAB);
    result.emplace_back(heavyProd, p4HeavyLAB);
    return result;

}
