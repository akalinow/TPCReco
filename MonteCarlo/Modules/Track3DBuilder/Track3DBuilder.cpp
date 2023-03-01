#include "Track3DBuilder.h"
#include "TRandom.h"

fwk::VModule::EResultFlag Track3DBuilder::Init(boost::property_tree::ptree config) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Track3DBuilder::Process(ModuleExchangeSpace &event) {
    auto &currentSimEvent = event.simEvt;
    TrackSegment3D aSegment;
    Track3D aTrack;
    //loop over tracks
    for (const auto &t: currentSimEvent.GetTracks()) {
        aSegment.setGeometry(geometry);
        aSegment.setStartEnd(t.GetStartTruncated(), t.GetStopTruncated());
        aSegment.setPID(t.GetPrimaryParticle().GetID());
        aTrack.addSegment(aSegment);
    }
    event.track3D = aTrack;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Track3DBuilder::Finish() {
    return fwk::VModule::eSuccess;
}
