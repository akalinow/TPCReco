#include "TPCReco/ReactionParticleGun.h"
#include "Math/LorentzRotation.h"

ReactionParticleGun::ReactionParticleGun(std::unique_ptr<AngleProvider> theta, std::unique_ptr<AngleProvider> phi,
                                         std::unique_ptr<EProvider> energy, pid_type ionType)
        : particleId{ionType}, thetaProv{std::move(theta)}, phiProv{std::move(phi)}, eProv{std::move(energy)} {
    particleMass=ionProp->GetAtomMass(ionType);
}

PrimaryParticles ReactionParticleGun::GeneratePrimaries(double v, const ROOT::Math::Rotation3D &beamToDetRotation) {
    auto theta = thetaProv->GetAngle();
    auto phi = phiProv->GetAngle();
    auto e = eProv->GetEnergy();
    auto mom = sqrt(e*e+2*e*particleMass); //momentum (the relativistic way)
    ROOT::Math::Polar3DVector p3(mom, theta, phi);
    ROOT::Math::PxPyPzEVector p4(p3.X(), p3.Y(), p3.Z(),
                                        sqrt(mom * mom + particleMass*particleMass));

    ROOT::Math::LorentzRotation lRot(beamToDetRotation);
    p4=lRot*p4;

    TLorentzVector lVec(p4.Px(),p4.Py(),p4.Pz(),p4.E());
    //we return single particle:
    return {PrimaryParticle{particleId,lVec}};
}
