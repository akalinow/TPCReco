#include <map>
#include <iostream>
#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include "CommonDefinitions.h"

class ConfigManager
{
public:
    ConfigManager();
    boost::property_tree::ptree getConfig(int, char**);

    event_type getEventType(boost::property_tree::ptree tree);
    void setEventType(boost::property_tree::ptree &tree, event_type evtype);
    void setOnlineFlag(boost::property_tree::ptree& tree, bool flag) {};
    bool getOnlineFlag(boost::property_tree::ptree& tree) {};

private:
    boost::program_options::variables_map parseCmdLineArgs(int, char **);   
};