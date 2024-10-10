#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"
#include <cmath> // to use M_PI in JSON and cmd line expressions

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Custom std::validator method to read value of type T from cmd line argument needed by BOOST program options,
// where T denotes: int, usigned int, float or double.
// Simple math expressions, such as "M_PI*2", will be converted to their numerical representation at run time.
// An instance of TApplication must be already initialized beforehand in order to pass the valid pointer to gInterpreter.
// NOTE: if pointer to gInterpreter if zero, then math expressions are passed as-is.
// 
namespace std {
  template<typename T> void validate(boost::any & v, const std::vector<std::string> & val,
				     ConfigManager::myValue<T>*, int) {
    if (v.empty())
      v = boost::any(ConfigManager::myValue<T>());
    ConfigManager::myValue<T> *p = boost::any_cast<ConfigManager::myValue<T> >(&v);
    BOOST_ASSERT(p);
    
    // Extract the first string
    auto str = boost::program_options::validators::get_single_string(val);

    // Evaluate math expression (if any)
    auto filteredValue = boost::program_options::validators::get_single_string(ConfigManager::filterMathExpressions<T>({str.c_str()}, gInterpreter));

    if(std::is_same<T, bool>::value) { // special case
      filteredValue = std::regex_replace(filteredValue, std::regex("(?:true)"), "1");
      filteredValue = std::regex_replace(filteredValue, std::regex("(?:false)"), "0");
    }
    
    *p = boost::lexical_cast<T>(filteredValue);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Custom std::validator method to read vector of type T from cmd line arguments needed by BOOST program options,
// where T denotes: std::string, int, usigned int, float, double or bool.
// Values have to be separated by spaces and, optionally, can be enclosed in square brackets.
// In case of T=bool the following input representations are acceptable:
// 0, 1, true, false, True, False, TRUE, FALSE.
// 
namespace std {
  template<typename T>
  void validate(boost::any & v, const std::vector<std::string> & val,
		ConfigManager::myVector<T>*, int)
  {
    if (v.empty())
      v = boost::any(ConfigManager::myVector<T>());
    ConfigManager::myVector<T> *p = boost::any_cast<ConfigManager::myVector<T>>(&v);
    BOOST_ASSERT(p);

    // remove existing square brackets pair if present
    auto filteredValues = ConfigManager::filterSquareBrackets(val);
    
    // convert symbolic math expressions (if any) to numerical values
    auto filteredValues2 = ConfigManager::filterMathExpressions<T>(filteredValues, gInterpreter);

    // fill array with filtered values
    BOOST_FOREACH(auto& t, filteredValues2) {

      if(std::is_same<T, bool>::value) { // special case
      	t = std::regex_replace(t, std::regex("(?:true)"), "1");
      	t = std::regex_replace(t, std::regex("(?:false)"), "0");
      }
      p->push_back(boost::lexical_cast<T>(t));
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigManager::string_code ConfigManager::getTypeCode (std::string const & inString) const {
    std::string data(inString);
    data = std::regex_replace(data, std::regex("(?:std::vector|vector)"), "vec"); // support short form of "vector"
    data = std::regex_replace(data, std::regex("(?:std::string|string)"), "str"); // support short form of "string"
    data = std::regex_replace(data, std::regex("(?:unsigned int)"), "uint"); // support short form of "unsigned int"
    data = std::regex_replace(data, std::regex("(?:boost::property_tree::ptree)"), "ptree"); // support short form of BOOST property tree
    if (data == "int") return string_code::eint;
    if (data == "uint") return string_code::euint;
    if (data == "float") return string_code::efloat;
    if (data == "double") return string_code::edouble;
    if (data == "str") return string_code::estr;
    if (data == "bool") return string_code::ebool;
    if (data == "ptree") return string_code::eptree;
    if (data == "vec<int>") return string_code::evector_int;
    if (data == "vec<uint>") return string_code::evector_uint;
    if (data == "vec<float>") return string_code::evector_float;
    if (data == "vec<double>") return string_code::evector_double;
    if (data == "vec<str>") return string_code::evector_str;
    if (data == "vec<bool>") return string_code::evector_bool;
    if (data == "vec<ptree>") return string_code::evector_ptree;
    return string_code::eunknown;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigManager::ConfigManager(const std::vector<std::string> allowedNodeList, const std::string alternativeAllowedOptPath) {
    allowedOptPath = (alternativeAllowedOptPath=="" ?
		      std::string(std::getenv("HOME"))+"/.tpcreco/config/allowedOptions.json" :
		      alternativeAllowedOptPath);
    setAllowedArgs(allowedNodeList);
    parseAllowedArgs(allowedOptPath);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::parseAllowedArgs(const std::string & jsonName){

    visibleCmdLineOptDesc.add_options()("help", "produce help message");
        
    std::cout<<KBLU<<"ConfigManager: using config file "<<RST<<jsonName<<std::endl
             <<KBLU<<"for a list of allowed command line arguments "
             <<"and default parameters values"
             <<RST<<std::endl;                           
    
    boost::property_tree::ptree paramsDescTree;
    boost::property_tree::read_json(jsonName, paramsDescTree);

    // //////// DEBUG
    // std::cout << __FUNCTION__ << ": ######### PARAMETER_DESCRIPTION_TREE after default init - START #######" << std::endl;
    // ConfigManager::printTree(paramsDescTree);
    // std::cout << __FUNCTION__ << ": ######### PARAMETER_DESCRIPTION_TREE after default init - START #######" << std::endl;
    // //////// DEBUG

    BOOST_FOREACH(const boost::property_tree::ptree::value_type &v, paramsDescTree) {         
        
        const std::string groupPath{"group"};        
        const std::string typePath {"type"};
        const std::string defaultValuePath{"defaultValue"};
        const std::string descriptionPath{"description"};

	std::string groupName = v.second.get<std::string>(groupPath);
	std::string optName = groupName+"."+v.first;
        std::string typeName = v.second.get<std::string>(typePath);
	std::string description = v.second.get<std::string>(descriptionPath);
	string_code type = getTypeCode(typeName);

	// protection against dots (.) inside very last child node names
	// this prevents ambiguity between parameter and group names
	if(v.first.find_first_of(".")!=std::string::npos) {
	  std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		   <<"): ERROR: child node name '"<< v.first <<"' with dot-separator is not allowed!"<<RST<<std::endl;
	  throw std::logic_error("wrong ptree node name");
	}

	// protection against double-dots (..) inside absolute node names
	// this prevents silent defintions of vector<ptree>
	if(optName.find("..")!=std::string::npos) {
	  std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		   <<"): ERROR: absolute node name '"<< optName <<"' with double-dot separator is not allowed!"<<RST<<std::endl;
	  throw std::logic_error("wrong ptree node name");
	}

	// protection against forbidden characters and restricted node names
	if(!checkNodeNameSyntax(optName)) continue;

	// protection against duplicated absolute node names
        if(varTypeMap.find(optName)!=varTypeMap.end()) {
	  std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		   <<"): ERROR: absolute node name '"<< optName <<"' is duplicated!"<<RST<<std::endl;
	  throw std::logic_error("duplicated ptree node name");
	}

	// protection against conflicts between ptree/vector<ptree> and ordinary types T/vector<T>
	BOOST_FOREACH(auto &it, varTypeMap) {
	  if((type==string_code::eptree || type==string_code::evector_ptree) &&
	     it.first.find(optName)==0 && // begin of string matches
	     (it.first.size()==optName.size() || // found exact string match
	      (it.first.size()>optName.size() && it.first.at(optName.size())=='.'))) { // found partial match up to the nearest delimeter dot (.)
	    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		     <<"): ERROR: ptree node name '"<< optName <<"' conflicts with non-ptree '"<<it.first<<"' one!"<<RST<<std::endl;
	    throw std::logic_error("duplicated ptree node name");
	  }
	  if(!(type==string_code::eptree || type==string_code::evector_ptree) &&
	     optName.find(it.first)==0 && // begin of string matches
	     (optName.size()==it.first.size() || // found exact string match
	      (optName.size()>it.first.size() && optName.at(it.first.size())=='.'))) { // found partial match up to the nearest delimeter dot (.)
	    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		     <<"): ERROR: ptree node name '"<<it.first<<"' conflicts with non-ptree '"<<optName<<"' one!"<<RST<<std::endl;
	    throw std::logic_error("duplicated ptree node name");
	  }
	}
	varTypeMap[optName] = type;

	//////// DEBUG
	// std::cout << __FUNCTION__ << ": added option: " << optName << ", type: " << type << ", default: " << v.second.get(defaultValuePath, "### ERROR ###") << std::endl;
	//////// DEBUG
	
            switch(type) {
            case string_code::eint:                
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<int>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<int>>(defaultValuePath));
                break;
            case string_code::euint:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<unsigned int>>()->multitoken()->zero_tokens(), // force exactly ONE argument
                                            description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<unsigned int>>(defaultValuePath));
                break;
            case string_code::efloat:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<float>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<float>>(defaultValuePath));
                break;
            case string_code::edouble:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<double>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<double>>(defaultValuePath));
                break;
            case string_code::ebool:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<bool>>()->implicit_value(true)->multitoken()->zero_tokens(), // missing value is acting as "enable flag", otherwise expects exactly ONE argument
					     description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<bool>>(defaultValuePath));
                break;
            case string_code::estr:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<std::string>>()->multitoken()->zero_tokens(), // force exactly ONE argument
                                             description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<std::string>>(defaultValuePath));
                break;
            case string_code::eptree:
                visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<std::string>()->multitoken()->zero_tokens(), // force exactly ONE argument
                                             description.c_str());
		configTree.put_child(optName, v.second.get_child(defaultValuePath));
                break;
            case string_code::evector_int:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<int>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) { // filter input
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << boost::lexical_cast<ConfigManager::myValue<int>>(i.second.data()) << " ";
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_uint:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<unsigned int>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) {
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << boost::lexical_cast<ConfigManager::myValue<unsigned int>>(i.second.data()) << " ";
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_float:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<float>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) {
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << boost::lexical_cast<ConfigManager::myValue<float>>(i.second.data()) << " ";
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_double:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<double>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) {
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << boost::lexical_cast<ConfigManager::myValue<double>>(i.second.data()) << " ";
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_str:

	        // special case for hidden option to store list of input config files
	        if(optName==hiddenFilesOpt) {
	          hiddenCmdLineOptDesc.add_options()(optName.c_str(),
					      boost::program_options::value<ConfigManager::myVector<std::string>>()->multitoken()->composing()->implicit_value({}),
					      description.c_str());
	        } else {
	          visibleCmdLineOptDesc.add_options()(optName.c_str(),
					       boost::program_options::value<ConfigManager::myVector<std::string>>()->multitoken()->composing()->implicit_value({}),
					       description.c_str());
		}
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) {
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << "\"" << boost::lexical_cast<ConfigManager::myValue<std::string>>(i.second.data()) << "\" "; // quoted string, may contain blank spaces
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_bool:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<bool>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		{
		  // construct array of quoted strings in JSON format
		  std::stringstream ss;
		  bool isFirst = true;
		  ss << "[ ";
		  BOOST_FOREACH(auto &i, v.second.get_child(defaultValuePath)) {
		    if(isFirst) isFirst=false;
		    else ss << ", ";
		    ss << boost::lexical_cast<ConfigManager::myValue<bool>>(i.second.data()) << " ";
		  }
		  ss << "]";
		  insertVector(optName, ss.str()); // JSON format
		}
                break;
            case string_code::evector_ptree:
	        visibleCmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<std::string>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
		insertVector(optName, v.second.get_child(defaultValuePath));
                break;
            case string_code::eunknown:
	        std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
			 <<"): ERROR: node '"<<optName<<"' has unknown type '"<<typeName<<"' declaration!"<<RST<< std::endl;
		throw std::logic_error("wrong type");
            }
    }
    //////// DEBUG
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after JSON read - START #######" << std::endl;
    // ConfigManager::printTree(configTree);
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after JSON read - END #######" << std::endl;
    //////// DEBUG

