#include<cstdlib>
#include <iostream>

#include "EventSourceROOT.h"
#include "TFile.h"
#include "TTree.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::EventSourceROOT() {

	treeName = "TPCData";
	aPtr = myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::~EventSourceROOT() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadDataFile(const std::string& fileName) {

	EventSourceBase::loadDataFile(fileName);

	myFile = std::make_shared<TFile>(fileName.c_str(), "READ");
	if (myFile->IsOpen()) {
		std::cerr << "File: " << fileName << "not found!" << std::endl;
		exit(0);
	}

	myTree.reset((TTree*)myFile->Get(treeName.c_str()));
	myTree->SetBranchAddress("Event", &aPtr);
	nEvents = myTree->GetEntries();
	std::cout << "File: " << fileName << " loaded." << std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadFileEntry(unsigned long int iEntry) {

	if (!myTree) {
		std::cerr << "ROOT tree not available!" << std::endl;
		return;
	}
	if ((long int)iEntry >= myTree->GetEntries()) iEntry = myTree->GetEntries() - 1;

	myTree->GetEntry(iEntry);
	myCurrentEntry = iEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
