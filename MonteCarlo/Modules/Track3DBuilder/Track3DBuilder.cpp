#include "Track3DBuilder.h"
#include "TRandom.h"
#include "TPCReco/colorText.h"

fwk::VModule::EResultFlag Track3DBuilder::Init(boost::property_tree::ptree config) {
    // set hit filter params for creating pseudo RecHits for generator level TrackSegment3D
    auto hitFilterTypeName = config.get<std::string>("simRecoHitFilter.recoClusterType");
    auto hitFilterThreshold = config.get<double>("simRecoHitFilter.recoClusterThreshold");
    auto hitFilterFraction = config.get<double>("simRecoHitFilter.recoClusterConstantFractionThreshold");
    auto hitFilterDeltaStrips = config.get<int>("simRecoHitFilter.recoClusterDeltaStrips");
    auto hitFilterDeltaTimeCells = config.get<int>("simRecoHitFilter.recoClusterDeltaTimeCells");
    boost::property_tree::ptree aHitFilterConfig; // needed for pseudo RecHits per track
    aHitFilterConfig.put("hitFilter.recoClusterThreshold", hitFilterThreshold);
    aHitFilterConfig.put("hitFilter.recoClusterConstantFractionThreshold", hitFilterFraction);
    aHitFilterConfig.put("hitFilter.recoClusterDeltaStrips", hitFilterDeltaStrips); 
    aHitFilterConfig.put("hitFilter.recoClusterDeltaTimeCells", hitFilterDeltaTimeCells);
    pseudoRecoHitFilterType = enumDict::GetHitFilterType(hitFilterTypeName);
    pseudoRecoEventTPC.SetGeoPtr(geometry);
    pseudoRecoEventTPC.setHitFilterConfig(pseudoRecoHitFilterType, aHitFilterConfig);
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Track3DBuilder::Process(ModuleExchangeSpace &event) {
    auto &currentSimEvent = event.simEvt;
    TrackSegment3D aSegment;
    Track3D aTrack;

    // needed to store true RecHits per individual generator level TrackSegment3D
    auto iTrack=0U; // track iterator for event.trackPEvt[] vector 
    std::vector<TH2D> aRecHitsColl;

    //loop over tracks
    for (const auto &t: currentSimEvent.GetTracks()) {
        ++iTrack;

        //do not add segments when track is fully out of active volume
        if(t.IsOutOfActiveVolume()) continue;

        aSegment.setGeometry(geometry);
        aSegment.setStartEnd(t.GetTruncatedStart(), t.GetTruncatedStop());
        aSegment.setPID(t.GetPrimaryParticle().GetID());

	// store true RecHits per individual generator level TrackSegment3D
	if(event.trackPEvt.size()<iTrack) continue; // RecHits info is missing or disabled
	pseudoRecoEventTPC.SetChargeMap(event.trackPEvt.at(iTrack-1).GetChargeMap());
	aRecHitsColl.resize(0);
	for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
	  auto projType = get2DProjectionType(strip_dir);
	  aRecHitsColl.push_back( *(pseudoRecoEventTPC.get2DProjection(projType, pseudoRecoHitFilterType, scale_type::mm)) );
	  //	  std::cout << KBLU << __FUNCTION__ << ": eventId=" << event.eventInfo.GetEventId()
	  //		    << ", track=" << iTrack
	  //		    << ": " << geometry->GetDirName(strip_dir) << "-dir charge = " << aRecHitsColl.back().Integral()
	  //		    << ", entries = " << aRecHitsColl.back().GetEntries()
	  //		    << RST << std::endl;
	}
	aSegment.setRecHits(aRecHitsColl);

        aTrack.addSegment(aSegment);
    }
    event.track3D = aTrack;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Track3DBuilder::Finish() {
    return fwk::VModule::eSuccess;
}
