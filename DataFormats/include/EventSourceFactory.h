#pragma once
#include "EventSourceBase.h"
#include <boost/property_tree/json_parser.hpp>
#include "GUI_commons.h"

class EventSourceFactory {
private:
	EventSourceFactory() {};

public:
	~EventSourceFactory() {};

	static std::shared_ptr<EventSourceBase> makeEventSourceObject(boost::property_tree::ptree& myConfig);
};