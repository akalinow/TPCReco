#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "EventHits.h"
#include "TrackBuilder.h"

#include "TLine.h"
#include "TGraph.h"
#include "TH2Poly.h"

#include "CommonDefinitions.h"

class TH2D;
class TH3D;

class GeometryTPC;
class EventCharges;
class TrackBuilder;
class HistoManager;

HistoManager& HistogramManager();

class HistoManager {
	friend HistoManager& HistogramManager();
public:

	~HistoManager() = default;

	void setEvent(std::shared_ptr<EventCharges> aEvent);

	TH2D&& getRawStripVsTime(direction strip_dir);

	std::shared_ptr<TH2D> getCartesianProjection(direction strip_dir);

	std::shared_ptr<TH2D> getRecHitStripVsTime(direction strip_dir);

	Reconstr_hist getReconstruction(bool force);

	std::shared_ptr<TH3D> get3DReconstruction(bool force = false);

	std::shared_ptr<TH2D> get2DReconstruction(bool force = false);

	void drawTrack2DSeed(direction strip_dir, TVirtualPad* aPad);

	void drawTrack3D(TVirtualPad* aPad);

	void drawTrack3DProjectionTimeStrip(direction strip_dir, TVirtualPad* aPad);

	void drawTrack3DProjectionXY(TVirtualPad* aPad);

	void drawChargeAlongTrack3D(TVirtualPad* aPad) const;

	TH2D&& GetStripVsTime(direction strip_dir);                               // whole event, all strip dirs

	std::shared_ptr<TH2D> GetStripVsTimeInMM(direction strip_dir);  // valid range [0-2]

	Reconstr_hist&& Get(double radius,          // clustered hits only, / clustered hits only, 3D view
		int rebin_space = EVENTTPC_DEFAULT_STRIP_REBIN,   // directions on: XY, XZ, YZ planes / all planes
		int rebin_time = EVENTTPC_DEFAULT_TIME_REBIN,
		int method = EVENTTPC_DEFAULT_RECO_METHOD);

private:
	HistoManager() = default;

	std::shared_ptr<EventCharges> charges;
	std::shared_ptr<EventHits> hits;

	std::vector<TH2D*> directionsInCartesianCoords;
	std::shared_ptr<TH3D> h3DReco;
	TrackBuilder myTkBuilder;

	Reconstr_hist reconstruction;

	bool reconstruction_done = false;

};
#endif
