#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <map>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

#include "TPCReco/CommonDefinitions.h"

class ConfigManager
{
public:
    ConfigManager();
    
    const boost::property_tree::ptree & getConfig(int argc, char** argv);
    static const boost::property_tree::ptree & getConfig();
    
    void dumpConfig(const std::string & jsonName);
    
private:

    void parseAllowedArgs(const std::string & jsonFile);
    const boost::program_options::variables_map & parseCmdLineArgs(int argc, char ** argv);
    
    void updateWithJsonFile(const std::string jsonName);
    void updateWithCmdLineArgs(const boost::program_options::variables_map & varMap);
    
    void mergeTrees(boost::property_tree::ptree & tree, const boost::property_tree::ptree & updates);
    
    //helpers
    enum class string_code {
      eint,
      euint,
      efloat,
      edouble,
      estr,
      ebool,
      eunknown
    };
    string_code hashit (std::string const& ) const;
    
    boost::program_options::options_description cmdLineOptDesc;
    boost::program_options::variables_map varMap;
    std::map<std::string, string_code> varTypeMap;
    static boost::property_tree::ptree configTree;
};

#endif