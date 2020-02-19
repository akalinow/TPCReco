#include <cstdlib>
#include <iostream>

#include "TCanvas.h"

#include "GeometryTPC.h"
#include "EventCharges.h"

#include "DataManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadDataFile(const std::string& fileName) {

	const std::string treeName = "TPCData";

	myFile.OpenFile(fileName.c_str(), "READ");
	if (myFile.IsOpen()) {
		std::cerr << "File: " << fileName << "not found!" << std::endl;
		return;
	}
	myTree = (TTree*)myFile.Get(treeName.c_str());
	if (myTree == nullptr) {
		throw std::runtime_error("ROOT tree not available!");
	}
	myTree->SetBranchAddress("Event", &currentEvent_internal); //MIGHT BE INCORRECT
	nEvents = myTree->GetEntries();
	loadTreeEntry(0);

	std::cout << "File: " << fileName << " loaded." << std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadTreeEntry(unsigned int iEntry) {

	if (myTree->GetEntries() <= iEntry) return;

	myTree->GetEntry(iEntry);
	currentEvent_external_copy = std::make_shared<EventCharges>(*currentEvent_internal);
	myCurrentEntry = iEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadEventId(unsigned int iEvent) {

	int iEntry = 0;
	while (currentEventNumber() != iEvent) {
		loadTreeEntry(iEntry);
		++iEntry;
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventCharges> DataManager::getCurrentEvent() const {

	return currentEvent_external_copy;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadNextEvent() {
	if (myCurrentEntry < nEvents) {
		loadTreeEntry(++myCurrentEntry);
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadPreviousEvent() {
	if (myCurrentEntry > 0) {
		loadTreeEntry(--myCurrentEntry);
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::numberOfEvents() const {

	return nEvents;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::currentEventNumber() const {

	if (currentEvent_external_copy != nullptr) {
		return currentEvent_external_copy->Info().EventId();
	}

	return 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
