#include "ReactionTwoProng.h"
#include "Math/Boost.h"
#include "Math/LorentzRotation.h"

using namespace std::string_literals;

ReactionTwoProng::ReactionTwoProng(std::unique_ptr<AngleProvider> theta, std::unique_ptr<AngleProvider> phi,
                                   pid_type targetIon, pid_type prod1Ion, pid_type prod2Ion)
        : thetaProv{std::move(theta)}, phiProv{std::move(phi)}, target{targetIon}, prod1Pid{prod1Ion},
          prod2Pid{prod2Ion} {
    CheckStoichiometry({targetIon},{prod1Ion,prod2Ion});
    targetMass = ionProp->GetAtomMass(target);
    prod1Mass = ionProp->GetAtomMass(prod1Ion);
    prod2Mass = ionProp->GetAtomMass(prod2Ion);
}

PrimaryParticles
ReactionTwoProng::GeneratePrmaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation) {
    GetKinematics(gammaMom, targetMass);
    auto Qvalue = totalEnergy - prod1Mass - prod2Mass;
    //return empty vector if we do not have enough energy
    if (Qvalue < 0) {
        auto msg = "Beam energy is too low to create "s;
        msg += enumDict::GetPidName(prod1Pid) + "+" + enumDict::GetPidName(prod2Pid);
        msg += " pair! Creating empty event";
        std::cout << msg << std::endl;
        return {};
    }
    auto p_CM = 0.5 * sqrt((pow(totalEnergy, 2) - pow(prod2Mass - prod1Mass, 2)) *
                           (pow(totalEnergy, 2) - pow(prod1Mass + prod2Mass, 2))) / totalEnergy;
    auto thetaCM = thetaProv->GetAngle();
    auto phiCM = phiProv->GetAngle();
    auto p3 = ROOT::Math::Polar3DVector(p_CM, thetaCM, phiCM);
    auto p4FirstCM = ROOT::Math::PxPyPzEVector(p3.X(), p3.Y(), p3.Z(),
                                               sqrt(p_CM * p_CM + prod1Mass * prod1Mass));
    auto p4SecondCM = ROOT::Math::PxPyPzEVector(-p3.X(), -p3.Y(), -p3.Z(),
                                                sqrt(p_CM * p_CM + prod2Mass * prod2Mass));

    auto bst = ROOT::Math::Boost(-betaCM);
    auto lRot = ROOT::Math::LorentzRotation(beamToDetRotation);
    auto p4FirstLAB = lRot * bst(p4FirstCM);
    auto p4SecondLAB = lRot * bst(p4SecondCM);
    PrimaryParticles result;
    TLorentzVector p4First{p4FirstLAB.Px(), p4FirstLAB.Py(), p4FirstLAB.Pz(), p4FirstLAB.E()};
    TLorentzVector p4Second{p4SecondLAB.Px(), p4SecondLAB.Py(), p4SecondLAB.Pz(), p4SecondLAB.E()};
    result.emplace_back(prod1Pid, p4First);
    result.emplace_back(prod2Pid, p4Second);
    return result;

}
