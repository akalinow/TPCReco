#ifndef _grawToEventTPC_H_
#define _grawToEventTPC_H_

#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

int convertGRAWFile(boost::property_tree::ptree & aConfig);

#endif