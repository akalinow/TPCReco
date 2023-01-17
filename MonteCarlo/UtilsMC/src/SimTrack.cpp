#include "SimTrack.h"

#include <utility>


SimTrack::SimTrack()
    :A{0}, Z{0}, pdgID{0}, particleID{UNKNOWN}, energy{0}
{
	startPos=ROOT::Math::XYZPoint(0,0,0);
	stopPos=ROOT::Math::XYZPoint(0,0,0);
	momentum=ROOT::Math::XYZVector (0,0,0);
}

void SimTrack::SetStart(ROOT::Math::XYZPoint start)
{
	startPos=std::move(start);
}
void SimTrack::SetStart(double x, double y, double z)
{
	startPos=ROOT::Math::XYZPoint(x,y,z);
}
void SimTrack::SetStop(ROOT::Math::XYZPoint stop)
{
	stopPos=std::move(stop);
}
void SimTrack::SetStop(double x, double y, double z)
{
	stopPos=ROOT::Math::XYZPoint(x,y,z);
}
void SimTrack::SetMomentum(ROOT::Math::XYZVector mom)
{
	momentum=std::move(mom);
}
void SimTrack::SetMomentum(double px, double py, double pz)
{
	momentum=ROOT::Math::XYZVector (px,py,pz);
}
void SimTrack::SetA(unsigned int a)
{
	A=a;
}
void SimTrack::SetZ(unsigned int z)
{
	Z=z;
}
void SimTrack::SetPDG(unsigned int ID)
{
    pdgID=ID;
}
void SimTrack::SetEnergy(double E)
{
	energy=E;
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
void SimTrack::SetID(pid_type id)
{
    particleID=id;
}


