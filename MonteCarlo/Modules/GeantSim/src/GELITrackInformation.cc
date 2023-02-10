#include "GELITrackInformation.hh"
#include "G4ios.hh"

//G4Allocator<GELITrackInformation> aTrackInformationAllocator;

GELITrackInformation::GELITrackInformation() {
    originalTrackID = 0;
    particleDefinition = 0;
    originalPosition = G4ThreeVector(0., 0., 0.);
    originalMomentum = G4ThreeVector(0., 0., 0.);
    originalEnergy = 0.;
    originalTime = 0.;
    primID = 0;
}

GELITrackInformation::GELITrackInformation(const G4Track *aTrack, size_t pId) {
    originalTrackID = aTrack->GetTrackID();
    particleDefinition = aTrack->GetDefinition();
    originalPosition = aTrack->GetPosition();
    originalMomentum = aTrack->GetMomentum();
    originalEnergy = aTrack->GetKineticEnergy();
    originalTime = aTrack->GetGlobalTime();
    primID = pId;
}

GELITrackInformation::GELITrackInformation(const GELITrackInformation *aTrackInfo) {
    originalTrackID = aTrackInfo->originalTrackID;
    particleDefinition = aTrackInfo->particleDefinition;
    originalPosition = aTrackInfo->originalPosition;
    originalMomentum = aTrackInfo->originalMomentum;
    originalEnergy = aTrackInfo->originalEnergy;
    originalTime = aTrackInfo->originalTime;
    primID = aTrackInfo->primID;
}

GELITrackInformation::~GELITrackInformation() { ; }

void GELITrackInformation::Print() const {
    G4cout
            << "Original track ID " << originalTrackID
            << " at " << originalPosition << G4endl;
}