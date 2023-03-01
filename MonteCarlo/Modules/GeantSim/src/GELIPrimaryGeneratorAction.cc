/**
 * @file GELIPrimaryGeneratorAction.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIPrimaryGeneratorAction class
 */

#include "GELIPrimaryGeneratorAction.hh"

#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4String.hh"
#include "G4GeneralParticleSource.hh"


/// \cond
#include <vector>
/// \endcond

void GELIPrimaryParticleInfo::Print() const {
    G4cout << "Corresponding primary particle ID: " << primId << G4endl;
}


void GELIPrimaryGenerator::GeneratePrimaryVertex(G4Event *evt) {
    auto vtxPos = buffer.simEv->GetTrueVertexPosition();
    auto tracks = buffer.simEv->GetTracks();
    auto vtx = new G4PrimaryVertex(vtxPos.X(), vtxPos.Y(), vtxPos.Z(), 0);
    for (size_t i = 0; i < tracks.size(); i++) {
        const auto primPart = tracks[i].GetPrimaryParticle();
        const auto mom = primPart.GetFourMomentum();
        auto g4particle = new G4PrimaryParticle(
                GetGeantParticleDefinition(primPart.GetID()),
                mom.Px(),
                mom.Py(),
                mom.Pz()
        );
        g4particle->SetUserInformation(new GELIPrimaryParticleInfo(i));
        g4particle->SetMass(prop->GetAtomMass(primPart.GetID()));
        vtx->SetPrimary(g4particle);

    }
    evt->AddPrimaryVertex(vtx);
}

/// @brief for the moment ions only
/// \param particleID - id of the particle
/// \return pointer to G4ParticleDefinition
const G4ParticleDefinition *GELIPrimaryGenerator::GetGeantParticleDefinition(const pid_type particleID) {
    auto A = prop->GetA(particleID);
    auto Z = prop->GetZ(particleID);
    G4IonTable *iTable = G4IonTable::GetIonTable();
    auto particleDefinition = iTable->GetIon(Z, A, 0.);
    return particleDefinition;
}