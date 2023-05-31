/**
 * @file GELISteppingAction.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELISteppingAction class
 */

#include "GELISteppingAction.hh"

#include "G4SteppingManager.hh"
#include "G4VTouchable.hh"
#include "G4SystemOfUnits.hh"
#include "G4EventManager.hh"
#include "GELITrackInformation.hh"


#include "G4RunManager.hh"
#include "TFile.h"
#include "TTree.h"


void GELISteppingAction::UserSteppingAction(const G4Step *aStep) {
    //Collection at SSD in N-tuples. Electrons and photons separated
    //Prestep point in World, next volume MeasureVolume, process transportation


    //register only hits in gas mixture (world is gas)
    auto namepre = aStep->GetPostStepPoint()->GetTouchableHandle()->GetVolume()->GetName();
    auto namepost = aStep->GetPostStepPoint()->GetTouchableHandle()->GetVolume()->GetName();

    if(namepre!="world_physical" || namepost != "world_physical")
        return;



    G4double x, y, z, Edep;
    G4ThreeVector v1;
    G4ThreeVector v2;
    Edep = aStep->GetTotalEnergyDeposit() / MeV;
    if (Edep == 0)
        return;
    v1 = aStep->GetPreStepPoint()->GetPosition();
    v2 = aStep->GetPostStepPoint()->GetPosition();
    G4double x1, y1, x2, y2, z1, z2; //point coordinates in mm


    x1 = v1.getX() / mm;
    y1 = v1.getY() / mm;
    z1 = v1.getZ() / mm;
    x2 = v2.getX() / mm;
    y2 = v2.getY() / mm;
    z2 = v2.getZ() / mm;

    x = (x1 + x2) / 2;
    y = (y1 + y2) / 2;
    z = (z1 + z2) / 2;


    auto info = dynamic_cast<GELITrackInformation *>(aStep->GetTrack()->GetUserInformation());
    auto primID = info->GetOriginalPrimaryId();


    buffer.simEv->GetTracks()[primID].InsertHit({x, y, z, Edep});
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
