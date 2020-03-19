#include "SimTrack.hh"


SimTrack::SimTrack()
{
	startPos=TVector3(0,0,0);
	stopPos=TVector3(0,0,0);
	momentum=TVector3(0,0,0);
	A=0;
	Z=0;
	particleID=0;
	energy=0;

}

void SimTrack::SetStart(TVector3 start)
{
	startPos=start;
}
void SimTrack::SetStart(double x, double y, double z)
{
	startPos=TVector3(x,y,z);
}
void SimTrack::SetStop(TVector3 stop)
{
	stopPos=stop;
	length=(stopPos-startPos).Mag();
}
void SimTrack::SetStop(double x, double y, double z)
{
	stopPos=TVector3(x,y,z);
}
void SimTrack::SetMomentum(TVector3 mom)
{
	momentum=mom;
}
void SimTrack::SetMomentum(double px, double py, double pz)
{
	momentum=TVector3(px,py,pz);
}
void SimTrack::SetA(unsigned int a)
{
	A=a;
}
void SimTrack::SetZ(unsigned int z)
{
	Z=z;
}
void SimTrack::SetID(unsigned int ID)
{
	particleID=ID;
}
void SimTrack::SetEnergy(double E)
{
	energy=E;
}
void SimTrack::InsertHit(SimHit hit)
{
	hits.push_back(hit);
}

SimHits SimTrack::GetHits()
{
	return hits;
}

double SimTrack::GetEnergy()
{
	return energy;
}

	unsigned int GetNHits();

SimHitsIterator SimTrack::HitsBegin()
{
	return hits.begin();
}

SimHitsIterator SimTrack::HitsEnd()
{
	return hits.end();
}

double SimTrack::GetLength()
{
	return length;
}

double SimTrack::GetEdep()
{
	double edep=0;
	for(auto hit : hits)
	{
		edep+=hit.GetEnergy();
	}
}