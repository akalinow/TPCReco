#include "FromTransportGenerator.h"
FromTransportGenerator::FromTransportGenerator() : AbstractGenerator() {
}

void FromTransportGenerator::generateTrack() {
	myTrack3D = *(reinterpret_cast<TH3D*>(simEvent->GetAfterTransportHisto()));
	clearParameters();
	tracksNo = simEvent->GetTracks().size();
	for (auto& i : simEvent->GetTracks()) {
		A.push_back(i.GetA());
		Z.push_back(i.GetZ());
		momentum.push_back(i.GetMomentum());
		start.push_back(i.GetStart());
		stop.push_back(i.GetStop());
		energy.push_back(i.GetEnergy());
		length.push_back(i.GetLength());
	}
}

void FromTransportGenerator::loadDataFile(std::string dataFileAddress) {
	dataFile = new TFile(dataFileAddress.c_str(), "READ");
	dataFile->GetObject("t", dataTree);
	setBranches();
}

void FromTransportGenerator::setBranches() {
	dataTree->SetBranchAddress("evt", &simEvent);
}

void FromTransportGenerator::setEntry(int i) {
	dataTree->GetEntry(i);
	eventNr = i;
}

void FromTransportGenerator::generateEvents(int counts) {
	int eventsNo = 0;
	if (counts) {
		eventsNo = counts;
	}
	else {
		eventsNo = dataTree->GetEntries();
	}

	for (int i = 1; i <= eventsNo; ++i) {
		std::cout << "Event " << i << " ..." << std::endl;
		setEntry(i);
		generateEvent();
		if (outputFile->IsOpen()) {
			outputTree->Fill();
		}
	}
}