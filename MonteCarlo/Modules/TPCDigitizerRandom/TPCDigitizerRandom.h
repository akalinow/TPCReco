#ifndef TPCSOFT_TPCDIGITIZERRANDOM_H
#define TPCSOFT_TPCDIGITIZERRANDOM_H

#include "TPCReco/VModule.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"
#include <cstdint>
#include <vector>


class TPCDigitizerRandom : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:
    std::unique_ptr<eventraw::EventInfo> aEventInfo;
    uint32_t eventID{};
    double MeVToChargeScale{1};
    double diffSigmaXY{};
    double diffSigmaZ{};
    double diffSigmaXYmin{};
    double diffSigmaXYmax{};
    double diffSigmaZmin{};
    double diffSigmaZmax{};
    unsigned int nSamplesPerHit{};
    bool enableSimHitsPerTrack{}; // used by Track3DBuilder to create true RecHits per individual generator level TrackSegment3D
    // optional drift-dependent diffusion (default off), as in TPCDigitizerSRC: every sample is smeared in addition by a
    // Gaussian of variance D^2 * d [D in mm/sqrt(cm), d in cm], d = physical drift distance from the GEM plane,
    // d = z_true - driftGEMZ, z_true = z before the TriggerSimulator shift. sigma^2 = sigma_event^2 + D^2 * d.
    double driftDiffusionT{0}; // [mm/sqrt(cm)] transverse (X, Y)
    double driftDiffusionL{0}; // [mm/sqrt(cm)] longitudinal (Z)
    double driftGEMZ{0};       // [mm] z_true of the GEM plane, default: geometry drift cage Zmin (closest to the readout)
    // strip lookup table: TH2Poly::FindBin on the strip map (1018 strip polygons of ~355 vertices each) dominated the
    // run time. At Init the UVW active area is divided into square cells of stripLookupCellMM (optional, default
    // 0.05 mm; <= 0 disables the table) and every strip polygon is rasterized into it (scanline, even-odd rule at the
    // cell centres, as TGraph::IsInside). A sample is assigned to the strip of its cell's centre: strip boundaries are
    // displaced by at most half a cell. Points outside the table area use FindBin.
    double lookupCell{0.05};   // [mm]
    double lookupX0{0}, lookupY0{0};
    long lookupNx{0}, lookupNy{0};
    std::vector<int16_t> lookupBin; // TH2Poly bin number, -1 = no strip
    void buildLookup();
    // charge pooling: the samples of one track are summed in a flat (TH2Poly bin, time cell) buffer and written to the
    // PEventTPC objects once per touched entry at the end of the track (one map insertion per entry instead of one per
    // sample). poolTouched lists the non-zero entries.
    int poolNCells{0};
    int poolNBins{0};
    std::vector<double> pool;
    std::vector<int> poolTouched;
    // time cell of z: fractional cell = cellC0 + cellC1 * z (GeometryTPC::Pos2timecell, precomputed)
    double cellC0{0}, cellC1{0};
    int findPolyBin(double x, double y);

    REGISTER_MODULE(TPCDigitizerRandom)
};


#endif //TPCSOFT_TPCDIGITIZERRANDOM_H
