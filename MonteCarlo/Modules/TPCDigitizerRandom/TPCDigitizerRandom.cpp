#include "TPCDigitizerRandom.h"
#include "TRandom.h"
#include "TPCReco/ConfigManager.h"
#include <cmath>
#include <tuple>
#include "TH2Poly.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include <algorithm>
#include <string>

fwk::VModule::EResultFlag TPCDigitizerRandom::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    aEventInfo = std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);
    // MeVToChargeScale = config.get<double>("MeVToChargeScale"); // without MATH expressions
    // diffSigmaXYmin = config.get<double>("sigmaXYmin"); // without MATH expressions
    // diffSigmaXYmax = config.get<double>("sigmaXYmax"); // without MATH expressions
    // diffSigmaZmin = config.get<double>("sigmaZmin"); // without MATH expressions
    // diffSigmaZmax = config.get<double>("sigmaZmax"); // without MATH expressions
    // nSamplesPerHit = config.get<unsigned int>("NSamplesPerHit"); // without MATH expressions
    // enableSimHitsPerTrack = config.get<bool>("transient.enableSimHitsPerTrack"); // enabled when Track3DBuilder MC module is present
    MeVToChargeScale = ConfigManager::getScalar<double>(config, "MeVToChargeScale"); // [ADC counts/MeV]
    diffSigmaXYmin = ConfigManager::getScalar<double>(config, "sigmaXYmin"); // [mm]
    diffSigmaXYmax = ConfigManager::getScalar<double>(config, "sigmaXYmax"); // [mm]
    diffSigmaZmin = ConfigManager::getScalar<double>(config, "sigmaZmin"); // [mm]
    diffSigmaZmax = ConfigManager::getScalar<double>(config, "sigmaZmax"); // [mm]
    nSamplesPerHit = ConfigManager::getScalar<unsigned int>(config, "NSamplesPerHit");
    enableSimHitsPerTrack = ConfigManager::getScalar<bool>(config, "transient.enableSimHitsPerTrack"); // enabled when Track3DBuilder MC module is present
    driftDiffusionT = config.get_optional<std::string>("driftDiffusionT") ? ConfigManager::getScalar<double>(config, "driftDiffusionT") : 0.0;
    driftDiffusionL = config.get_optional<std::string>("driftDiffusionL") ? ConfigManager::getScalar<double>(config, "driftDiffusionL") : 0.0;
    driftGEMZ = config.get_optional<std::string>("driftGEMZ") ? ConfigManager::getScalar<double>(config, "driftGEMZ") : geometry->GetDriftCageZmin();
    if(driftDiffusionT<0 || driftDiffusionL<0) throw std::runtime_error("TPCDigitizerRandom: invalid drift diffusion settings");
    lookupCell = config.get_optional<std::string>("stripLookupCellMM") ? ConfigManager::getScalar<double>(config, "stripLookupCellMM") : 0.05;
    buildLookup();
    poolNCells = geometry->GetAgetNtimecells();
    poolNBins = geometry->GetTH2Poly()->GetNumberOfBins();
    pool.assign(static_cast<size_t>(poolNBins + 1) * poolNCells, 0.0);
    poolTouched.clear();
    cellC0 = poolNCells - geometry->GetSamplingRate() * geometry->GetTriggerDelay();
    cellC1 = geometry->GetSamplingRate() / (geometry->GetDriftVelocity() * 10.0);
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    auto &currentPEventTPC = event.tpcPEvt;
    currentPEventTPC.Clear();

    // used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
    auto iTrack=0U; // track iterator for event.trackPEvt[] vector 
    if(enableSimHitsPerTrack) {
      event.trackPEvt.resize(currentSimEvent.GetTracks().size());
      for(auto &pEvt : event.trackPEvt) {
	pEvt.Clear();
      }
    }

    const int nCells = poolNCells;
    // z shift applied by TriggerSimulator (0 without it): z_true = z - triggerShiftZ
    const double triggerShiftZ = currentSimEvent.GetTrigShiftedVertexPosition().Z() - currentSimEvent.GetTrueVertexPosition().Z();
    //loop over tracks
    // diffsigmaXY = rand->Gaus(0, diffSigmaXY);
    diffSigmaXY = gRandom->Uniform(diffSigmaXYmin, diffSigmaXYmax); // TODO: replace by diffsigmaXY(Z_DET)
    diffSigmaZ = gRandom->Uniform(diffSigmaZmin, diffSigmaZmax); // TODO: replace by diffsigmaZ(Z_DET)
    for (auto &t: currentSimEvent.GetTracks()) {
        iTrack++;
        //loop over hits
        for (auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            auto isIn = geometry->IsInsideActiveVolume(pos);
            h.SetInside(isIn);
            if(isIn) {
                const double drift = std::max(0.0, pos.Z() - triggerShiftZ - driftGEMZ) / 10.0; // [cm] physical drift distance
                const double sXY = std::sqrt(diffSigmaXY * diffSigmaXY + driftDiffusionT * driftDiffusionT * drift);
                const double sZ = std::sqrt(diffSigmaZ * diffSigmaZ + driftDiffusionL * driftDiffusionL * drift);
                const double q = edep / nSamplesPerHit * MeVToChargeScale;
                for (unsigned int i = 0; i < nSamplesPerHit; i++) {
                    // drawn in the order of the previous TVector3(Gaus(x), Gaus(y), Gaus(z)) as compiled by GCC
                    // (arguments evaluated right to left): same random sequence, same events for a given seed
                    const double z = gRandom->Gaus(pos.Z(), sZ);
                    const double y = gRandom->Gaus(pos.Y(), sXY);
                    const double x = gRandom->Gaus(pos.X(), sXY);
                    if (q <= 0) continue;
                    const int iPolyBin = findPolyBin(x, y);
                    if (iPolyBin < 1 || iPolyBin > poolNBins) continue;
                    // nearest time cell: cell i covers [i-0.5, i+0.5) in fractional cells, as in the reconstruction
                    // histograms (EventTPC: bin centre of cell i at Timecell2pos(i))
                    const double fCell = cellC0 + cellC1 * z;
                    if (!(fCell >= -0.5 && fCell < nCells - 0.5)) continue;
                    const int iCell = static_cast<int>(std::floor(fCell + 0.5));
                    const size_t idx = static_cast<size_t>(iPolyBin) * nCells + iCell;
                    if (pool[idx] == 0) poolTouched.push_back(static_cast<int>(idx));
                    pool[idx] += q;
                }
            }
        }
        // flush the pooled charge of this track
        for (const int idx : poolTouched) {
            auto strip = geometry->GetTH2PolyStrip(idx / nCells);
            if (strip) {
                currentPEventTPC.AddValByStrip(strip, idx % nCells, pool[idx]);
                // used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
                if(enableSimHitsPerTrack) event.trackPEvt.at(iTrack-1).AddValByStrip(strip, idx % nCells, pool[idx]);
            }
            pool[idx] = 0;
        }
        poolTouched.clear();
    }
    currentPEventTPC.SetEventInfo(*aEventInfo);
    event.eventInfo = *aEventInfo;
    return fwk::VModule::eSuccess;
}

