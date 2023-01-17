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
class SimEvent : public TObject
{	
public:
	/**
	 * @brief      Constructor
	 */
	SimEvent()=default;
    virtual ~SimEvent()=default;

	/**
	 * @brief      Constructor taking vector of SimHit objects as an argument

	 */
	explicit SimEvent(SimTracks &tracks);

	void SetSimTracks(SimTracks &trcks);
	SimTracks GetTracks();
	SimTracksIterator TracksBegin();
	SimTracksIterator TracksEnd();
	
private:
	SimTracks tracks; /// Vector with simulated tracks (primary particles)
	ClassDef(SimEvent,1); ///< ROOT macro to register SimEvent class
};

#endif