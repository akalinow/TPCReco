#ifndef _ConvertGrawFile_h_
#define _ConvertGrawFile_h_

#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

int convertGRAWFile(boost::property_tree::ptree & aConfig);

#endif
