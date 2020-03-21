#ifndef _EventSourceROOT_H_
#define _EventSourceROOT_H_

#include "EventSourceBase.h"
#include "EventCharges.h"

class TFile;
class TTree;

class EventSourceROOT : public EventSourceBase {
public:

	EventSourceROOT();

	~EventSourceROOT();

	void loadDataFile(const std::string& fileName);

	void loadFileEntry(unsigned long int iEntry);

private:

	std::shared_ptr<EventCharges> aPtr;
	std::string treeName;
	std::shared_ptr<TFile> myFile;
	std::shared_ptr<TTree> myTree;

};
#endif

