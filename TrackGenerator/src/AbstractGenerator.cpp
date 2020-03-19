#include "AbstractGenerator.h"

AbstractGenerator::AbstractGenerator() {
	persistentEvent = &myEvent;
}

void AbstractGenerator::setTrackSpace(int NbinsX, double xmin, double xmax,
	int NbinsY, double ymin, double ymax,
	int NbinsZ, double zmin, double zmax) {
	myTrack3D = TH3D("h_xz", "Pseudo data;X [mm];Z [mm];Charge/bin [arb.u.]",
		NbinsX, xmin, xmax,
		NbinsY, ymin, ymax,
		NbinsZ, zmin, zmax);
}


void AbstractGenerator::loadGeometry(std::shared_ptr<GeometryTPC> geometryPtr) {
	myProjectorPtr = std::make_unique<UVWprojector>();
	myEvent.Clear();
}

void AbstractGenerator::loadGeometry(const std::string& fileName) {
	loadGeometry(std::make_shared<GeometryTPC>(fileName.c_str()));
}

EventCharges& AbstractGenerator::generateEvent() {
	generateTrack();
	project();
	fillEvent();
	return myEvent;
}


void AbstractGenerator::project() {

	myProjectorPtr->SetEvent3D(myTrack3D);
	for (auto dir : dir_vec_UVW) {
		projectionsCollection[dir] = myProjectorPtr->GetStripVsTime_TH2D(dir);
	}
}

void AbstractGenerator::fillEvent() {
	myEvent.Clear();
	for (auto dir : dir_vec_UVW) {
		auto hist = projectionsCollection.at(dir);
		for (int i = 1; i != hist->GetNbinsX(); ++i) {
			for (int j = 1; j != hist->GetNbinsY(); ++j) {
				myEvent.AddValByStrip(Geometry().GetStripByDir(dir, j), i - 1, hist->GetBinContent(i, j)); //might work, might not
			}
		}
		//hist->Delete();
	}
	myEvent().EventId(eventNr);
}

void AbstractGenerator::setOutput(std::string outputName) {
	outputFile = new TFile(outputName.c_str(), "RECREATE");
	outputTree = new TTree("TPCData", "");
	outputTree->Branch("Event", &persistentEvent);
	outputTree->Branch("tracksNo", &tracksNo);
	outputTree->Branch("A", &A);
	outputTree->Branch("Z", &Z);
	outputTree->Branch("momentum", &momentum);
	outputTree->Branch("start", &start);
	outputTree->Branch("stop", &stop);
	outputTree->Branch("energy", &energy);
	outputTree->Branch("length", &length);
}
void AbstractGenerator::writeOutput() {
	//outputTree->Write();
	outputFile->WriteTObject(outputTree);
}

void AbstractGenerator::setEntry(int i) {
	eventNr = i;
}

void AbstractGenerator::clearParameters() {
	tracksNo = 0;
	A.clear();
	Z.clear();
	momentum.clear();
	start.clear();
	stop.clear();
	energy.clear();
	length.clear();
}