    // complete list of options to be parsed from the cmd line
    allCmdLineOptDesc.add(visibleCmdLineOptDesc).add(hiddenCmdLineOptDesc);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::program_options::variables_map & ConfigManager::parseCmdLineArgs(int argc, char **argv){

    //////// DEBUG
    // std::cout << __FUNCTION__ << ": STARTED" << std::flush << std::endl;
    //////// DEBUG

    // additonal protection against duplicated cmd line options
    // (since BOOST cmd line parsing with has enabled multitoken/composing for some parameters)
    std::stringstream ss;
    std::copy(argv, argv + argc, std::ostream_iterator<char *>(ss, "\n"));
    auto storedOptions = listTree(configTree);
    std::string item;
    std::set<std::string> cmdOptionsSet;
    while(ss >> item) {
      if(item.find("--")==0) item.erase(0,2); // remove leading double-dash sequence (--)
      else continue;
      if(!item.size()) continue;
      for(auto &it: storedOptions) {
	if(it.find(item)==0 && // begin of string matches
	   (it.size()==item.size() || // found exact string match
	    (it.size()>item.size() && it.at(item.size())=='.'))) { // found partial match up to the nearest delimeter dot (.)
	  if(cmdOptionsSet.find(item)==cmdOptionsSet.end()) { // check if cmd line option was already processed
	    cmdOptionsSet.insert(item);
	    break;
	  } else { // duplicated cmd line options are not allowed
	    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		     <<"): ERROR: command line option '--"<< item <<"' is duplicated!"<<RST<< std::endl;
	    throw std::logic_error("wrong command line sytax");
	  }
	}
      }
    }

    // allow only "--longOption" cmd line style to enable parsing negative numbers
    // positional options at the beginning will be assigned to --meta.configJson
    try {
      boost::program_options::positional_options_description positional;
      positional.add(hiddenFilesOpt.c_str(), -1); // allow unlimited number of input JSON files at the begin of cmd line arguments
      auto parsedOptions = boost::program_options::command_line_parser(argc, argv)
	.options(allCmdLineOptDesc)
	.style(boost::program_options::command_line_style::default_style
	       ^ boost::program_options::command_line_style::allow_short // disable '-option' style
	       ^ boost::program_options::command_line_style::long_allow_adjacent) // disable '--option=VAL" style
	.positional(positional) // NOTE: use .positional({}) to disable any unregistered arguments
	                        //       and then catch(boost::program_options::too_many_positional_options_error &e)
	.run();

      // check if hidden option --meta.configJson is present along with some unregistered cmd line options
      auto found_unregistered{false};
      auto found_hidden{false};
      BOOST_FOREACH(auto const& opt, parsedOptions.options) {
	if(opt.position_key==-1 && opt.string_key==hiddenFilesOpt) found_hidden=true; // registered options have positon of -1
	if(opt.position_key!=-1) found_unregistered=true; // unregistered options have positions 0...N
      }
      if(found_hidden && found_unregistered) {
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		 <<"): ERROR: mixing unregistered command line arguments with '--"<<hiddenFilesOpt<<"' option is not allowed!"<<RST<< std::endl;
	throw std::logic_error("wrong command line syntax");
      }

      boost::program_options::store(parsedOptions, varMap);
      boost::program_options::notify(varMap);
    } catch (boost::program_options::unknown_option &e) {
      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	       <<"): ERROR: unknown command line option!"<<RST<< std::endl;
      throw std::logic_error(e.what());
    }
    // } catch (boost::program_options::too_many_positional_options_error &e) { // unregistered option(s) detected
    //   std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
    // 	       <<"): ERROR: unknown command line option!"<<RST<< std::endl;
    //   throw std::logic_error("wrong command line syntax");
    // }

    //////// DEBUG
    //    std::cout << __FUNCTION__ << ": ENDED" << std::flush << std::endl;
    //////// DEBUG

    return varMap;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::updateWithJsonFile(const std::string & jsonName){

    std::cout<<KBLU<<"ConfigManager: updating parameters with configuration file: "<<RST<<jsonName<<std::endl;
    boost::property_tree::ptree configTreeUpdate;
    boost::property_tree::read_json(jsonName, configTreeUpdate);

    // parse text math expressions (if any)
    boost::property_tree::ptree configTreeUpdateFiltered;
    auto keys_updates = ConfigManager::listTree(configTreeUpdate);

    ///////// DEBUG
    // {
    //   auto keys_orig = ConfigManager::listTree(configTree);
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - START (" << keys_orig.size() << " keys) #######" << RST << std::endl;
    //   BOOST_FOREACH(auto& key, keys_orig) {
    // 	std::cout << key << std::endl;
    //   }
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - END (" << keys_orig.size() << " keys) #######" << RST << std::endl;
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique unfilltered keys to be UPDATED - START (" << keys_updates.size() << " keys) #######" << RST << std::endl;
    //   BOOST_FOREACH(auto& key, keys_updates) {
    // 	  std::cout << key << std::endl;
    //   }
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique unfiltered keys to be UPDATED - END (" << keys_updates.size() << " keys) #######" << RST << std::endl;
    //   std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE unfiltered values to be UPDATED - START #######" << RST << std::endl;
    //   ConfigManager::printTree(configTreeUpdate);
    //   std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE unfiltered values to be UPDATED - END #######" << RST << std::endl;
    // }
    ///////// DEBUG

    // filter list of update keys to keep only "parent.child" depth level
    // (this is relevant for ptree and vector<ptree> parameter types)
    // check if every key of the update tree is valid:
    // - scalar type T:  has to match exactly "groupName.parName"
    // - type vector<T>: has to match one of: "groupName.parName" (empty vector) or "groupName.parName." (non-empty vector)
    // - ptree:          partial match to registered parent path "groupName.ptreeName"
    // - vector<ptree>:  partial match to registered parent path "groupName.ptreeName" (empty vector) or "groupName.ptreeName.." (non-empty vector)
    std::unordered_set<std::string> keys_updates_filtered;
    BOOST_FOREACH(auto& key, keys_updates) {
      auto found = false;
      BOOST_FOREACH(auto &item, varTypeMap) {
	if(key.find(item.first)==0 && // begin of string matches
	   (key.size()==item.first.size() || // found exact string match
	    (key.size()>item.first.size() && key.at(item.first.size())=='.'))) { // found partial match up to the nearest delimeter dot (.)
	  keys_updates_filtered.insert(item.first);
	  found = true;
	  break;
	}
      }
      if(!found) {
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		 <<"): ERROR: Input JSON file '"<<jsonName<<"' contains node '"<<key<<"' of unknown type!"<<RST<<std::endl;
	throw std::logic_error("wrong JSON file");
      }
    }

    ///////// DEBUG
    // {
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique filtered keys to be UPDATED - START (" << keys_updates_filtered.size() << " keys)  #######" << RST << std::endl;
    //   BOOST_FOREACH(auto& key, keys_updates_filtered) {
    // 	std::cout << key << std::endl;
    //   }
    //   std::cout << KGRN << __FUNCTION__ << ": ####### List of unique filtered keys to be UPDATED - END (" << keys_updates_filtered.size() << " keys) #######" << RST << std::endl;
    // }
    ///////// DEBUG

    BOOST_FOREACH(auto& nodePath, keys_updates_filtered) {
      string_code type = (varTypeMap.find(nodePath)==varTypeMap.end() ? string_code::eunknown : varTypeMap.at(nodePath) );

      switch(type) {
      case string_code::eint:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<int>>(nodePath));
	break;
      case string_code::euint:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<unsigned int>>(nodePath));
	break;
      case string_code::efloat:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<float>>(nodePath));
	break;
      case string_code::edouble:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<double>>(nodePath));
	break;
      case string_code::ebool:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<bool>>(nodePath));
	break;
      case string_code::estr:
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<std::string>(nodePath));
	break;
      case string_code::eptree:
	{
	  // protection against silent assignment of vector<ptree> value to parameter declared as non-vector ptree
	  if(configTreeUpdate.get_child(nodePath).size() &&
	     configTreeUpdate.get_child(nodePath).front().first=="" &&
	     configTreeUpdate.get_child(nodePath).back().first=="") {
	    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		     <<"): ERROR: Input JSON file '"<<jsonName<<"' assigns vector<ptree> value to node '"<<nodePath<<"' declared as non-vector ptree!"<<RST<<std::endl;
	    throw std::logic_error("wrong JSON file");
	  }
	  pruneTree(configTreeUpdateFiltered, nodePath);
	  configTreeUpdateFiltered.put_child(nodePath, configTreeUpdate.get_child(nodePath));
	}
	break;
      case string_code::evector_int:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<int>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_uint:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<unsigned int>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_float:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<float>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_double:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<double>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_str:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<std::string>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_bool:
	{
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
	  BOOST_FOREACH(auto& item, configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(boost::lexical_cast<ConfigManager::myValue<bool>>(item.second.data()))))); // NOTE: this method works in both Release and Debug CMAKE modes
	  }
	}
	break;
      case string_code::evector_ptree:
	{
	  // protection against silent assignment of non-vector ptree value to parameter declared as vector<ptree>
	  if(configTreeUpdate.get_child(nodePath).size() &&
	     !(configTreeUpdate.get_child(nodePath).front().first=="" &&
	       configTreeUpdate.get_child(nodePath).back().first=="")) {
	    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		     <<"): ERROR: Input JSON file '"<<jsonName<<"' assigns non-vector ptree value to node '"<<nodePath<<"' declared as vector<ptree>!"<<RST<<std::endl;
	    throw std::logic_error("wrong JSON file");
	  }
	  pruneTree(configTreeUpdateFiltered, nodePath);
	  auto& array = configTreeUpdateFiltered.add_child(nodePath, boost::property_tree::ptree());
	  BOOST_FOREACH(auto& i,  configTreeUpdate.get_child(nodePath)) {
	    array.push_back(std::make_pair( "", i.second ));
	  }
	}
	break;
      case string_code::eunknown:
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: node '"<<nodePath<<"' has unknown type!"<<RST<<std::endl;
	throw std::logic_error("wrong type");
      }
    }

    ///////// DEBUG
    // {
    //   std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE filtered values to be UPDATED - START #######" << RST << std::endl;
    //    ConfigManager::printTree(configTreeUpdateFiltered);
    //   std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE filtered values to be UPDATED - END #######" << RST << std::endl;
    // }
    ///////// DEBUG

    BOOST_FOREACH(auto& key, keys_updates_filtered) {
      configTree.get_child(key)=configTreeUpdateFiltered.get_child(key);
      // alternatively use: { pruneTree(pt, key); pt.put(key, updates.get_child(key).data()); }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::updateWithCmdLineArgs(const boost::program_options::variables_map & varMap){

    if(varMap.size()) {
      std::cout<<KBLU<<"ConfigManager: updating parameters with command line arguments:"<<RST<<std::endl;
    }
    
    // parse cmd line arguments including text math expressions (if any)
    for(const auto& item: varMap){        
        std::cout<<item.first<<std::endl;
	string_code type = (varTypeMap.find(item.first)==varTypeMap.end() ? string_code::eunknown : varTypeMap.at(item.first) );
        switch(type) {
            case string_code::eint:                
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<int>>());   
                break;
            case string_code::euint:
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<unsigned int>>());   
                break;
            case string_code::efloat:
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<float>>());   
                break;
            case string_code::edouble:
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<double>>());   
                break;
            case string_code::ebool:
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<bool>>());   
                break;
            case string_code::estr:
	        configTree.put(item.first, item.second.as<ConfigManager::myValue<std::string>>());   
                break;
            case string_code::eptree: {
		boost::property_tree::ptree pt;
		std::stringstream ss(item.second.as<std::string>());
		boost::property_tree::read_json(ss, pt);

	        // protection against silent assignment of vector<ptree> value to parameter declared as non-vector ptree
	        if(pt.size() &&
		   pt.front().first=="" &&
		   pt.back().first=="") {
		  std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
			   <<"): ERROR: Command line option '--"<<item.first<<"' assigns vector<ptree> value to node declared as non-vector ptree!"<<RST<<std::endl;
		  throw std::logic_error("wrong command line syntax");
		}
		configTree.put_child(item.first, pt);
	        }
                break;
	    case string_code::evector_int:
	        insertVector(item.first, item.second.as<ConfigManager::myVector<int>>());
                break;
	    case string_code::evector_uint:
	        insertVector(item.first, item.second.as<ConfigManager::myVector<unsigned int>>());
                break;
	    case string_code::evector_float:
	        insertVector(item.first, item.second.as<ConfigManager::myVector<float>>());
                break;
	    case string_code::evector_double:
	        insertVector(item.first, item.second.as<ConfigManager::myVector<double>>());
                break;
	    case string_code::evector_str:
		insertVector(item.first, item.second.as<ConfigManager::myVector<std::string>>());
                break;
	    case string_code::evector_bool:
	        insertVector(item.first, item.second.as<ConfigManager::myVector<bool>>());
                break;
	    case string_code::evector_ptree:
	      {
		boost::property_tree::ptree update;
		auto& array = update.add_child("dummy", boost::property_tree::ptree()); // new array to be populated
		BOOST_FOREACH(auto it, item.second.as<ConfigManager::myVector<std::string>>()) {
		  std::stringstream ss(it);
		  boost::property_tree::ptree pt;
		  boost::property_tree::read_json(ss, pt);
		  array.push_back(std::make_pair("", pt));
		}

	        // protection against silent assignment of non-vector ptree value to parameter declared as vector<ptree>
	        if(update.get_child("dummy").size() &&
		   !(update.get_child("dummy").front().first=="" &&
		     update.get_child("dummy").back().first=="")) {
		  std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
			   <<"): ERROR: Command line option '--"<<item.first<<"' assigns non-vector ptree value to node declared as vector<ptree>!"<<RST<<std::endl;
		  throw std::logic_error("wrong command line syntax");
		}
		insertVector(item.first, update.get_child("dummy")); // this will also prune previously existing node
	      }
	      break;
            case string_code::eunknown:
	      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: node '"<<item.first<<"' has unknown type!"<<RST<<std::endl;
	      throw std::logic_error("wrong type");
	}
    }
    
    //////// DEBUG
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after parsing CMD LINE - START #######" << std::endl;
    // ConfigManager::printTree(configTree);
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after parsing CMD LINE - END #######" << std::endl;
    //////// DEBUG
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::property_tree::ptree & ConfigManager::getConfig(int argc, char **argv){

    boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);

    if(argc<2){ // warn about using default parameters
      std::cout<<KBLU<<"ConfigManager: using default configuration file: "<<RST<<allowedOptPath<<std::endl;
    }
    else if (varMap.count("help")) { // display help info and set HELP mode
      helpMode=true;
      // std::cout<<"\nUsage: "<<argv[0]<<" [--scalarOption <VAL>] [--vectorOption <VAL1> <VAL2> ... ] [--switchFlag]"
      // 	       <<"\n\nAvailable options:\n\n"<<visibleCmdLineOptDesc
      // 	       <<"\nExample: "<<argv[0]<<" --meta.configJson config_A.json config_B.json\n\n";
      std::cout<<"\nUsage: "<<argv[0]<<" File1.json ... FileN.json [--scalarOption <VAL>] [--vectorOption <VAL1> <VAL2> ... ] [--switchFlag]\n\n"
	       <<visibleCmdLineOptDesc
	       <<"\n\nExample: "<<argv[0]<<" config.json --beamParameters.energy 10.\n\n";
      return configTree; // nothing more to do, exit
    }
    else if(varMap.count(hiddenFilesOpt.c_str())) { // apply changes from JSON file(s)
      auto jsonNameVec = varMap[hiddenFilesOpt.c_str()].as<ConfigManager::myVector<std::string>>();
      for(auto &jsonName: jsonNameVec) {
	updateWithJsonFile(jsonName);
      }
    }

    // finally apply changes from command line options
    updateWithCmdLineArgs(varMap);

    std::cout<<std::endl<<KBLU<<"List of final configuration tree to be used:"<<RST<<std::endl;
    ConfigManager::printTree(configTree);

    return configTree;
}
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //
// // Helper static method to replace some parts of the existing BOOST proprerty tree (target tree called 'pt').
// // For self-consistency, the tree with updates (source tree called 'updates')
// // must not introduce any new root nodes not present in the BOOST tree to be updated.
// //
// void ConfigManager::mergeTrees(boost::property_tree::ptree & pt, const boost::property_tree::ptree & updates){

