#ifndef GELITRACKINFORMATION_H
#define GELITRACKINFORMATION_H

#include "globals.hh"
#include "G4ThreeVector.hh"
#include "G4ParticleDefinition.hh"
#include "G4Track.hh"
#include "G4Allocator.hh"
#include "G4VUserTrackInformation.hh"

class GELITrackInformation : public G4VUserTrackInformation {
public:
    GELITrackInformation();

    GELITrackInformation(const G4Track *aTrack, size_t pId);

    explicit GELITrackInformation(const GELITrackInformation *aTrackInfo);

    ~GELITrackInformation() override = default;

    //   inline void *operator new(size_t);
    //   inline void operator delete(void *aTrackInfo);
    inline int operator==(const GELITrackInformation &right) const { return (this == &right); }

    void Print() const override;

private:
    G4int originalTrackID;
    G4ParticleDefinition *particleDefinition;
    G4ThreeVector originalPosition;
    G4ThreeVector originalMomentum;
    G4double originalEnergy;
    G4double originalTime;
    size_t primID;


public:
    inline G4int GetOriginalTrackID() const { return originalTrackID; }

    inline G4ParticleDefinition *GetOriginalParticle() const { return particleDefinition; }

    inline G4ThreeVector GetOriginalPosition() const { return originalPosition; }

    inline G4ThreeVector GetOriginalMomentum() const { return originalMomentum; }

    inline G4double GetOriginalEnergy() const { return originalEnergy; }

    inline G4double GetOriginalTime() const { return originalTime; }

    inline size_t GetOriginalPrimaryId() const { return primID; }
};

#endif