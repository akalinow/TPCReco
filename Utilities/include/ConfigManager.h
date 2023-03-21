#include <map>
#include <iostream>
#include <string>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include "CommonDefinitions.h"

class ConfigManager : public boost::property_tree::ptree
{
public:
    ConfigManager();
    void configureTree(boost::property_tree::ptree, int, char**);//czy pracowa� na kopii czy oryginale?
    /*u�ycie:
    boost::property_tree::ptree tree;
    boost::property_tree::ptree tree1 = configureTree(tree);
    */
    void setEventType(boost::property_tree::ptree& tree, event_type evtype);

    event_type getEventType(boost::property_tree::ptree& tree);

    void setOnlineFlag(boost::property_tree::ptree& tree, bool flag) {};

    bool getOnlineFlag(boost::property_tree::ptree& tree) {};

    void configureTree(boost::property_tree::ptree &, int, char**);

    private:
    boost::program_options::variables_map parseCmdLineArgs(int, char **);
};