#include "SimTrack.h"


SimTrack::SimTrack()
        : stopPos{} {}

void SimTrack::SetStop(const TVector3 &stop) {
    stopPos = stop;
    hasStopPos = true;
}

void SimTrack::SetStart(const TVector3 &start) {
    startPos = start;
}

void SimTrack::SetTruncatedStop(const TVector3 &stop) {
    truncatedStopPos = stop;
}

void SimTrack::SetTruncatedStart(const TVector3 &start) {
    truncatedStartPos = start;
}


void SimTrack::InsertHit(const SimHit &hit) {
    hits.push_back(hit);
}

double SimTrack::GetEnergyDeposit() const {
    double edep = 0;
    for (const auto &hit: hits) {
        edep += hit.GetEnergy();
    }
    return edep;
}

double SimTrack::GetRange() const {
    if (hasStopPos)
        return (stopPos - startPos).Mag();
    else
        return 0;
}

double SimTrack::GetTruncatedRange() const {
    return (truncatedStopPos - truncatedStartPos).Mag();
}


void SimTrack::SortHits() {
    //sort by distance from the emission point
    auto hitCompare = [=](const SimHit &a, const SimHit &b) -> bool {
        auto dsta = (a.GetPosition() - startPos).Mag();
        auto dstb = (b.GetPosition() - startPos).Mag();
        return dsta < dstb;
    };

    std::sort(HitsBegin(), HitsEnd(), hitCompare);

}

void SimTrack::RecalculateStopPosition() {
    //set stop position to be equal to the position of the last hit
    if (!hits.empty())
        SetStop(hits[hits.size() - 1].GetPosition());
}

void SimTrack::Shift(TVector3 &offset) {
    startPos += offset;
    stopPos += offset;
    truncatedStartPos += offset;
    truncatedStopPos += offset;
    for (auto &h: hits) {
        h.SetPosition(h.GetPosition() + offset);
    }
}