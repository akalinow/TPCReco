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

SimEvent::SimEvent(SimTracks &trackVector, const TVector3 &vertexPos, reaction_type type)
        : tracks{trackVector}, trueVertexPosition{vertexPos}, trigShiftedVertexPosition{vertexPos}, reactionType{type} {
    UpdateSimTracksStartPoint();
}

SimTracks &SimEvent::GetTracks() {
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

TVector3 SimEvent::GetTrueVertexPosition() const {
    return trueVertexPosition;
}

TVector3 SimEvent::GetTrigShiftedVertexPosition() const {
    return trigShiftedVertexPosition;
}


void SimEvent::SetTrueVertexPosition(const TVector3 &pos) {
    trueVertexPosition = pos;
}

void SimEvent::SetTrigShiftedVertexPosition(const TVector3 &pos) {
    trigShiftedVertexPosition = pos;
}

void SimEvent::UpdateSimTracksStartPoint() {
    for (auto &t: tracks)
        t.SetStart(trueVertexPosition);
}

void SimEvent::SetReactionType(reaction_type type) {
    reactionType = type;
}

reaction_type SimEvent::GetReactionType() const {
    return reactionType;
}


void SimEvent::Clear(Option_t *op) {
    //clear SimTracks:
    tracks.clear();
    //set reaction type to unknown:
    reactionType = reaction_type::UNKNOWN;
    //zero-out vertex position:
    trueVertexPosition = {0, 0, 0};
}

bool SimEvent::IsFullyContained() const {
    return isFullyContained;
}

void SimEvent::SetFullyContained(bool cont) {
    isFullyContained=cont;
}

void SimEvent::Shift(TVector3 &offset) {
    trigShiftedVertexPosition+=offset;
    for(auto& t: tracks){
        t.Shift(offset);
    }
}
