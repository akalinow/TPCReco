
#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <Fit/Fitter.h>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"
#include "TFitResult.h"
#include "Math/Functor.h"
#include "TH1D.h"

#include "GeometryTPC.h"
#include "EventCharges.h"
#include "EventHits.h"

#include "TrackSegment2D.h"
#include "TrackSegment3D.h"
#include "Track3D.h"

class TH2D;
class TF1;

class GeometryTPC;
class EventCharges;
class HistoManager;

class TrackBuilder {
public:
	
	TrackBuilder();

	~TrackBuilder() = default;

	std::shared_ptr<EventHits> setEvent(std::shared_ptr<EventCharges> evt_ch);

	void initialize();

	void reconstruct();

	const std::shared_ptr<EventHits> GetHits() const { return hitsObject; }

	const std::shared_ptr <TH2D> getRecHits2D(direction dir) const;

	const TrackSegment2D& getSegment2D(direction dir, unsigned int iTrack = 0) const;

	const TrackSegment3D& getSegment3DSeed() const;

	const Track3D& getTrack3D(unsigned int iSegment) const;

private:

	void makeRecHits(direction dir);

	TF1 fitTimeWindow(std::shared_ptr<TH1D> hProj);

	void fillHoughAccumulator(direction dir);

	TrackSegment2DCollection findSegment2DCollection(direction dir);

	TrackSegment2D findSegment2D(direction dir, int iPeak) const;

	TrackSegment3D buildSegment3D() const;

	Track3D fitTrack3D(const TrackSegment3D& aTrackSeedSegment) const;

	Track3D fitTrackNodes(const Track3D& aTrack) const;


	std::shared_ptr<EventCharges> chargesObject;
	std::shared_ptr<EventHits> hitsObject;

	bool myHistoInitialized;
	int nAccumulatorRhoBins, nAccumulatorPhiBins;

	TVector3 aHoughOffest;
	std::map<direction, TH2D> myAccumulators;
	std::map<direction, std::shared_ptr<TH2D>> myRecHits;
	std::map<direction, TrackSegment2DCollection> my2DSeeds;

	TrackSegment3D myTrack3DSeed;

	Track3D myFittedTrack;

	mutable ROOT::Fit::Fitter fitter;

};
#endif

