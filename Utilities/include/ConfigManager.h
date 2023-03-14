#include <map>
#include <iostream>
#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

class ConfigManager
{
    public:
    ConfigManager();
    void configureTree(boost::property_tree::ptree &, int, char**);
    void setEventType(boost::property_tree::ptree &tree, std::string evtype);
    std::string getEventType(boost::property_tree::ptree tree);

    private:
    boost::program_options::variables_map parseCmdLineArgs(int, char **);
};