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
    boost::property_tree::ptree getConfig(int argc, char** argv);

    static const std::string masterConfigPath;
    static const std::string allowedOptPath;

private:
    boost::program_options::options_description parseAllowedArgs(std::string pathToFile);
    boost::program_options::variables_map parseCmdLineArgs(int argc, char ** argv);

    //helpers
    enum string_code {
      eint,
      euint,
      efloat,
      edouble,
      estr,
      ebool,
      eunknown
    };
    string_code hashit (std::string const& );
    typedef std::vector< std::tuple<std::string,std::string> > vecOfTuples;
    vecOfTuples allowedOptList (std::string pathToFile);   
};
