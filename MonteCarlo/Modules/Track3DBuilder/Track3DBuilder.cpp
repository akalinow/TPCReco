#include "Track3DBuilder.h"
#include "TRandom.h"
#include "TPCReco/ConfigManager.h"
//#include "TPCReco/colorText.h" // for DEBUG mode

fwk::VModule::EResultFlag Track3DBuilder::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    // set hit filter params for creating pseudo RecHits for generator level TrackSegment3D
    // auto hitFilterTypeName = config.get<std::string>("simRecoHitFilter.recoClusterType"); // without MATH expressions
    // auto hitFilterThreshold = config.get<double>("simRecoHitFilter.recoClusterThreshold"); // without MATH expressions
    // auto hitFilterFraction = config.get<double>("simRecoHitFilter.recoClusterConstantFractionThreshold"); // without MATH expressions
    // auto hitFilterDeltaStrips = config.get<int>("simRecoHitFilter.recoClusterDeltaStrips"); // without MATH expressions
    // auto hitFilterDeltaTimeCells = config.get<int>("simRecoHitFilter.recoClusterDeltaTimeCells"); // without MATH expressions
    auto hitFilterTypeName = ConfigManager::getScalar<std::string>(config, "simRecoHitFilter.recoClusterType"); // see CommonDefinitions.h
    auto hitFilterThreshold = ConfigManager::getScalar<double>(config, "simRecoHitFilter.recoClusterThreshold"); // [ADC units]
    auto hitFilterFraction = ConfigManager::getScalar<double>(config, "simRecoHitFilter.recoClusterConstantFractionThreshold"); // range [0,1]
    auto hitFilterDeltaStrips = ConfigManager::getScalar<int>(config, "simRecoHitFilter.recoClusterDeltaStrips");
    auto hitFilterDeltaTimeCells = ConfigManager::getScalar<int>(config, "simRecoHitFilter.recoClusterDeltaTimeCells");
    boost::property_tree::ptree aHitFilterConfig; // needed for pseudo RecHits per track
    aHitFilterConfig.put("hitFilter.recoClusterThreshold", hitFilterThreshold); // transient
    aHitFilterConfig.put("hitFilter.recoClusterConstantFractionThreshold", hitFilterFraction); // transient
    aHitFilterConfig.put("hitFilter.recoClusterDeltaStrips", hitFilterDeltaStrips); // transient
    aHitFilterConfig.put("hitFilter.recoClusterDeltaTimeCells", hitFilterDeltaTimeCells); // transient
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

	// set basic TrackSegment3D info
        aSegment.setGeometry(geometry);
        aSegment.setStartEnd(t.GetTruncatedStart(), t.GetTruncatedStop());
        aSegment.setPID(t.GetPrimaryParticle().GetID());

	// optionally store true RecHits per individual generator level TrackSegment3D
	if(event.trackPEvt.size()>=iTrack) {
	  pseudoRecoEventTPC.SetChargeMap(event.trackPEvt.at(iTrack-1).GetChargeMap());
	  aRecHitsColl.resize(0);
	  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
	    auto projType = get2DProjectionType(strip_dir);
	    aRecHitsColl.push_back( *(pseudoRecoEventTPC.get2DProjection(projType, pseudoRecoHitFilterType, scale_type::mm)) );
	    // 	  std::cout << KBLU << __FUNCTION__ << ": eventId=" << event.eventInfo.GetEventId()
	    //		    << ", track=" << iTrack
	    //		    << ": " << geometry->GetDirName(strip_dir) << "-dir charge = " << aRecHitsColl.back().Integral()
	    //		    << ", entries = " << aRecHitsColl.back().GetEntries()
	    //		    << RST << std::endl;
	  } // end of strip direction loop
	  aSegment.setRecHits(aRecHitsColl);
	} // end of optional RecHits per individual generator level TrackSegment3D

	// add TrackSegment3D info to Track3D collection
        aTrack.addSegment(aSegment);
    } // end of SimTrack loop
    event.track3D = aTrack;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Track3DBuilder::Finish() {
    return fwk::VModule::eSuccess;
}
