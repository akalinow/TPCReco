#include "GELITrackingAction.hh"
#include "G4TrackingManager.hh"
#include "G4Track.hh"
#include "G4TrackVector.hh"
#include "GELITrackInformation.hh"
#include <G4Event.hh>
#include <G4EventManager.hh>
#include "GELIPrimaryGeneratorAction.hh"


void GELITrackingAction::PreUserTrackingAction(const G4Track *aTrack) {

    //Pass information on primary particle to a track
    if (aTrack->GetParentID() == 0 && aTrack->GetUserInformation() == nullptr) {
        auto trackId = aTrack->GetTrackID();
        auto geantEvent = G4EventManager::GetEventManager()->GetConstCurrentEvent();
        const auto nPrimaryVertices = geantEvent->GetNumberOfPrimaryVertex();
        for (G4int iVertex = 0; iVertex < nPrimaryVertices; ++iVertex) {
            auto primaryVertex = geantEvent->GetPrimaryVertex(iVertex);
            const auto nVertexParticles = primaryVertex->GetNumberOfParticle();
            for (G4int iParticle = 0; iParticle < nVertexParticles; ++iParticle) {
                const G4PrimaryParticle *primaryParticle =
                        primaryVertex->GetPrimary(iParticle);

                const auto userInfo = dynamic_cast<GELIPrimaryParticleInfo *>(primaryParticle->GetUserInformation());
                auto primId = userInfo->primId;
                auto trackIdPrim = primaryParticle->GetTrackID();
                if (trackId == trackIdPrim)
                    aTrack->SetUserInformation(new GELITrackInformation(aTrack, primId));

            }
        }

    }
}

void GELITrackingAction::PostUserTrackingAction(const G4Track *aTrack) {
    auto secondaries = fpTrackingManager->GimmeSecondaries();
    if (secondaries) {
        auto info = dynamic_cast<GELITrackInformation *>(aTrack->GetUserInformation());
        size_t nSeco = secondaries->size();
        if (nSeco > 0) {
            for (size_t i = 0; i < nSeco; i++) {
                auto infoNew = new GELITrackInformation(info);
                (*secondaries)[i]->SetUserInformation(infoNew);
            }
        }
    }
}

