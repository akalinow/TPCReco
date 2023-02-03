#include "ReactionThreeProngIntermediate.h"
#include "Math/EulerAngles.h"
#include "Math/LorentzRotation.h"

using namespace std::string_literals;

PrimaryParticles
ReactionThreeProngIntermediate::GeneratePrimaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation) {
    auto target = pid_type::CARBON_12;
    auto product = pid_type::ALPHA;
    auto targetMass = ionProp->GetAtomMass(target);
    auto productMass = ionProp->GetAtomMass(product);

    //sanity check, it will work for hard-coded particles, to keep good practise
    CheckStoichiometry({target}, {product, product, product});
    GetKinematics(gammaMom,targetMass);
    //TODO: make dedicated BR handler to avoid code duplication with ReactionLibrary
    //mass of the intermediate state, from Breit-Wigner distribution
    auto intMass = BreitWignerRandom();

    auto Qvalue1 = totalEnergy - intMass - productMass;
    auto Qvalue2 = intMass - 2*productMass;
    //return empty vector if we do not have enough energy
    if (Qvalue1 < 0) {
        auto msg = "Beam energy is too low to create "s;
        msg += enumDict::GetPidName(product);
        msg += "+ IntermediateIon pair! Creating empty event";
        std::cout << msg << std::endl;
        return {};
    }
    if (Qvalue2 < 0) {
        auto msg = "Intermediate ion is too light to create two"s;
        msg += enumDict::GetPidName(product);
        msg += " ions! Creating empty event";
        std::cout << msg << std::endl;
        return {};
    }

    auto p_CM = 0.5 * sqrt((pow(totalEnergy, 2) - pow(productMass - intMass, 2)) *
                           (pow(totalEnergy, 2) - pow(productMass + intMass, 2))) / totalEnergy;
    auto theta1 = thetaFirst->GetAngle();
    auto phi1 = phiFirst->GetAngle();
    auto p3 = ROOT::Math::Polar3DVector(p_CM, theta1, phi1);
    //first product:
    auto p4FirstCM = ROOT::Math::PxPyPzEVector(p3.X(), p3.Y(), p3.Z(),
                                               sqrt(p_CM * p_CM + productMass * productMass));
    auto betaIntermediate = ROOT::Math::PxPyPzEVector(-p3.X(), -p3.Y(), -p3.Z(),
                                                      sqrt(p3.Mag2() + intMass*intMass)).BoostToCM();

    auto boostIntermediate = ROOT::Math::Boost(-betaIntermediate);

    //momentum (magnitude) of alphas in intermediate ion rest frame:
    p_CM = 0.5 * sqrt((pow(intMass, 2) - pow(2*productMass, 2)));
    //momentum (vector) of one alpha in intermediate ion rest frame
    auto theta2 = thetaSecond->GetAngle();
    auto phi2 = phiSecond->GetAngle();
    p3=  ROOT::Math::Polar3DVector(p_CM, theta2, phi2);
    //and rotated to CM frame, this way we have orientation of CM:
    p3= ROOT::Math::EulerAngles(-phi1, ROOT::Math::Pi() - theta1, 0) * p3;
    //we just need to boost it to have momenta in CM:
    auto p4SecondCM = boostIntermediate*ROOT::Math::PxPyPzEVector(p3.X(), p3.Y(), p3.Z(),
                                               sqrt(p3.Mag2() + productMass * productMass));
    auto p4ThirdCM = boostIntermediate*ROOT::Math::PxPyPzEVector(-p3.X(), -p3.Y(), -p3.Z(),
                                                                  sqrt(p3.Mag2() + productMass * productMass));

    auto lRot = ROOT::Math::LorentzRotation(beamToDetRotation);
    auto bst = ROOT::Math::Boost(-betaCM);

    auto p4FirstLAB = lRot * bst(p4FirstCM);
    auto p4SecondLAB = lRot * bst(p4SecondCM);
    auto p4ThirdLAB = lRot * bst(p4ThirdCM);

    TLorentzVector p4First(p4FirstLAB.Px(),p4FirstLAB.Py(),p4FirstLAB.Pz(),p4FirstLAB.E());
    TLorentzVector p4Second(p4SecondLAB.Px(),p4SecondLAB.Py(),p4SecondLAB.Pz(),p4SecondLAB.E());
    TLorentzVector p4Third(p4ThirdLAB.Px(),p4ThirdLAB.Py(),p4ThirdLAB.Pz(),p4ThirdLAB.E());

    PrimaryParticles result;
    result.emplace_back(product,p4First);
    result.emplace_back(product,p4Second);
    result.emplace_back(product,p4Third);

    return result;
}

std::pair<double, double> ReactionThreeProngIntermediate::SelectIntermediateState() {
    if (!isBrInitialized)
    {
        auto nStates = intermediateStates.size();
        if (nStates == 0)
            throw std::runtime_error(
                    "ReactionThreeProngIntermediate: at least one intermediate state has to be provided!");
        brHelperHisto = std::make_unique<TH1D>("hLibraryHelper", "", nStates, 0, nStates);
        for (auto i = 0U; i < intermediateStates.size(); i++)
        {
            brHelperHisto->SetBinContent(i + 1, intermediateStates[i].branchingRatio);
            isBrInitialized = true;
        }
    }
    auto stateId = brHelperHisto->FindBin(brHelperHisto->GetRandom()) - 1;
    return {intermediateStates[stateId].mass,intermediateStates[stateId].width};
}

double ReactionThreeProngIntermediate::BreitWignerRandom() {
    double intMass, intWidth;
    std::tie(intMass,intWidth)=SelectIntermediateState();
    auto r=bwTF1->GetRandom();
    return 0.5*r*intWidth+intMass;

}
