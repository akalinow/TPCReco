#include "SimTrack.h"

#include <utility>


SimTrack::SimTrack()
        : stopPos{} {}

void SimTrack::SetStop(TVector3 &stop) {
    stopPos = stop;
    hasStopPos=true;
}

void SimTrack::SetStart(TVector3 &start) {
    startPos = start;
}

void SimTrack::InsertHit(const SimHit &hit) {
    hits.push_back(hit);
}

double SimTrack::GetEdep() const {
    double edep = 0;
    for (const auto& hit: hits)
    {
        edep += hit.GetEnergy();
    }
    return edep;
}

double SimTrack::GetLength() const {
    if(hasStopPos)
        return (stopPos - startPos).Mag();
    else
        return 0;
}


