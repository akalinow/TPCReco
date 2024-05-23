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
    ///////// DEBUG
    // std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue): ################################ input vector size=" << val.size() << RST << std::endl;
    ///////// DEBUG
    
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
      ///////// DEBUG
      //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue<bool>): ################################ processed VAL: " << std::quoted(filteredValue) << RST << std::endl;
      ///////// DEBUG
    }
    
    *p = boost::lexical_cast<T>(filteredValue);
  }
}
/*
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Overloaded std::operator<<() to convert std::vector<T> into a text stream,
// where T denotes: std::string, int, usigned int, float, double or bool.
// The output is enclosed in square brackets and quoted values are separated by spaces and commas
// to resemble array in JSON format, e.g. [ "value1", "value2", "value3" ].
// In case of T=bool the resulting text representation will be "true" or "false".
//
namespace std {
  template<typename T> static std::ostream& operator<<(std::ostream & os,
						       const std::vector<T> & vec) {
    os << "[ ";
    bool isFirst = true;
    for(auto item : vec) { // NOTE: do not use here the reference operator (&) to enable support for std::vector<bool> 
      std::stringstream ss;
      ss << item;
      if(std::is_same<T,bool>::value) {
      	auto b = (ss.str()=="1");
      	ss.str("");
      	ss << (b ? "true" : "false"); 
      }
      if(isFirst) {
	isFirst = false;
      } else {
	os << ", ";
      }
      os << std::quoted(ss.str());
    }
    os << " ]";
    return os;
  }
}
*/
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

      ///////// DEBUG
      //      std::cout << __FUNCTION__ << ": loop -------- inserting as: " << std::quoted(t) << std::endl;
      ///////// DEBUG

      p->push_back(boost::lexical_cast<T>(t));
    }
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Overloaded std::operator<<() to convert myVector<T> into a text stream,
// where T denotes: std::string, int, usigned int, float, double or bool.
// The output is enclosed in square brackets and quoted values are separated by spaces and commas
// to resemble array in JSON format, e.g. [ "value1", "value2", "value3" ].
// In case of T=bool the resulting text representation will be "true" or "false".
//
/*
namespace std {
  template<typename T> static std::ostream& operator<<(std::ostream & os,
						       const ConfigManager::myVector<T> & vec) {
    os << "[ ";
    bool isFirst = true;
    for(auto item : vec.vec) { // NOTE: do not use here the reference operator (&) to enable support for std::vector<bool> 
      std::stringstream ss;
      ss << item;
      if(std::is_same<T,bool>::value) {
      	auto b = (ss.str()=="1");
      	ss.str("");
      	ss << (b ? "true" : "false"); 
      }
      if(isFirst) {
	isFirst = false;
      } else {
	os << ", ";
      }
      os << std::quoted(ss.str());
    }
    os << " ]";
    return os;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Custom std::validator method to read std::vector<T> from cmd line arguments needed by BOOST program options,
// where T denotes: std::string, int, usigned int, float, double or bool.
// Values have to be separated by spaces and, optionally, can be enclosed in square brackets.
// In case of T=bool the following input representations are acceptable:
// 0, 1, true, false, True, False, TRUE, FALSE.
// 
namespace std {
  template<typename T>
  void validate(boost::any & v, const std::vector<std::string> & val,
		std::vector<T>*, int)
  {
    if (v.empty())
      v = boost::any(std::vector<T>());
    std::vector<T> *p = boost::any_cast<std::vector<T>>(&v);
    BOOST_ASSERT(p);

    // remove existing square brackets pair if present
    auto filteredValues = ConfigManager::filterSquareBrackets(val);
    
    // convert symbolic math expressions (if any) to numerical values
    auto filteredValues2 = ConfigManager::filterMathExpressions<T>(filteredValues, gInterpreter);

    // fill array with filtered values
    BOOST_FOREACH(auto& t, filteredValues2) {

      if(std::is_same<T, bool>::value) { // bool case

	// translate plain text to integer values (if any)
	t = std::regex_replace(t, std::regex("(?:true)"), "1");
	t = std::regex_replace(t, std::regex("(?:false)"), "0");
	p->push_back(boost::lexical_cast<T>(t));

	///////// DEBUG
	std::cout << __FUNCTION__ << ": loop -------- inserting as-is: " << boost::lexical_cast<bool>(t) << std::endl;
	///////// DEBUG
	
      } else if(std::is_same<T, std::string>::value ||
		std::is_same<T, float>::value ||
		std::is_same<T, double>::value ||
		std::is_same<T, unsigned int>::value ||
		std::is_same<T, int>::value) {

	p->push_back(boost::lexical_cast<T>(t));
	///////// DEBUG
	std::cout << __FUNCTION__ << ": loop -------- inserting as-is: " << boost::lexical_cast<T>(t) << std::endl;
	///////// DEBUG
	
      } else {
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): unsupported vector type!"<<RST<<std::endl;
	throw;
      }
    }
  }
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
ConfigManager::string_code ConfigManager::getTypeCode (std::string const & inString) const {
    std::string data(inString);
    data = std::regex_replace(data, std::regex("(?:std::vector|vector)"), "vec"); // support short form of "vector"
    data = std::regex_replace(data, std::regex("(?:std::string|string)"), "str"); // support short form of "string"
    data = std::regex_replace(data, std::regex("(?:unsigned int)"), "uint"); // support short form of "unsigned int"
    if (data == "int") return string_code::eint;
    if (data == "uint") return string_code::euint;
    if (data == "float") return string_code::efloat;
    if (data == "double") return string_code::edouble;
    if (data == "str") return string_code::estr;
    if (data == "bool") return string_code::ebool;
    if (data == "vec<int>") return string_code::evector_int;
    if (data == "vec<uint>") return string_code::evector_uint;
    if (data == "vec<float>") return string_code::evector_float;
    if (data == "vec<double>") return string_code::evector_double;
    if (data == "vec<str>") return string_code::evector_str;
    if (data == "vec<bool>") return string_code::evector_bool;
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

    cmdLineOptDesc.add_options()("help", "produce help message");
        
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
	if(!checkNodeNameSyntax(optName)) continue;
	
        std::string typeName = v.second.get<std::string>(typePath);
	std::string description = v.second.get<std::string>(descriptionPath);
	string_code type = getTypeCode(typeName);
        varTypeMap[optName] = type;

	//////// DEBUG
	// std::cout << __FUNCTION__ << ": added option: " << optName << ", type: " << type << ", default: " << v.second.get(defaultValuePath, "### ERROR ###") << std::endl;
	//////// DEBUG
	
            switch(type) {
            case string_code::eint:                
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<int>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<int>>(defaultValuePath));
                break;
            case string_code::euint:
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<unsigned int>>()->multitoken()->zero_tokens(), // force exactly ONE argument
                                            description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<unsigned int>>(defaultValuePath));
                break;
            case string_code::efloat:
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<float>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<float>>(defaultValuePath));
                break;
            case string_code::edouble:
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<double>>()->multitoken()->zero_tokens(), // force exactly ONE argument
					     description.c_str());
                configTree.put(optName, v.second.get<ConfigManager::myValue<double>>(defaultValuePath));
                break;
            case string_code::ebool:
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<bool>>()->implicit_value(true)->multitoken()->zero_tokens(), // missing value is acting as "enable flag", otherwise expects exactly ONE argument
					     description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<bool>>(defaultValuePath));
                break;
            case string_code::estr:
                cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myValue<std::string>>()->multitoken()->zero_tokens(), // force exactly ONE argument
                                             description.c_str());
		configTree.put(optName, v.second.get<ConfigManager::myValue<std::string>>(defaultValuePath));
                break;
            case string_code::evector_int:
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<int>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
	        // insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
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
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<unsigned int>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
		// insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
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
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<float>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
	        // insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
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
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<double>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
		// insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
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
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<std::string>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
	        // insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
					     boost::program_options::value<ConfigManager::myVector<std::string>>()->multitoken()->composing()->implicit_value({}),
					     description.c_str());
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
	        // cmdLineOptDesc.add_options()(optName.c_str(),
		// 			     boost::program_options::value<std::vector<bool>>()->multitoken()->composing()->implicit_value({}),
		// 			     description.c_str());
	        // insertVector(optName, v.second.get_child(defaultValuePath));
	        cmdLineOptDesc.add_options()(optName.c_str(),
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
            case string_code::eunknown:
	        std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
			 <<"): ERROR: unsupported type \""<< typeName <<"\" of node \""<< optName <<"\"!"<<RST<< std::endl;
		throw std::logic_error("wrong type");
            }
        }
    //////// DEBUG
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after JSON read - START #######" << std::endl;
    // ConfigManager::printTree(configTree);
    // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values after JSON read - END #######" << std::endl;
    //////// DEBUG
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const boost::program_options::variables_map & ConfigManager::parseCmdLineArgs(int argc, char **argv){

    //////// DEBUG
    // std::cout << __FUNCTION__ << ": STARTED" << std::flush << std::endl;
    //////// DEBUG

    // apply additional protection against duplicated cmd line options (due to BOOST cmd line parsing with enabled multitoken/composing)
    // std::stringstream ss;
    // std::copy(argv, argv + argc, std::ostream_iterator<char *>(ss, "\n"));
    // auto storedOptions = listTree(configTree);
    // std::string item;
    // std::set<std::string> cmdOptionsSet;
    // while(ss >> item) {
    //   if(item.find("--")==0) item.erase(0,2); // remove leading dash characters "-"
    //   if(item.size()) {
    // 	for(auto &it: storedOptions) {
    // 	  if(item==it || item+'.'==it) {
    // 	    if(cmdOptionsSet.find(item)==cmdOptionsSet.end()) {
    // 	      cmdOptionsSet.insert(item);
    // 	    } else { // duplicated cmd line options are not allowed
    // 	      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
    // 		       <<"): ERROR: command line option \"--"<< item <<"\" appears more than once!"<<RST<< std::endl;
    // 	      throw;
    // 	    }
    // 	  }
    // 	}
    //   }
    // }

    // allow only "--longOption" cmd line style to enable parsing negative numbers
    boost::program_options::store
      (boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc,
						  boost::program_options::command_line_style::default_style
						  ^ boost::program_options::command_line_style::allow_short
						  ^ boost::program_options::command_line_style::long_allow_adjacent), varMap);
    boost::program_options::notify(varMap);

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
    BOOST_FOREACH(auto& key, keys_updates) {
      std::string nodePath(key);
      if(nodePath.back()=='.') nodePath.erase(nodePath.size()-1);
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
	configTreeUpdateFiltered.put(nodePath, configTreeUpdate.get<ConfigManager::myValue<std::string>>(nodePath));
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
      case string_code::eunknown:
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: unknown type of parameter: "<<RST<<nodePath<<std::endl;
	throw std::logic_error("wrong type");
      }
    }
    mergeTrees(configTree, configTreeUpdateFiltered);
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
            case string_code::eunknown:
	      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: unknown type of parameter: "<<RST<<item.first<<std::endl;
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
      std::cout<<"\nUsage: "<<argv[0]<<" [--scalarOption <VAL>] [--vectorOption <VAL1> <VAL2> ... ] [--switchFlag]"
	       <<"\n\nAvailable options:\n\n"<<cmdLineOptDesc
	       <<"\nExample: "<<argv[0]<<" --meta.configJson config_A.json config_B.json\n\n";
      return configTree; // nothing more to do, exit
    }
    else if(varMap.count("meta.configJson")) { // apply changes from JSON file(s)
      auto jsonNameVec = varMap["meta.configJson"].as<ConfigManager::myVector<std::string>>();
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper static method to replace some parts of the existing BOOST proprerty tree (target tree called 'pt').
// For self-consistency, the tree with updates (source tree called 'updates')
// must not introduce any new root nodes not present in the BOOST tree to be updated.
// 
void ConfigManager::mergeTrees(boost::property_tree::ptree & pt, const boost::property_tree::ptree & updates){

  auto keys_orig = ConfigManager::listTree(pt);
  auto keys_updates = ConfigManager::listTree(updates);

  ///////// DEBUG
  // {
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - START (" << keys_orig.size() << " keys) #######" << std::endl;
  //   BOOST_FOREACH(auto& key, keys_orig) {
  //     std::cout << key << std::endl;
  //   }
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys ORIGINAL - END (" << keys_orig.size() << " keys) #######" << std::endl;
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys to be UPDATED - START (" << keys_updates.size() << " keys)  #######" << std::endl;
  //   BOOST_FOREACH(auto& key, keys_updates) {
  //     std::cout << key << std::endl;
  //   }
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys to be UPDATED - END (" << keys_updates.size() << " keys) #######" << std::endl;
  //   std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values to be UPDATED - START #######" << std::endl;
  //   ConfigManager::printTree(updates);
  //   boost::property_tree::write_json("to_be_updated.json", updates);
  //   std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values to be UPDATED - END #######" << std::endl;
  // }
  ///////// DEBUG

  // verify that all path names in 'update' tree are valid
  BOOST_FOREACH(auto& key, keys_updates) {
    auto it = keys_orig.end();

    ////// DEBUG
    // if(key.find("input.sss")!=std::string::npos) {
    //   it=keys_orig.find(key);
    //   std::cout << "input.sss:  after exact match:       ok=" << (bool)(it!=keys_orig.end()) << std::endl;
    //   it=keys_orig.find(key.substr(0, key.size()-1));
    //   std::cout << "input.sss:  after updates dot match: ok=" << (bool)(it!=keys_orig.end()) << std::endl;
    //   it=keys_orig.find(key+".");
    //   std::cout << "input.sss:  after orig dot match:    ok=" << (bool)(it!=keys_orig.end()) << std::endl;
    // }
    ////// DEBUG
    
    if((it=keys_orig.find(key))!=keys_orig.end() || // exact match
       (key.back()=='.' && (it=keys_orig.find(key.substr(0, key.size()-1)))!=keys_orig.end()) || // partial match, update key ends with dot "."
       (key.back()!='.' && (it=keys_orig.find(key+"."))!=keys_orig.end())) { // partial match, original key ends with dot "."
      continue;
    }
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	     <<"): ERROR: node \""<< key <<"\" is missing in target tree!"<<RST<< std::endl;
    throw std::logic_error("missing node in target ptree");
  }

  
  // ///////// DEBUG
  // {
  //   auto keys_pruned = ConfigManager::listTree(pt);
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys AFTER PRUNE - START (" << keys_pruned.size() << " keys) #######" << std::endl;
  //   BOOST_FOREACH(auto& key, keys_pruned) {
  //     std::cout << key << std::endl;
  //   }
  //   std::cout << __FUNCTION__ << ": ####### List of unique keys AFTER PRUNE - END (" << keys_pruned.size() << " keys) #######" << std::endl;
  //   // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values AFTER PRUNE - START #######" << std::endl;
  //   // ConfigManager::printTree(pt);
  //   // boost::property_tree::write_json("after_prune.json", pt);
  //   // std::cout << __FUNCTION__ << ": ######### CONFIG_TREE values AFTER PRUNE - END #######" << std::endl;
  // }
  // ///////// DEBUG

  BOOST_FOREACH(auto& key, keys_updates) {
    if(key.back()=='.') { // array case
      auto nodePath = key.substr(0, key.size()-1); // remove trailing dot "."
      pruneTree(pt, nodePath); // remove old data
      auto& array = pt.add_child(nodePath, boost::property_tree::ptree()); // new array to be populated
      BOOST_FOREACH(auto& item, updates.get_child(nodePath)) {
	array.push_back(std::make_pair( "", item.second )); // NOTE: this method works in both Release and Debug CMAKE modes
      }
    } else { // single value or empty vector case
      pt.get_child(key)=updates.get_child(key); // alternatively use: { pruneTree(pt, key); pt.put(key, updates.get_child(key).data()); }
    }
  }
}
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
	     <<"): ERROR: output JSON file name \""<< jsonName <<"\" is not allowed!"<<RST<<std::endl;
    throw std::logic_error("wrong JSON name");
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
  ////// DEBUG
  // // check if nodePath refers to an element of array
  // bool isVector = (nodePath.back()=='.');
  // std::cout << __FUNCTION__ << ": removing nodePath=" << nodePath << ", isVector=" << isVector << std::endl;
  ////// DEBUG

  // split up the path into each sub part
  std::vector<std::string> pathParts;
  boost::split(pathParts, nodePath, boost::is_any_of("."));

  // check each part of the path
  int counter=0;
  auto *root = &pt;
  for(const auto &part : pathParts) {
    
    // exit if such path does not exist
    auto found = root->find(part);

    ////// DEBUG
    // std::cout << __FUNCTION__ << ": removing nodePath=" << nodePath << ", checking part=" << part << ", found=" << (root->find(part)!=root->not_found()) << std::endl;
    ////// DEBUG
    
    if (found == root->not_found()) {

      ////// DEBUG
      // std::cout << __FUNCTION__ << ": removing nodePath=" << nodePath << " --> not found" << std::endl;
      ////// DEBUG

      //      return;
      break;
    }
    // erase all instances having same paths (e.g. in case of arrays)
    while( (found=root->find(part))!=root->not_found() ) {
      
      // if this was the last one to look for remove it
      if (&part == &pathParts.back()) {

	////// DEBUG
	// std::cout << __FUNCTION__ << ": removing nodePath=" << nodePath << ", part=" << part << std::endl;
	////// DEBUG

	root->erase(root->to_iterator(found));
	counter++;
      } // next sub child
      else {
	root = &found->second;
      }
    }
  }
  // found at least one instance
  ////// DEBUG
  // std::cout << __FUNCTION__ << ": removing nodePath=" << nodePath << " --> removed " << counter << " instance(s)" << std::endl;
  ////// DEBUG
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
    //////// DEBUG
    // std::cout<<__FUNCTION__<<"(A): adding element of vector: " << nodePath << " equal to: " << array.back().second.data() << std::endl;
    //////// DEBUG
  }

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

  //////// DEBUG
  // std::cout<<__FUNCTION__<<"(E): updating node: " << nodePath << ", vector lexical cast: " << boost::lexical_cast<std::string>(vec) << std::endl;
  //////// DEBUG

  insertVector(nodePath, boost::lexical_cast<std::string>(vec)); // vector converted to JSON format using custom T::operator<<()
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
template <typename T>
void ConfigManager::insertVector(const std::string & nodePath, const T & vec) {

  //////// DEBUG
  std::cout<<__FUNCTION__<<"(C): updating node: " << nodePath << ", vector lexical cast: " << boost::lexical_cast<std::string>(vec) << std::endl;
  //////// DEBUG
  
  pruneTree(configTree, nodePath);
  auto &array = configTree.put_child(nodePath, boost::property_tree::ptree());
   BOOST_FOREACH(auto i, vec) {    
    array.push_back(std::make_pair( "", boost::property_tree::ptree(boost::lexical_cast<std::string>(i))));
    //////// DEBUG
    std::cout<<__FUNCTION__<<"(C): adding element of vector: " << nodePath << " equal to: " << array.back().second.data() << std::endl;
    //////// DEBUG
  }
}
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConfigManager::insertVector(const std::string & nodePath, const std::string & vectorInJsonFormat) {

  //////// DEBUG
  // std::cout<<__FUNCTION__<<"(D): updating node: " << nodePath << ", vector lexical cast: " << vectorInJsonFormat << std::endl;
  //////// DEBUG

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
	     <<"): ERROR: node name \""<< nodePath <<"\" is not allowed!"<<RST<<std::endl;
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
    std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: unsupported value type!"<<RST<<std::endl;
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
      std::cout<<KRED<<__FUNCTION__<<"<"<< strType <<">("<<__LINE__<<"): expression "<< std::quoted(item) <<" cannot be evaluated by TInterpreter!"<<RST<<std::endl;
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
