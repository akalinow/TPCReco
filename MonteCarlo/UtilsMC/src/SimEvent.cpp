/**
 * @file SimEvent.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of SimEvent class and PrimaryParticle struct
 */

#include "SimEvent.h"
/// \cond
#include<iostream>
/// \endcond
//ClassImp(SimEvent);


SimEvent::SimEvent(SimTracks &tracks) {
    this->tracks = tracks;
}

SimTracks SimEvent::GetTracks() {
    return tracks;
}

SimTracksIterator SimEvent::TracksBegin() {
    return tracks.begin();
}

SimTracksIterator SimEvent::TracksEnd() {
    return tracks.end();
}

void SimEvent::SetSimTracks(SimTracks &tracks) {
    this->tracks = tracks;
}