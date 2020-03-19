#include "SimHit.hh"


SimHit::SimHit()
{
	position=TVector3(0,0,0);
	Edep=0;
}

SimHit::SimHit(TVector3 pos, double edep)
{
	position=pos;
	Edep=edep;
}

SimHit::SimHit(double x, double y, double z, double edep)
{
	position=TVector3(x,y,z);
	Edep=edep;
}

void SimHit::SetPosition(TVector3 pos)
{
	position=pos;
}

void SimHit::SetEnergy(double E)
{
	Edep=E;
}

TVector3 SimHit::GetPosition()
{
	return position;
}
double SimHit::GetEnergy()
{
	return Edep;
}