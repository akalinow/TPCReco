#ifndef _EventSourceBase_H_
#define _EventSourceBase_H_

#include <string>
#include <vector>
#include <memory>

#include "EventCharges.h"
#include "GeometryTPC.h"

class EventSourceBase {
public:

	EventSourceBase();

	virtual ~EventSourceBase() = default;

	bool isFileLoaded() const { return nEvents > 0; }

	void loadGeometry(const std::string& fileName);

	virtual void loadDataFile(const std::string& fileName);

	virtual void loadFileEntry(unsigned long int iEntry) = 0;

	void loadEventId(unsigned long int iEvent);

	std::string getCurrentPath() const;

	std::shared_ptr<EventCharges> getCurrentEvent() const;

	std::shared_ptr<EventCharges> getNextEvent();

	std::shared_ptr<EventCharges> getPreviousEvent();

	std::shared_ptr<EventCharges> getLastEvent();

	uint64_t numberOfEvents() const { return nEvents; };

	uint64_t currentEventNumber() const;

protected:

	std::string currentFilePath;

	uint64_t nEvents;
	uint64_t myCurrentEntry;

	std::shared_ptr<EventCharges> myCurrentEvent;

};
#endif

