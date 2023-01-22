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

SimEvent::SimEvent(SimTracks &trackVector, ROOT::Math::XYZPoint vertexPos, reaction_type type)
        : tracks{trackVector}, vertexPosition{std::move(vertexPos)}, reactionType{type} {
    UpdateSimTracksStartPoint();
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

void SimEvent::SetSimTracks(SimTracks &trackVector) {
    tracks = trackVector;
    UpdateSimTracksStartPoint();
}

ROOT::Math::XYZPoint SimEvent::GetVertexPosition() {
    return vertexPosition;
}

void SimEvent::SetStartVertexPosition(ROOT::Math::XYZPoint pos) {
    vertexPosition = std::move(pos);


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
