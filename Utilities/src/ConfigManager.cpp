#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"


boost::property_tree::ptree ConfigManager::configTree;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigManager::string_code ConfigManager::hashit (std::string const& inString) const {
    if (inString == "int") return string_code::eint;
    if (inString == "unsigned int") return string_code::euint;
    if (inString == "float") return string_code::efloat;
    if (inString == "double") return string_code::edouble;
    if (inString == "string") return string_code::estr;
    if (inString == "bool") return string_code::ebool;
    return string_code::eunknown;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigManager::ConfigManager(){

std::string allowedOptPath = std::string(std::getenv("HOME"))+"/.tpcreco/config/allowedOptions.json";
parseAllowedArgs(allowedOptPath);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::parseAllowedArgs(const std::string & jsonFile){

    cmdLineOptDesc.add_options()("help", "produce help message");
        
    std::cout<<KBLU<<"ConfigManager: using config file "<<RST<<jsonFile<<std::endl
             <<KBLU<<"for a list of allowed command line arguments "
             <<"and default parameters values"
             <<RST<<std::endl;                           
    
    boost::property_tree::ptree paramsDescTree;
    boost::property_tree::read_json(jsonFile, paramsDescTree);
       
    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, paramsDescTree) {         
        
        std::string groupPath = v.first+".group";        
        std::string typePath = v.first+".type";
        std::string defaultValuePath = v.first+".defaultValue";
        std::string descriptionPath = v.first+".description";

        std::string group = paramsDescTree.get<std::string>(groupPath); 
        std::string optName = group+"."+v.first;     
        std::string type = paramsDescTree.get<std::string>(typePath);        
        std::string description = paramsDescTree.get<std::string>(descriptionPath);
        varTypeMap[optName] = hashit(type);
            
            switch(hashit(type)) {
            case string_code::eint:                
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                            boost::program_options::value<int>(), 
                                            description.c_str());
                configTree.put(optName, paramsDescTree.get<int>(defaultValuePath));                
                break;
            case string_code::euint:
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                            boost::program_options::value<unsigned int>(), 
                                            description.c_str());
                configTree.put(optName, paramsDescTree.get<unsigned int>(defaultValuePath));
                break;
            case string_code::efloat:
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                            boost::program_options::value<float>(),  
                                            description.c_str());
                configTree.put(optName, paramsDescTree.get<float>(defaultValuePath));
                break;
            case string_code::edouble:
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                            boost::program_options::value<double>(),   
                                            description.c_str());
                configTree.put(optName, paramsDescTree.get<double>(defaultValuePath));
                break;
            case string_code::ebool:
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                            boost::program_options::value<bool>(),   
                                            description.c_str());
                configTree.put(optName, paramsDescTree.get<bool>(defaultValuePath));
                break;
            case string_code::estr:
                cmdLineOptDesc.add_options()(optName.c_str(), 
                                             boost::program_options::value<std::string>(),   
                                             description.c_str());
                configTree.put(optName, paramsDescTree.get<std::string>(defaultValuePath));
                break;
            case string_code::eunknown:
                std::cout<<KRED<<"Unknown type of an argument "<<RST<<optName<<std::endl;
                break;
            }
        }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::program_options::variables_map & ConfigManager::parseCmdLineArgs(int argc, char **argv){

    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    boost::program_options::notify(varMap); 
    return varMap;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::updateWithJsonFile(const std::string jsonName){

    std::cout<<KBLU<<"Updating parameters with config file: "<<RST<<jsonName<<std::endl;
    boost::property_tree::ptree configTreeUpdate;
    boost::property_tree::read_json(jsonName, configTreeUpdate);
    mergeTrees(configTree, configTreeUpdate);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::updateWithCmdLineArgs(const boost::program_options::variables_map & varMap){

    std::cout<<KBLU<<"Updating parameters with command line arguments: "<<RST<<std::endl;
    
    for(const auto& item: varMap){        
        std::cout<<item.first<<std::endl;
        string_code type = varTypeMap.at(item.first);
        switch(type) {
            case string_code::eint:                
                configTree.put(item.first, item.second.as<int>());   
                break;
            case string_code::euint:
                configTree.put(item.first, item.second.as<unsigned int>());   
                break;
            case string_code::efloat:
                configTree.put(item.first, item.second.as<float>());   
                break;
            case string_code::edouble:
                configTree.put(item.first, item.second.as<double>());   
                break;
            case string_code::ebool:
                configTree.put(item.first, item.second.as<bool>());   
                break;
            case string_code::estr:
                configTree.put(item.first, item.second.as<std::string>());   
                break;
            case string_code::eunknown:
                std::cout<<KRED<<"Unknown type of for parameter: "<<RST<<item.first<<std::endl;
                break;
            }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::property_tree::ptree & ConfigManager::getConfig(int argc, char **argv){

    boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);

    if(argc<2){
        std::cout<<KBLU<<"Using default config file: defaultConfig.json"<<RST<<std::endl;
        return configTree;
    }
    else if (varMap.count("help")) {
        std::cout<<cmdLineOptDesc<<std::endl;
        return configTree;
    }
    else if(varMap.count("meta.configJson")){  
        std::string jsonName = varMap["meta.configJson"].as<std::string>();
        if(jsonName.size()) updateWithJsonFile(jsonName);
    }
    updateWithCmdLineArgs(varMap);
    return configTree;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::property_tree::ptree & ConfigManager::getConfig(){

    return configTree;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::mergeTrees(boost::property_tree::ptree& pt, const boost::property_tree::ptree& updates){
   BOOST_FOREACH( auto& update_lvl1, updates ){
      BOOST_FOREACH( auto& update_lvl2, updates.get_child(update_lvl1.first)){
        std::string name = update_lvl1.first+"."+update_lvl2.first; 
        pt.put(name, update_lvl2.second.data());
        }
   }   
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::dumpConfig(const std::string & jsonName){
    write_json(jsonName, configTree);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

