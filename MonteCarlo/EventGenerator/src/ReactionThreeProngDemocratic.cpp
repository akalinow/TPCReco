#include "ReactionThreeProngDemocratic.h"
#include "TRandom3.h"
#include "Math/LorentzVector.h"
#include "Math/LorentzRotation.h"

using namespace std::string_literals;

PrimaryParticles
ReactionThreeProngDemocratic::GeneratePrimaries(double gammaMom, const ROOT::Math::Rotation3D &beamToDetRotation) {
    //Basically a copy-paste from Mikolaj's generateFakeRecoEvents.cxx
    //slightly modified geometry definitions and adjusted to work with Reaction interface.
    auto r = gRandom; //random engine
    //For the moment fix target and products nuclei, if needed we can generalize it.
    auto target = pid_type::CARBON_12;
    auto product = pid_type::ALPHA;
    auto targetMass = ionProp->GetAtomMass(target);
    auto productMass = ionProp->GetAtomMass(product);

    //sanity check, it will work for hard-coded particles, to keep good practise
    CheckStoichiometry({target}, {product, product, product});
    GetKinematics(gammaMom, targetMass);
    double Qvalue = totalEnergy - 3 * productMass;
    if (Qvalue < 0) {
        auto msg = "Beam energy is too low to create three "s;
        msg += enumDict::GetPidName(product) + "'s+! Creating empty event";
        std::cout << msg << std::endl;
        return {};
    }
    // assign randomly kinetic energies for 3 alpha particles:
    //
    // - matrix element is constant
    // - Q = T1+T2+T3 = height of a regular triangle (Dalitz plot for same-particle 3-body decay)
    // - inscribed circle of radius R=Q/3 is uniformely populated (non-relativistic case, Ti<<M)
    // - T3=R+r0*sin(f0), 0<=r0<=R, 0<=f0<2*pi
    // - T2=R+r0*sin(f0+2*pi/3)
    // - T1=R+r0*sin(f0+4*pi/3)=Q-T2-T3
    //
    double phi0 = r->Uniform(0, TMath::TwoPi());
    double r0 = sqrt(r->Uniform(0, pow(Qvalue / 3, 2))); // dS=dPhi*d(r^2)/2
    double T1_CMS = Qvalue / 3 + r0 * sin(phi0);
    double T2_CMS = Qvalue / 3 + r0 * sin(phi0 + TMath::TwoPi() / 3);
    double T3_CMS = Qvalue - T1_CMS - T2_CMS;
    double p1_CMS = sqrt(T1_CMS * (T1_CMS + 2 * productMass)); // sqrt(2*T1_CMS*alphaMass); // MeV
    double p2_CMS = sqrt(T2_CMS * (T2_CMS + 2 * productMass)); // sqrt(2*T2_CMS*alphaMass); // MeV
    double p3_CMS = sqrt(T3_CMS * (T3_CMS + 2 * productMass)); // sqrt(2*T3_CMS*alphaMass); // MeV

    // distribute isotropically 1st alpha particle momentum:
    double phi1 = r->Uniform(0, TMath::TwoPi()); // rad
    double theta1 = acos(r->Uniform(-1, 1)); // rad, dOmega=dPhi*d(cosTheta)
    // 4-momentum vector in beam coordinate system, CMS reference frame:
    TLorentzVector alpha1_P4_CMS(0, 0, p1_CMS, productMass + T1_CMS); // initial = along Z_BEAM
    alpha1_P4_CMS.RotateY(theta1); // rotate by theta_BEAM about Y_BEAM
    alpha1_P4_CMS.RotateZ(phi1); // rotate by phi_BEAM about beam axis

    // distribute isotropically 2nd alpha particle momentum
    // - cos(theta12) is constrained by momentum conservation: p3^2 = p1^2 + p2^2 + 2*p1*p2*cos(theta12)
    double phi12 = r->Uniform(0, TMath::TwoPi()); // rad
    double theta12 = acos(0.5 * (p3_CMS * p3_CMS - p1_CMS * p1_CMS - p2_CMS * p2_CMS) / p1_CMS / p2_CMS); // rad
    TLorentzVector alpha2_P4_CMS(p2_CMS * alpha1_P4_CMS.Vect().Unit(), productMass + T2_CMS); // initial = along ALPHA1
    alpha2_P4_CMS.Rotate(theta12,
                         alpha1_P4_CMS.Vect().Orthogonal()); // rotate by theta12 about axis orthogonal to ALPHA1
    alpha2_P4_CMS.Rotate(phi12, alpha1_P4_CMS.Vect()); // rotate by phi12 about ALPHA1 axis

    // calculate 3rd alpha particle momentum: P3=-(P1+P2)
    TLorentzVector alpha3_P4_CMS;
    alpha3_P4_CMS.SetVectM(-(alpha1_P4_CMS.Vect() + alpha2_P4_CMS.Vect()), productMass);

    auto convert = [](auto &LV) { return ROOT::Math::PxPyPzEVector(LV.X(), LV.Y(), LV.Z(), LV.E()); };
    auto convertBack = [](auto &v) { return TLorentzVector(v.Px(), v.Py(), v.Pz(), v.E()); };


    // boosting P4 from CMS frame to DET/LAB frame
    auto bst = ROOT::Math::Boost(-betaCM);
    auto lRot = ROOT::Math::LorentzRotation(beamToDetRotation);
    auto alpha1_P4_DET = lRot * bst(convert(alpha1_P4_CMS));
    auto alpha2_P4_DET = lRot * bst(convert(alpha2_P4_CMS));
    auto alpha3_P4_DET = lRot * bst(convert(alpha3_P4_CMS));

    PrimaryParticles result;

    result.emplace_back(product, convertBack(alpha1_P4_DET));
    result.emplace_back(product, convertBack(alpha2_P4_DET));
    result.emplace_back(product, convertBack(alpha3_P4_DET));
    return result;
}
