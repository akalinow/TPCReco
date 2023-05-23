#pragma once
#include "EventSourceBase.h"
#include <boost/property_tree/json_parser.hpp>
#include "GUI_commons.h"

namespace EventSourceFactory {
	std::shared_ptr<EventSourceBase> makeEventSourceObject(boost::property_tree::ptree& myConfig);
};