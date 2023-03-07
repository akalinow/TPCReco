#include<map>
#include<iostream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

class ConfigManager
{
    public:
    ConfigManager();
    void configureTree(boost::property_tree::ptree, int, char**);//czy pracować na kopii czy oryginale?
    /*użycie:
    boost::property_tree::ptree tree;
    boost::property_tree::ptree confTree = configureTree(tree); 
    */
    private:
    boost::program_options::variables_map parseCmdLineArgs(int, char **);
};