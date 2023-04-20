#include <map>
#include <iostream>
#include <string>
#include <vector>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "CommonDefinitions.h"

class ConfigManager
{
public:
    ConfigManager();
    boost::property_tree::ptree getConfig(int argc, char** argv, std::vector<std::string> requiredOpt);

private:
    boost::program_options::variables_map parseCmdLineArgs(int argc, char ** argv);   
};