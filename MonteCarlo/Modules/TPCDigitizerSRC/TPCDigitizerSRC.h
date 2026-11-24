#ifndef TPCSOFT_TPCDIGITIZERSRC_H
#define TPCSOFT_TPCDIGITIZERSRC_H

#include "TPCReco/VModule.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/StripResponseCalculator.h"
#include "boost/filesystem.hpp"


class TPCDigitizerSRC : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:
    std::unique_ptr<eventraw::EventInfo> aEventInfo;
    std::unique_ptr<StripResponseCalculator> calculator;
    std::shared_ptr<PEventTPC> currentPEventTPC;

    uint32_t eventID{};
    double MeVToChargeScale{1};
    double diffSigmaXY{};
    double diffSigmaZ{};
    int th2PolyPartitionX{};
    int th2PolyPartitionY{};
    double peakingTime{};
    int nStrips{};
    int nCells{};
    int nPads{};
    boost::filesystem::path pathToResponses;
    bool enableSimHitsPerTrack{}; // used by Track3DBuilder to create true RecHits per individual generator level TrackSegment3D
    // optional drift-dependent diffusion on top of the strip response (default off): every hit is split into
    // driftSubCharges sub-charges, each displaced by a Gaussian of variance D^2 * d [D in mm/sqrt(cm), d in cm], where
    // d is the physical drift distance from the GEM plane: d = z_true - driftGEMZ, z_true = hit z before the
    // TriggerSimulator shift (the shift is recovered per event from the true and trigger-shifted vertex). The strip
    // response (sigmaXY, sigmaZ) is then the non-diffusion part: sigma^2(d) = sigma_resp^2 + D^2 * d.
    double driftDiffusionT{0}; // [mm/sqrt(cm)] transverse (X, Y)
    double driftDiffusionL{0}; // [mm/sqrt(cm)] longitudinal (Z)
    double driftGEMZ{0};       // [mm] z_true of the GEM plane, default: geometry drift cage Zmin (closest to the readout)
    int driftSubCharges{10};

    REGISTER_MODULE(TPCDigitizerSRC)
};


#endif //TPCSOFT_TPCDIGITIZERSRC_H