//   auto keys_orig = ConfigManager::listTree(pt);
//   auto keys_updates = ConfigManager::listTree(updates);

//   ///////// DEBUG
//   {
//     std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - START (" << keys_orig.size() << " keys) #######" << RST << std::endl;
//     BOOST_FOREACH(auto& key, keys_orig) {
//       std::cout << key << std::endl;
//     }
//     std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - END (" << keys_orig.size() << " keys) #######" << RST << std::endl;
//     std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys to be UPDATED - START (" << keys_updates.size() << " keys)  #######" << RST << std::endl;
//     BOOST_FOREACH(auto& key, keys_updates) {
//       std::cout << key << std::endl;
//     }
//     std::cout << KGRN << __FUNCTION__ << ": ####### List of unique keys to be UPDATED - END (" << keys_updates.size() << " keys) #######" << RST << std::endl;
//     std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE values to be UPDATED - START #######" << RST << std::endl;
//     ConfigManager::printTree(updates);
//     boost::property_tree::write_json("to_be_updated.json", updates);
//     std::cout << KGRN << __FUNCTION__ << ": ######### CONFIG_TREE values to be UPDATED - END #######" << RST << std::endl;
//   }
//   ///////// DEBUG

//   // verify that all path names in 'update' tree are valid
//   BOOST_FOREACH(auto& key, keys_updates) {
//     auto it = keys_orig.end();

