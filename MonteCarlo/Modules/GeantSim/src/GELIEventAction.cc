/**
 * @file GELIEventAction.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIEventAction class
 */

#include "GELIEventAction.hh"

void GELIEventAction::EndOfEventAction(const G4Event *evt) {
    auto &tracks = buffer.simEv->GetTracks();
    for (auto &track: tracks) {

        //first we sort hits, so that they start at the vertex and continue outward in the vector
        track.SortHits();
        //Now we set the track stop position (hit furthest from the start):
        track.RecalculateStopPosition();

        //set truncated positions to be tha same as the real ones - no truncation has been performed yet
        track.SetTruncatedStart(track.GetStart());
        track.SetTruncatedStop(track.GetStop());
    }
}

















