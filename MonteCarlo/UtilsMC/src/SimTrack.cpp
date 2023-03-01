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

void SimTrack::SetStopTruncated(const TVector3 &stop) {
    stopPosTruncated = stop;
}

void SimTrack::SetStartTruncated(const TVector3 &start) {
    startPosTruncated = start;
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

double SimTrack::GetLength() const {
    if (hasStopPos)
        return (stopPos - startPos).Mag();
    else
        return 0;
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
    startPosTruncated += offset;
    stopPosTruncated += offset;
    for (auto &h: hits) {
        h.SetPosition(h.GetPosition() + offset);
    }
}
