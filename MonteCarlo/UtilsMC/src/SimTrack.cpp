#include "SimTrack.h"

#include <utility>


SimTrack::SimTrack()
    :stopPos{} {}

void SimTrack::SetStop(ROOT::Math::XYZPoint stop)
{
	stopPos=std::move(stop);
}

void SimTrack::InsertHit(const SimHit& hit)
{
	hits.push_back(hit);
}
double SimTrack::GetEdep() const
{
	double edep=0;
	for(auto hit : hits)
	{
		edep+=hit.GetEnergy();
	}
    return edep;
}