//     ////// DEBUG
//     // if(key.find("input.sss")!=std::string::npos) {
//     //   it=keys_orig.find(key);
//     //   std::cout << "input.sss:  after exact match:       ok=" << (bool)(it!=keys_orig.end()) << std::endl;
//     //   it=keys_orig.find(key.substr(0, key.size()-1));
//     //   std::cout << "input.sss:  after updates dot match: ok=" << (bool)(it!=keys_orig.end()) << std::endl;
//     //   it=keys_orig.find(key+".");
//     //   std::cout << "input.sss:  after orig dot match:    ok=" << (bool)(it!=keys_orig.end()) << std::endl;
//     // }
//     ////// DEBUG
    
//     if((it=keys_orig.find(key))!=keys_orig.end() || // exact match
//        (key.back()=='.' && (it=keys_orig.find(key.substr(0, key.size()-1)))!=keys_orig.end()) || // partial match, update key ends with dot "."
//        (key.back()!='.' && (it=keys_orig.find(key+"."))!=keys_orig.end())) { // partial match, original key ends with dot "."
//       continue;
//     }
//     std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
// 	     <<"): ERROR: node \""<< key <<"\" is missing in target tree!"<<RST<< std::endl;
//     throw std::logic_error("missing node in target ptree");
//   }

  
//   // ///////// DEBUG
//   // {
//   //   auto keys_pruned = ConfigManager::listTree(pt);
//   //   std::cout << __FUNCTION__ << ": ####### List of unique keys AFTER PRUNE - START (" << keys_pruned.size() << " keys) #######" << std::endl;
//   //   BOOST_FOREACH(auto& key, keys_pruned) {
//   //     std::cout << key << std::endl;
//   //   }
//   //   std::cout << __FUNCTION__ << ": ####### List of unique keys AFTER PRUNE - END (" << keys_pruned.size() << " keys) #######" << std::endl;
//   //   // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values AFTER PRUNE - START #######" << std::endl;
//   //   // ConfigManager::printTree(pt);
//   //   // boost::property_tree::write_json("after_prune.json", pt);
//   //   // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values AFTER PRUNE - END #######" << std::endl;
//   // }
//   // ///////// DEBUG

