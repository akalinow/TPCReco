/**
 * @file SimEvent.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of SimEvent class and PrimaryParticle struct
 */

#include "SimEvent.h"
/// \cond
#include<iostream>
#include <utility>
/// \endcond
//ClassImp(SimEvent);

SimEvent::SimEvent(SimTracks &trackVector, const TVector3& vertexPos, reaction_type type)
        : tracks{trackVector}, vertexPosition{vertexPos}, reactionType{type} {
    UpdateSimTracksStartPoint();
}

SimTracks & SimEvent::GetTracks() {
    return tracks;
}

SimTracksIterator SimEvent::TracksBegin() {
    return tracks.begin();
}

SimTracksIterator SimEvent::TracksEnd() {
    return tracks.end();
}

void SimEvent::SetSimTracks(SimTracks &trackVector) {
    tracks = trackVector;
    UpdateSimTracksStartPoint();
}

TVector3 SimEvent::GetVertexPosition() {
    return vertexPosition;
}

void SimEvent::SetStartVertexPosition(TVector3 &pos) {
    vertexPosition = pos;


}

void SimEvent::UpdateSimTracksStartPoint() {
    for (auto &t: tracks)
        t.SetStart(vertexPosition);
}

void SimEvent::SetReactionType(reaction_type type) {
    reactionType=type;
}

reaction_type SimEvent::GetReactionType() {
    return reactionType;
}


void SimEvent::Clear(Option_t *op) {
    //clear SimTracks:
    tracks.clear();
    //set reaction type to unknown:
    reactionType=reaction_type::UNKNOWN;
    //zero-out vertex position:
    vertexPosition={0,0,0};
}
