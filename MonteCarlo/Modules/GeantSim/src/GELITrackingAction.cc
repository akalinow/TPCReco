#include "GELITrackingAction.hh"
#include "G4TrackingManager.hh"
#include "G4Track.hh"
#include "G4TrackVector.hh"
#include "GELITrackInformation.hh"

GELITrackingAction::GELITrackingAction()
{;}

GELITrackingAction::~GELITrackingAction()
{;}

void GELITrackingAction::PreUserTrackingAction(const G4Track* aTrack)
{
  
  
  if(aTrack->GetParentID()==0 && aTrack->GetUserInformation()==0)
  {
    GELITrackInformation* anInfo = new GELITrackInformation(aTrack);
    aTrack->SetUserInformation(anInfo);
  }
}

void GELITrackingAction::PostUserTrackingAction(const G4Track* aTrack)
{
  G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
  if(secondaries)
  {
    GELITrackInformation* info = (GELITrackInformation*)(aTrack->GetUserInformation());
    size_t nSeco = secondaries->size();
    if(nSeco>0)
    {
      for(size_t i=0;i<nSeco;i++)
      { 
        GELITrackInformation* infoNew = new GELITrackInformation(info);
        (*secondaries)[i]->SetUserInformation(infoNew);
      }
    }
  }
}