//   BOOST_FOREACH(auto& key, keys_updates) {
//     if(key.back()=='.') { // array case
//       auto nodePath = key.substr(0, key.size()-1); // remove trailing dot "."
//       pruneTree(pt, nodePath); // remove old data
//       auto& array = pt.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
//       BOOST_FOREACH(auto& item, updates.get_child(nodePath)) {
// 	array.push_back(std::make_pair( "", item.second )); // NOTE: this method works in both Release and Debug CMAKE modes
//       }
//     } else { // single value or empty vector case
//       pt.get_child(key)=updates.get_child(key); // alternatively use: { pruneTree(pt, key); pt.put(key, updates.get_child(key).data()); }
//     }
//   }
// }
// void ConfigManager::mergeTrees(boost::property_tree::ptree& pt, const boost::property_tree::ptree& updates){
//   BOOST_FOREACH( auto& update_lvl1, updates ){
//     BOOST_FOREACH( auto& update_lvl2, updates.get_child(update_lvl1.first)){
//       std::string name = update_lvl1.first+"."+update_lvl2.first; 
//       pt.put(name, update_lvl2.second.data());
//     }
//   }   
// }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper method to write all configuration data into single JSON file.
// 
void ConfigManager::dumpConfig(const std::string & jsonName){
  // check JSON name syntax
  if(!jsonName.size() || // cannot be empty
     jsonName.find(".")==0 || jsonName.find("--")==0 || // cannot start with '--' or '.' sequence
     !std::regex_match(jsonName, std::regex("^[A-Za-z0-9.-_/]+$"))) { // allowed are: alphanumeric, dot, dash, underscore, slash
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	     <<"): ERROR: output JSON file name '"<< jsonName <<"' is not allowed!"<<RST<<std::endl;
    throw std::logic_error("wrong JSON file");
  }
  boost::property_tree::write_json(jsonName, configTree);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method to remove a given node path (and all its descendants) from the BOOST property tree.
// To remove all elements of arryay without removing name of the array use node path ending with dot character ("."), e.g.
// path.to.value  <-- removes the node
// path.to.array  <-- removes all elements of array and its node
// path.to.array. <-- removes all elements of array but preserves the node for future use (empty array)
//
void ConfigManager::pruneTree(boost::property_tree::ptree & pt, const std::string & nodePath)
{
  // split up the path into each sub part
  std::vector<std::string> pathParts;
  boost::split(pathParts, nodePath, boost::is_any_of("."));

  // check each part of the path
  int counter=0;
  auto *root = &pt;
  for(const auto &part : pathParts) {
    
    // exit if such path does not exist
    auto found = root->find(part);
    if (found == root->not_found()) {
      break;
    }

    // erase all instances having same paths (e.g. in case of arrays)
    while( (found=root->find(part))!=root->not_found() ) {
      
      // if this was the last one to look for remove it
      if (&part == &pathParts.back()) {
	root->erase(root->to_iterator(found));
	counter++;
      } // next sub child
      else {
	root = &found->second;
      }
    }
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method that lists all unique keys separated by dot character (',') of the BOOST property tree.
// Elements of arrays are listed only once and are denoted with dot character ('.') at the end.
//
std::unordered_set<std::string> ConfigManager::listTree(const boost::property_tree::ptree & pt) {
  std::unordered_set<std::string> outputList;
  ConfigManager::prepareListTree(pt, outputList);
  return outputList;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::prepareListTree(const boost::property_tree::ptree & pt, std::unordered_set<std::string> & nodeList, const std::string & parentNode) {
  if(pt.empty()) {
    nodeList.insert(parentNode);
  }
  for(auto &item: pt) {
    std::string key = parentNode;
    if(!key.empty()) key += ".";
    key += item.first;
    ConfigManager::prepareListTree(item.second, nodeList, key);
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method that (recursively) prints nice-looking structure of the BOOST property tree.
//
void ConfigManager::printTree(const boost::property_tree::ptree & pt, const std::string prefix) {
  for(boost::property_tree::ptree::const_iterator it = pt.begin(); it != pt.end(); ++it) {
    auto data = it->second.get_value<std::string>();
    // substitute special characters
    data = std::regex_replace(data, std::regex("(?:\\r\\n|\\n|\\r)"), "\\n");
    data = std::regex_replace(data, std::regex("(?:\\t)"), "\\t");
    std::cout << RST << prefix << "|___" << KBLU << it->first << ( (it->first).size() ? ": " : "") << KRED << data << RST << std::endl;
    auto next_prefix{prefix};
    if(it==std::prev(pt.end())) {
      next_prefix += "    "; // very last element
    }
    else {
      next_prefix += "|   "; // regular element
    }
    if((it->second).size()) ConfigManager::printTree(it->second, next_prefix); // recursively process all children in BOOST ptree
    else if(it==std::prev(pt.end())) std::cout << RST << prefix << std::endl;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::insertVector(const std::string & nodePath, const boost::property_tree::ptree & ptreeVector) {

  pruneTree(configTree, nodePath);
  auto& array = configTree.add_child(nodePath, boost::property_tree::ptree());
  BOOST_FOREACH(auto& i, ptreeVector) {
    array.push_back(std::make_pair( "", i.second ));
  }
  //
  // NOTE: below standard piece of code crashes in CMAKE_BUILD_TYPE=Debug mode due to underlying BOOST assert conditions,
  //       while the code above works in both Debug and Release modes...
  // -----
  // pruneTree(configTree, nodePath); // discard all previous values
  // configTree.put(nodePath, ""); // create empty node to support empty vectors
  // BOOST_FOREACH(auto& i, ptreeVector.get_child(defaultValuePath)) {
  //	 configTree.add(nodePath+'.', i.second.data());
  // }
  // -----
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
void ConfigManager::insertVector(const std::string & nodePath, const T & vec) {

  // convert input vector into JSON text format representation using custom T::operator<<() provided
  // by T=ConfigManager::myVector<T2> class, where type T2 = int, unsigned int, float, double, bool or std::string
  insertVector(nodePath, boost::lexical_cast<std::string>(vec));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
template <typename T>
void ConfigManager::insertVector(const std::string & nodePath, const T & vec) {

  pruneTree(configTree, nodePath);
  auto &array = configTree.put_child(nodePath, boost::property_tree::ptree());
   BOOST_FOREACH(auto i, vec) {    
    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(i))));
  }
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::insertVector(const std::string & nodePath, const std::string & vectorInJsonFormat) {

  // convert vector JSON text format representation into BOOST property tree
  std::stringstream ss;
  ss << "{ " << std::quoted("dummy") << " : " << vectorInJsonFormat << " }";
  boost::property_tree::ptree pt;
  boost::property_tree::read_json(ss, pt);
  insertVector(nodePath, pt.get_child("dummy"));
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Allow to accept & parse only selected parameters from JSON or CLI, rather than all available.
//
void ConfigManager::setAllowedArgs(const std::vector<std::string> nodeList) {
  allowedOptionsSet.clear();
  for(auto &it: nodeList) {
    allowedOptionsSet.insert(it);
  }
  if(allowedOptionsSet.size()) {
    std::cout<<KBLU<<"Using the following subset of allowed parameters:"<<RST<<std::endl;
    for(auto &item: allowedOptionsSet) std::cout<<KRED<<item<<RST<<std::endl;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ConfigManager::checkNodeNameSyntax(const std::string & nodePath) {

  // check node name syntax
  if(!nodePath.size() || // cannot be empty
     nodePath.find("--")==0 || nodePath.find(".")==0 || // cannot start with '--' or '.' sequence
     !std::regex_match(nodePath, std::regex("^[A-Za-z0-9.-]+$"))) { // allowed are: alphanumeric, dot, dash
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	     <<"): ERROR: absolute node name '"<< nodePath <<"' is not allowed!"<<RST<<std::endl;
    throw std::logic_error("wrong ptree node name");
  }
  
  // check against exclusive (non-empty) list of allowed options
  if(allowedOptionsSet.size() && allowedOptionsSet.find(nodePath)==allowedOptionsSet.end()) return false; // non-existing node name
  return true; // valid node name
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method to evaluate simple math expressions at run time.
// An instance of TApplication must be already initialized beforehand in order to pass the valid pointer to gInterpreter.
// NOTE: if pointer to gInterpreter if zero, then math expressions are passed as-is.
//
template<typename T>
std::vector<std::string> ConfigManager::filterMathExpressions(const std::vector<std::string> & val, TInterpreter *inter) {

  std::vector<std::string> filteredValues(val);

  /////// DEBUG
  // std::cout << KRED << __FUNCTION__ << ": input vector size=" << val.size() << RST << std::endl;
  // BOOST_FOREACH(auto &item, val) {
  //   std::cout << KRED << __FUNCTION__ << ": input vector element: " << std::quoted(item) << RST << std::endl;
  // }
  /////// DEBUG
  
  if(std::is_same<T, bool>::value) { // bool case
    // first translate plain text to integer values (if any)
    BOOST_FOREACH(auto &t, filteredValues) {
      t = std::regex_replace(t, std::regex("(?:True|TRUE)"), "true");
      t = std::regex_replace(t, std::regex("(?:False|FALSE)"), "false");
    }
  }

  static std::string strFormat, strType;
  if(std::is_same<T, int>::value) {
    strFormat="%d";
    strType="int";
  } else if(std::is_same<T, unsigned int>::value) {
    strFormat="%u";
    strType="unsigned int";
  } else if(std::is_same<T, float>::value) {
    strFormat="%.6g";
    strType="float";
  } else if(std::is_same<T, double>::value) {
    strFormat="%.16lg";
    strType="double";
  } else if(std::is_same<T, bool>::value) { // special case
    strType="bool";
  } else if(std::is_same<T, std::string>::value) { // special case
    strType="std::string";      
  } else {
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: unsupported type in math expression!"<<RST<<std::endl;
    throw std::logic_error("wrong type");
  } 

  if(!inter) return filteredValues; // ROOT's gInterpreter not present, leave input as-is

  inter->Declare("#ifndef _HAVE_CONFIG_MANAGER_MATH_PARSER_\n" // declare TInterpreter variables only once
		 "#define _HAVE_CONFIG_MANAGER_MATH_PARSER_\n"
		 "std::string aString_(\"0\");\n"
		 "std::string ToString_() { return aString_; }\n"
		 "#endif");
  auto funcPtr = (std::string (*)())(inter->Calc("&ToString_"));     
  std::stringstream ss;
  for(auto &item: filteredValues) { // convert math expression to a numeric value using TInterprter
    ss.str("");
    if(std::is_same<T, bool>::value) { // special case
      ss << "((" << strType << ")(" << item << ")?\"true\":\"false\");";
    } else if(std::is_same<T, std::string>::value) { // special case, may contain blank spaces
      item = std::regex_replace(item, std::regex("(?:\")"), "\\\""); // preserve quotes (if any)
  /////// DEBUG
  // BOOST_FOREACH(auto &item, val) {
  //   std::cout << KRED << __FUNCTION__ << "<std::string>: filtered input vector element: " << std::quoted(item) << RST << std::endl;
  // }
  /////// DEBUG
      ss << "(" << strType << ")(\"" << item << "\");";
    } else { // all other formats
      ss << "TString::Format(\"" << strFormat << "\",(" << strType << ")(" << item << ")).Data();";
    }
    auto result = inter->ProcessLine(("aString_="+ss.str()).c_str());
    if(!result) {
      std::cout<<KRED<<__FUNCTION__<<"<"<< strType <<">("<<__LINE__<<"): evaluaton of math expression "<< std::quoted(item) <<" failed!"<<RST<<std::endl;
      throw std::logic_error("wrong math expression");
    }
    item = funcPtr();
    
    /////// DEBUG
    // std::cout << KRED << __FUNCTION__ << "<" << strType << ">: expression: " << std::quoted(item) << " parsed as: " << std::quoted(item) << RST << std::endl;
    /////// DEBUG
  }
  return filteredValues;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method to check square bracket syntax in cmd line inputs when initializing vector options.
// Accepted syntaxes are:
// [] or missing value <-- initializes empty array and clears existing one from JSON file
// [ "value1" "value2" "value3" ] <-- elements enclosed in square brackets, separated by spaces, quotes are optional
// "value1" "value2" "value3" <-- elements separated by spaces, quotes are optional
//
std::vector<std::string> ConfigManager::filterSquareBrackets(const std::vector<std::string> & val) {
  
  std::vector<std::string> filteredValues(val);
  if(!filteredValues.size()) return filteredValues;
  auto hasFrontBracket = (filteredValues.front().front()=='[');
  auto hasRearBracket = (filteredValues.back().back()==']');
  if(hasFrontBracket && hasRearBracket) {
    filteredValues.back().erase(std::prev(filteredValues.back().end())); // NOTE: order is important as front() & back() can point to the same string
    filteredValues.front().erase(filteredValues.front().begin());
    if(filteredValues.size() && filteredValues.back()=="") filteredValues.erase(std::prev(filteredValues.end())); // delete string if empty
    if(filteredValues.size() && filteredValues.front()=="") filteredValues.erase(filteredValues.begin()); // delete string if empty
  } else if(!(!hasFrontBracket && !hasRearBracket)) {
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	     <<"): ERROR: missing one of square brackets in command line arguments!"<<RST<<std::endl;
    throw std::logic_error("missing brackets");
  }
  return filteredValues;
}