// rasterize the strip polygons of the geometry TH2Poly into the lookup table (see the header)
void TPCDigitizerRandom::buildLookup() {
    lookupBin.clear(); lookupNx = lookupNy = 0;
    if(lookupCell <= 0) return;
    auto *poly = geometry->GetTH2Poly();
    if(poly->GetNumberOfBins() > INT16_MAX) throw std::runtime_error("TPCDigitizerRandom: too many TH2Poly bins for the lookup table");
    double xmin, xmax, ymin, ymax;
    std::tie(xmin, xmax, ymin, ymax) = geometry->rangeXY();
    lookupX0 = xmin - 1.0; lookupY0 = ymin - 1.0;   // 1 mm margin
    lookupNx = static_cast<long>(std::ceil((xmax - xmin + 2.0) / lookupCell));
    lookupNy = static_cast<long>(std::ceil((ymax - ymin + 2.0) / lookupCell));
    lookupBin.assign(lookupNx * lookupNy, int16_t(-1));
    std::vector<const TGraph*> graphs;
    std::vector<double> xs;
    for(auto *obj : *poly->GetBins()) {
        auto *bin = static_cast<TH2PolyBin*>(obj);
        graphs.clear();
        auto *shape = bin->GetPolygon();
        if(auto *mg = dynamic_cast<TMultiGraph*>(shape)) { for(auto *g : *mg->GetListOfGraphs()) graphs.push_back(static_cast<TGraph*>(g)); }
        else if(auto *g = dynamic_cast<TGraph*>(shape)) graphs.push_back(g);
        for(const auto *g : graphs) {
            const int n = g->GetN();
            if(n < 3) continue;
            const double *gx = g->GetX(), *gy = g->GetY();
            const double gymin = *std::min_element(gy, gy + n), gymax = *std::max_element(gy, gy + n);
            const long iy0 = std::max(0L, static_cast<long>(std::ceil((gymin - lookupY0) / lookupCell - 0.5)));
            const long iy1 = std::min(lookupNy - 1, static_cast<long>(std::floor((gymax - lookupY0) / lookupCell - 0.5)));
            for(long iy = iy0; iy <= iy1; iy++) {
                const double yc = lookupY0 + (iy + 0.5) * lookupCell;
                xs.clear();
                for(int i = 0, j = n - 1; i < n; j = i++) {   // edges (j, i), including the closing one
                    if((gy[i] > yc) != (gy[j] > yc)) xs.push_back(gx[j] + (yc - gy[j]) * (gx[i] - gx[j]) / (gy[i] - gy[j]));
                }
                std::sort(xs.begin(), xs.end());
                for(size_t k = 0; k + 1 < xs.size(); k += 2) {   // cell centres inside [xs[k], xs[k+1])
                    const long ix0 = std::max(0L, static_cast<long>(std::ceil((xs[k] - lookupX0) / lookupCell - 0.5)));
                    const long ix1 = std::min(lookupNx - 1, static_cast<long>(std::ceil((xs[k + 1] - lookupX0) / lookupCell - 0.5)) - 1);
                    for(long ix = ix0; ix <= ix1; ix++) lookupBin[iy * lookupNx + ix] = static_cast<int16_t>(bin->GetBinNumber());
                }
            }
        }
    }
}

// TH2Poly bin of the strip at (x, y) [mm]; < 1: no strip
int TPCDigitizerRandom::findPolyBin(double x, double y) {
    if(lookupNx > 0) {
        const long ix = static_cast<long>(std::floor((x - lookupX0) / lookupCell));
        const long iy = static_cast<long>(std::floor((y - lookupY0) / lookupCell));
        if(ix >= 0 && ix < lookupNx && iy >= 0 && iy < lookupNy) return lookupBin[iy * lookupNx + ix];
    }
    return geometry->GetTH2Poly()->FindBin(x, y);
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Finish() {
    return fwk::VModule::eSuccess;
}
