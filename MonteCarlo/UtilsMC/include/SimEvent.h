/**
 * @file SimEvent.hh
 * @author     Piotr Podlaski
 * @brief      Definition of SimEvent class, PrimaryParticle and SimHit structures
 */

#ifndef SIMEVENT_H
#define SIMEVENT_H

#include "TObject.h"
#include "TVector3.h"
#include "TH3F.h"
#include "TString.h"
#include "SimTrack.h"
/// \cond
#include <vector>
/// \endcond


/**
 * @class      SimEvent
 *
 * @brief      Class used to store information about simulated events.
 */
class SimEvent : public TObject {
public:
    /**
     * @brief      Constructor
     */
    SimEvent() = default;
    virtual ~SimEvent() = default;

    /**
     * @brief      Constructor setting all parameters
     */
    SimEvent(SimTracks &trackVector, const TVector3& vertexPos, reaction_type type);

    void SetSimTracks(SimTracks &trackVector);
    void SetStartVertexPosition(TVector3 &pos);
    void SetReactionType(reaction_type type);
    SimTracks GetTracks();
    SimTracksIterator TracksBegin();
    SimTracksIterator TracksEnd();
    TVector3 GetVertexPosition();
    reaction_type GetReactionType();

private:
    SimTracks tracks; /// Vector with simulated tracks (primary particles)
    TVector3 vertexPosition;
    reaction_type reactionType{reaction_type::UNKNOWN};
    void UpdateSimTracksStartPoint();
ClassDef(SimEvent, 1); ///< ROOT macro to register SimEvent class
};

#endif