#include "FromGeantGenerator.h"
FromGeantGenerator::FromGeantGenerator() : AbstractGenerator() {
}

void FromGeantGenerator::setEntry(int i) {
	depTree->GetEntry(i);
	eventNr = i;
}

void FromGeantGenerator::generateTrack() {
	for (uint32_t i = 0; i != x->size(); ++i) {
		myTrack3D.Fill(x->at(i), y->at(i), z->at(i), Edep->at(i));
	}
}

void FromGeantGenerator::generateEvents(int counts) {
	int eventsNo = 0;
	if (counts != 0) {
		eventsNo = counts;
	}
	else {
		eventsNo = depTree->GetEntries();
	}

	for (int i = 0; i != eventsNo; ++i) {
		std::cout << "Event " << i << " ..." << std::endl;
		setEntry(i);
		generateEvent();
		if (outputFile->IsOpen()) {
			outputTree->Fill();
		}
	}
}

void FromGeantGenerator::loadDataFile(std::string dataFileAddress) {
	dataFile = new TFile(dataFileAddress.c_str(), "READ");
	dataFile->GetObject("EDep", depTree);
	dataFile->GetObject("prim", primTree);
	setBranches();

}

void FromGeantGenerator::setBranches() {
	depTree->SetBranchAddress("x", &x);
	depTree->SetBranchAddress("y", &y);
	depTree->SetBranchAddress("z", &z);
	depTree->SetBranchAddress("Edep", &Edep);
}