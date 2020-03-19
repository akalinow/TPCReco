/**
 * @file SimEvent.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of SimEvent class and PrimaryParticle struct
 */

#include "SimEvent.hh"
/// \cond
#include<iostream>
/// \endcond
//ClassImp(SimEvent);


unsigned int SimEvent::nBinsX=350;
unsigned int SimEvent::nBinsY=200;
unsigned int SimEvent::nBinsZ=200;
double SimEvent::xLow=-175;
double SimEvent::xUp=175;
double SimEvent::yLow=-100;
double SimEvent::yUp=100;
double SimEvent::zLow=-100;
double SimEvent::zUp=100;

int SimEvent::eventID=0;

SimEvent::~SimEvent()
{
	
	if(hPrimaryEnergyDeposit!=nullptr)
		delete hPrimaryEnergyDeposit;
	if(hEnergyDepositAfterTransport!=nullptr)
		delete hEnergyDepositAfterTransport;
}

SimEvent::SimEvent()
{

	hPrimaryEnergyDeposit=new TH3F(TString(eventID++),"Energy deposit",nBinsX,xLow,xUp,nBinsY,yLow,yUp,nBinsZ,zLow,zUp);
	hEnergyDepositAfterTransport=new TH3F(TString(eventID++)+"_transport","Energy deposit",nBinsX,xLow,xUp,nBinsY,yLow,yUp,nBinsZ,zLow,zUp);
	hPrimaryEnergyDeposit->GetXaxis()->SetTitle("x [mm]");
	hPrimaryEnergyDeposit->GetYaxis()->SetTitle("y [mm]");
	hPrimaryEnergyDeposit->GetZaxis()->SetTitle("z [mm]");
	hEnergyDepositAfterTransport->GetXaxis()->SetTitle("x [mm]");
	hEnergyDepositAfterTransport->GetYaxis()->SetTitle("y [mm]");
	hEnergyDepositAfterTransport->GetZaxis()->SetTitle("z [mm]");
	SetDiffusion(0);
	SetAttachment(0);
	SetGain(0);
}

SimEvent::SimEvent(SimTracks tracks)
{
	hPrimaryEnergyDeposit=new TH3F(TString(eventID++),"Energy deposit",nBinsX,xLow,xUp,nBinsY,yLow,yUp,nBinsZ,zLow,zUp);
	hEnergyDepositAfterTransport=new TH3F(TString(eventID++)+"_transport","Energy deposit",nBinsX,xLow,xUp,nBinsY,yLow,yUp,nBinsZ,zLow,zUp);
	hPrimaryEnergyDeposit->GetXaxis()->SetTitle("x [mm]");
	hPrimaryEnergyDeposit->GetYaxis()->SetTitle("y [mm]");
	hPrimaryEnergyDeposit->GetZaxis()->SetTitle("z [mm]");
	hEnergyDepositAfterTransport->GetXaxis()->SetTitle("x [mm]");
	hEnergyDepositAfterTransport->GetYaxis()->SetTitle("y [mm]");
	hEnergyDepositAfterTransport->GetZaxis()->SetTitle("z [mm]");
	this->tracks=tracks;
	for(auto track : tracks)
	{
		for(auto hit : track.GetHits())
			Fill(hit.GetPosition().x(), hit.GetPosition().y(), hit.GetPosition().z(), hit.GetEnergy());
	}
	SetDiffusion(0);
	SetAttachment(0);
	SetGain(0);
}

void SimEvent::SetHistogram(unsigned int nx, double x1, double x2, unsigned int ny, double y1, double y2, unsigned int nz, double z1, double z2)
	{
		xLow=x1;
		xUp=x2;
		yLow=y1;
		yUp=y2;
		zLow=z1;
		zUp=z2;
		nBinsX=nx;
		nBinsY=ny;
		nBinsZ=nz;
	}
void SimEvent::Fill(double x, double y, double z, double w) 
{
	hPrimaryEnergyDeposit->Fill(x,y,z,w);
}

TH3F* SimEvent::GetPrimaryHisto()
{
	return hPrimaryEnergyDeposit;
}
TH3F* SimEvent::GetAfterTransportHisto()
{
	return hEnergyDepositAfterTransport;
}


SimTracks SimEvent::GetTracks()
{
	return tracks;
}

SimTracksIterator SimEvent::TracksBegin()
{
	return tracks.begin();
}

SimTracksIterator SimEvent::TracksEnd()
{
	return tracks.end();
}

void SimEvent::Clear()
{
	if(hEnergyDepositAfterTransport)
		hEnergyDepositAfterTransport->Reset();
	if(hPrimaryEnergyDeposit)
		hPrimaryEnergyDeposit->Reset();
	tracks.clear();
	SetDiffusion(0);
	SetAttachment(0);
	SetGain(0);
}



void SimEvent::SetSimTracks(SimTracks trcks)
{
	tracks=trcks;
}

double SimEvent::PrimaryIntegral()
{
	return hPrimaryEnergyDeposit->Integral();
}

double SimEvent::AfterTransportIntegral()
{
	return hEnergyDepositAfterTransport->Integral();
}