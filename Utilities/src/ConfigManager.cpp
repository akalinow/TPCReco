#include "ConfigManager.h"

const std::string ConfigManager::masterConfigPath = "/scratch/TPCReco/Utilities/config/masterConfig.json";
const std::string ConfigManager::allowedOptPath = "/scratch/TPCReco/Utilities/config/allowedOpt.json";

typedef std::vector< std::tuple<std::string,std::string> > vecOfTuples;

enum string_code {
    eint,
    euint,
    efloat,
    edouble,
    estr,
    ebool,
    //evecstr
    eunknown
};

ConfigManager::string_code ConfigManager::hashit (std::string const& inString) {
    if (inString == "int") return eint;
    if (inString == "unsigned int") return euint;
    if (inString == "float") return efloat;
    if (inString == "double") return edouble;
    if (inString == "std::string") return estr;
    if (inString == "bool") return ebool;
    //if (inString == "std::vector<std::string>") return evecstr;
    return eunknown;
}

ConfigManager::ConfigManager(){}

//helper function
vecOfTuples ConfigManager::allowedOptList (std::string pathToFile = "none"){
    boost::program_options::options_description cmdLineOptDesc("Allowed options");
    boost::property_tree::ptree optionsTree;
    if(pathToFile =="none"){
        boost::property_tree::read_json(ConfigManager::allowedOptPath, optionsTree);
    }
    else{
        boost::property_tree::read_json(pathToFile, optionsTree);
    }

    std::tuple<std::string, std::string> optDesc;
    vecOfTuples optList;

    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, optionsTree.get_child("Options")) {
        // v.first is the name of the child.
        // v.second is the child tree.
        std::string optName = v.first;
        std::string childPath = "Options."+optName;
        boost::property_tree::ptree childTree = v.second;

        std::string typePath = childPath+".type";

        std::string type= childTree.get_value<std::string>(typePath);

        optDesc = make_tuple(optName,type);
        optList.push_back(optDesc);
    }
    return optList;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

boost::program_options::options_description ConfigManager::parseAllowedArgs(std::string pathToFile = "none"){
    boost::program_options::options_description cmdLineOptDesc("Allowed options");
    boost::property_tree::ptree optionsTree;
    if(pathToFile =="none"){
        std::cout<<"Using config file allowedOpt.json"<<std::endl;
        boost::property_tree::read_json(ConfigManager::allowedOptPath, optionsTree);
    }
    else{
        std::cout<<"Using config file "<<pathToFile<<std::endl;
        boost::property_tree::read_json(pathToFile, optionsTree);
    }

    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, optionsTree.get_child("Options")) {

        std::string optName = v.first;
        std::string childPath = "Options."+optName;
        boost::property_tree::ptree childTree = v.second;

        std::string typePath = childPath+".type";
        std::string defaultValuePath = childPath + ".defaultValue";
        std::string descriptionPath = childPath + ".description";
        std::string isRequiredPath = childPath + ".isRequired";

        std::string type= childTree.get_value<std::string>(typePath);
        std::string defaultValue= childTree.get_value<std::string>(defaultValuePath);
        std::string description= childTree.get_value<std::string>(descriptionPath);
        std::string isRequired= childTree.get_value<std::string>(isRequiredPath);

        if(isRequired == "true"){
            switch(hashit(type)) {
            case eint:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<int>()->required(), description.c_str());
                break;
            case euint:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<unsigned int>()->required(), description.c_str());
                break;
            case efloat:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<float>()->required(), description.c_str());
                break;
            case edouble:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<double>()->required(), description.c_str());
                break;
            case ebool:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<bool>()->required(), description.c_str());
                break;
            case estr:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<std::string>()->required(), description.c_str());
                break;
            case eunknown:
                std::cout<<"Unknown type of an argument "<<optName<<std::endl;
                break;
            // case evecstr:
            //     cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<std::vector<std::string>>()->required(), description.c_str());
            //     break;
            }
        }
        else{
            switch(hashit(type)) {
            case eint:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<int>(), description.c_str());
                break;
            case euint:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<unsigned int>(), description.c_str());
                break;
            case efloat:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<float>(), description.c_str());
                break;
            case edouble:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<double>(), description.c_str());
                break;
            case ebool:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<bool>(), description.c_str());
                break;
            case estr:
                cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<std::string>(), description.c_str());
                break;
            case eunknown:
                std::cout<<"Unknown type of an argument "<<optName<<std::endl;
                break;
            // case evecstr:
            //     cmdLineOptDesc.add_options()(optName.c_str(), boost::program_options::value<std::vector<std::string>>(), description.c_str());
            //     break;
            }
        }
    
    }
    return cmdLineOptDesc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

boost::program_options::variables_map ConfigManager::parseCmdLineArgs(int argc, char **argv){
    
    boost::program_options::options_description cmdLineOptDesc;
    cmdLineOptDesc.add(parseAllowedArgs());

    boost::program_options::variables_map varMap;        
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    boost::program_options::notify(varMap); 

    if (varMap.count("help")) {
        std::cout<<cmdLineOptDesc<<std::endl;
        exit(1);
    }
    return varMap;
}

boost::property_tree::ptree ConfigManager::getConfig(int argc, char **argv){

    boost::property_tree::ptree tree;
    boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);

    if(argc<2){
        std::cout<<" Usage: masterConfig.json"<<std::endl;
        boost::property_tree::read_json(ConfigManager::masterConfigPath, tree);
        return tree;
    }
    else {
        std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
        boost::property_tree::read_json(argv[1], tree);
    }

    vecOfTuples optList = allowedOptList();
    unsigned int vecSize = optList.size();

    for(unsigned int i = 0; i < vecSize; i++)
    {
        std::string optName = std::get<0>(optList.at(i));
        std::string optType = std::get<1>(optList.at(i));
        if (varMap.count(optName.c_str())){
            switch(hashit(optType)) {
            case eint:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<int>());
                break;
            case euint:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<unsigned int>());
                break;
            case efloat:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<float>());
                break;
            case edouble:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<double>());
                break;
            case ebool:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<bool>());
                break;
            case estr:
                tree.put(optName.c_str(), varMap[optName.c_str()].as<std::string>());
                break;
            case eunknown:
                std::cout<<"Unknown type of an argument "<<optName<<std::endl;
                break;
            // case evecstr:
            //     tree.put(optName.c_str(), varMap[optName.c_str()].as<std::vector<std::string> >());
            //     break;
            } 
        }
    }
    
    return tree;
}