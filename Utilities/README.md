# ConfigManager class
This is a class that handles parsing cmd line and config files arguments
## Methods
- boost::program_options::options_description parseAllowedArgs(std::string pathToFile) - a method that is used to parse list of allowed options from the JSON file provided (by default Utilities/config/allowedOpt.json)
- boost::program_options::variables_map parseCmdLineArgs(int argc, char ** argv) - a method used for parsing cmd line arguments
- boost::property_tree::ptree getConfig(int argc, char** argv) - a method that returns a configured property tree using parameters given in config file or in cmd line (if none given, method uses default file Utilities/config/masterconfig.json) 

## Helper functions
- vecOfTuples allowedOptList (std::string pathToFile)
- string_code hashit (std::string const& );