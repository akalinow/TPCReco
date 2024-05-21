#include <map>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <regex> // for std::regex_replace, std::regex_match
#include <unordered_set> // for std::unordered_set
#include <iomanip> // for std::quoted
#include <type_traits> // for std::is_same

#include <boost/algorithm/string.hpp> // ADDED
#include <boost/foreach.hpp>
#include <boost/optional.hpp> // ADDED
#include <boost/tokenizer.hpp> // ADDED
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include <TROOT.h>
#include <TInterpreter.h>
#include <TString.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/colorText.h"

class ConfigManager
{
public:

  // constructor with optional exclusive list of allowed nodes and/or optional alternative file with definition of nodes
  ConfigManager(const std::vector<std::string> nodeList={}, const std::string alternativeAllowedOptPath="");
    
    const boost::property_tree::ptree & getConfig(int argc, char** argv);
    
    void dumpConfig(const std::string & jsonName);

    // informs application that HELP info was already displayed after detecting '--help' cmd line option
    bool isHelpMode() { return helpMode; }

    // general purpose static method to print nice-looking content of BOOST ptree
    static void printTree(const boost::property_tree::ptree & pt, const std::string prefix="");
    
    // general purpose static method to get list of unique nodes of BOOST ptree
    static std::unordered_set<std::string> listTree(const boost::property_tree::ptree & pt);
    static void prepareListTree(const boost::property_tree::ptree & pt, std::unordered_set<std::string> & nodeList, const std::string & parentNode="");    

    // general purpose static method to selectively update content of BOOST ptree
    static void mergeTrees(boost::property_tree::ptree & pt, const boost::property_tree::ptree & updates);

    // general purpose static method to remove all sub nodes starting from a given path of BOOST ptree
    static void pruneTree(boost::property_tree::ptree & pt, const std::string & nodePath);

 private:

    void parseAllowedArgs(const std::string & jsonName);
    const boost::program_options::variables_map & parseCmdLineArgs(int argc, char ** argv);
    void setAllowedArgs(const std::vector<std::string> nodeList);
    bool checkNodeNameSyntax(const std::string & nodePath); 
    void updateWithJsonFile(const std::string & jsonName);
    void updateWithCmdLineArgs(const boost::program_options::variables_map & varMap);
    void insertVector(const std::string & nodePath, const std::string & vectorInJsonFormat);
    void insertVector(const std::string & nodePath, const boost::property_tree::ptree & ptreeVector);
    template <typename T> void insertVector(const std::string & nodePath, const T & vec);

 private:
    
    //helpers
    enum class string_code {
      eint,
      euint,
      efloat,
      edouble,
      estr,
      ebool,
      evector_int,
      evector_uint,
      evector_float,
      evector_double,
      evector_str,
      evector_bool,
      eunknown
    };
    string_code getTypeCode (const std::string & inString) const;
    boost::program_options::options_description cmdLineOptDesc;
    boost::program_options::variables_map varMap;
    std::map<std::string, string_code> varTypeMap;
    boost::property_tree::ptree configTree;
    std::string allowedOptPath{""};
    std::set<std::string> allowedOptionsSet;
    bool helpMode{false};
    
 public:

    // helper storage classes needed by BOOST program options
    template<typename T> class myValue;
    template<typename T> class myVector;

    // helper static methods needed by BOOST program options
    static std::vector<std::string> filterSquareBrackets(const std::vector<std::string> & val);
    template<typename T=double> static std::vector<std::string> filterMathExpressions(const std::vector<std::string> & val, TInterpreter *inter=NULL);

    // convenience static getter method to get single value from BOOST ptree node
    template <typename T> static T getScalar(const boost::property_tree::ptree &pt, const std::string & nodePath) {
      BOOST_STATIC_ASSERT(std::is_arithmetic<T>::value);
      return pt.get<T>(nodePath);
    }

    // convenience static getter method to get vector of values from BOOST ptree node
    template <typename T> static std::vector<typename T::value_type> getVector(const boost::property_tree::ptree &pt, const std::string & nodePath) {
      T result;
      for (auto item : pt.get_child(nodePath)) {
	result.push_back(item.second.get_value<typename T::value_type>());
      }
      return result;
    }

    // getter method for accessing single stored value in a given configuration path
    // NOTE: this specialized function has to stay inside header file
    template <typename T> T getScalar(const std::string & nodePath) {
      if(!checkNodeNameSyntax(nodePath)) {
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		 <<"): ERROR: node \""<< nodePath <<"\" does not exist! Check your configuration file: "<< allowedOptPath <<" and/or ConfigManager initialization."<<RST<<std::endl;
	throw std::logic_error("wrong ptree node name");
      }
      if(std::is_same<T, int>::value ||
	 std::is_same<T, unsigned int>::value ||
	 std::is_same<T, float>::value ||
	 std::is_same<T, double>::value ||
	 std::is_same<T, bool>::value ||
	 std::is_same<T, std::string>::value) {
	return ConfigManager::getScalar<T>(configTree, nodePath);
      }
      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__<<"): ERROR: node \""<< nodePath <<"\" has unsupported value type!"<<RST<< std::endl;
      throw std::logic_error("wrong type");
    }
    
    // getter method for accessing entire vector stored in a given configuration path
    // NOTE: this specialized function has to stay inside header file
    template <typename T> std::vector<typename T::value_type> getVector(const std::string & nodePath) {
      if(!checkNodeNameSyntax(nodePath)) {
	std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
		 << "): ERROR: node \""<< nodePath <<"\" does not exist! Check your configuration file: "<< allowedOptPath <<" and/or ConfigManager initialization."<<RST<<std::endl;
	throw std::logic_error("wrong ptree node name");
      }
      if(std::is_same<T, std::vector<int>>::value || std::is_same<T, ConfigManager::myVector<int>>::value ||
	 std::is_same<T, std::vector<unsigned int>>::value || std::is_same<T, ConfigManager::myVector<unsigned int>>::value ||
	 std::is_same<T, std::vector<float>>::value || std::is_same<T, ConfigManager::myVector<float>>::value ||
	 std::is_same<T, std::vector<double>>::value || std::is_same<T, ConfigManager::myVector<double>>::value ||
	 std::is_same<T, std::vector<bool>>::value || std::is_same<T, ConfigManager::myVector<bool>>::value ||
	 std::is_same<T, std::vector<std::string>>::value || std::is_same<T, ConfigManager::myVector<std::string>>::value) {
	return ConfigManager::getVector<T>(configTree, nodePath);
      }
      std::cout<<KRED<<__FUNCTION__<<"("<<__LINE__
	       <<"): ERROR: node \""<< nodePath <<"\" has unsupported vector type!"<<RST<< std::endl;
      throw std::logic_error("wrong type");
    }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper storage class needed for custom validator of single cmd line argument of type T parsed by BOOST program options,
// where T denotes: int, unsigned int, float, double, bool or std::string.
// It provides two overloaded operators:
// - operator<<() - creates nice-looking text stream in JSON format that provides 15-16 significant digits for T=double
//   and "true/false" text labels for T=bool
// - operator>>() - allows to interpret simple math expressions at run-time for type T being: int, unisgned int, float,
//   double and bool.
// 
template<typename T>
class ConfigManager::myValue {
private:
  T value;
public:
  myValue(); // NOTE: specialized empty constructors are defined outside class definition
  myValue(T v) : value(v) { }
  
  friend std::ostream& operator<<(std::ostream & os, const ConfigManager::myValue<T> & v) {
    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue): ################################ input VAL: " << v.value << RST << std::endl;
    ///////// DEBUG
    if(std::is_same<T, bool>::value) { // special case
      if(v.value==true) os << "true";
      else              os << "false";
    } else if(std::is_same<T, std::string>::value) { // special case, may contain blank spaces
      os << boost::lexical_cast<T>(v.value);
    } else {
      static std::string strFormat;
      if(std::is_same<T, int>::value) {
	strFormat="%d";
      } else if(std::is_same<T, unsigned int>::value) {
	strFormat="%u";
      } else if(std::is_same<T, float>::value) {
	strFormat="%.6g";
      } else if(std::is_same<T, double>::value) {
	strFormat="%.16lg";
      } else {
	std::cout<<KRED<<__FUNCTION__<<"(ConfigManager::myValue)("<<__LINE__<<"): ERROR: unsupported value type!"<<RST<<std::endl;
	throw std::logic_error("wrong type");
      }
      os << Form(strFormat.c_str(), v.value);
    }
    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue): ################################ CLEAN EXIT" << RST << std::endl;
    ///////// DEBUG
    return os;
  }
  friend std::istream& operator>>(std::istream & is, ConfigManager::myValue<T> & v) {
    std::stringstream str;
    str << is.rdbuf(); // NOTE: also valid for std::string with blank spaces

    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue): ################################ input TEXT stream: " << str.str() << RST << std::endl;
    ///////// DEBUG

    // evaluate math expression (if any)
    auto filteredValue = boost::program_options::validators::get_single_string(ConfigManager::filterMathExpressions<T>({str.str().c_str()}, gInterpreter));
    if(std::is_same<T, bool>::value) { // special case
     filteredValue = std::regex_replace(filteredValue, std::regex("(?:true)"), "1");
     filteredValue = std::regex_replace(filteredValue, std::regex("(?:false)"), "0");
     ///////// DEBUG
     //     std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue<bool>): ################################ processed VAL: " << std::quoted(filteredValue) << RST << std::endl;
     ///////// DEBUG
    }
    ///////// DEBUG
    //    if(std::is_same<T, std::string>::value) { // special case
    //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue<string>): ################################ processed VAL: " << std::quoted(filteredValue) << RST << std::endl;
    //    }
    ///////// DEBUG

    v.value = boost::lexical_cast<T>(filteredValue);

    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myValue): ################################ output VAL: " << v.value << RST << std::endl;
    ///////// DEBUG
    return is;
    }
};
// only these specialized empty constructors are valid
template <> ConfigManager::myValue<std::string>::myValue()  : value("")    { }
template <> ConfigManager::myValue<int>::myValue()          : value(0)     { }
template <> ConfigManager::myValue<unsigned int>::myValue() : value(0U)    { }
template <> ConfigManager::myValue<float>::myValue()        : value(0.0)   { }
template <> ConfigManager::myValue<double>::myValue()       : value(0.0)   { }
template <> ConfigManager::myValue<bool>::myValue()         : value(false) { }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper storage class needed for custom validator of cmd line arguments used to initialize std::vector<T>
// by BOOST program options, where T denotes: int, unsigned int, float, double, bool or std::string.
// It behavies like a normal std::vector<T> and provides two overloaded operators:
// - operator<<() - creates nice-looking text stream in JSON vector format that provides 15-16 significant digits for T=double
//   and "true/false" text labels for T=bool
// - operator>>() - allows to interpret simple math expressions at run-time for type T being: int, unisgned int, float,
//   double and bool.
//
template<typename T>
class ConfigManager::myVector: public std::vector<T> {
  typedef std::vector<T> Vector;
public:
  template<typename... Args> myVector(Args... args) : Vector(args...) { }
  /*
template<typename T>
class ConfigManager::myVector: private std::vector<T> {
  typedef std::vector<T> Vector;
public:
  typedef T value_type;
  template<typename... Args> myVector(Args... args) : Vector(args...) { }
  using Vector::at;
  using Vector::clear;
  using Vector::iterator;
  using Vector::const_iterator;
  using Vector::begin;
  using Vector::end;
  using Vector::cbegin;
  using Vector::cend;
  using Vector::crbegin;
  using Vector::crend;
  using Vector::empty;
  using Vector::size;
  using Vector::reserve;
  using Vector::operator[];
  using Vector::assign;
  using Vector::insert;
  using Vector::erase;
  using Vector::front;
  using Vector::back;
  using Vector::push_back;
  using Vector::pop_back;
  using Vector::resize;
  myVector operator=(const myVector &v) const { return v; }
  myVector operator*(const myVector & ) const;
  myVector operator+(const myVector & ) const;
  */
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //
  // Custom ConfigManager::myVector<T>::operator<<() to convert vector of type T into a text stream,
  // where T denotes: std::string, int, usigned int, float, double or bool.
  // The output is enclosed in square brackets and quoted values are separated by spaces and commas
  // to resemble array in JSON format, e.g. [ "value1", "value2", "value3" ].
  // In case of T=bool the resulting text representation will be "true" or "false".
  //
  friend std::ostream& operator<<(std::ostream & os, const ConfigManager::myVector<T> & v) {
    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ input myVector.size: " << v.size() << RST << std::endl;
    ///////// DEBUG
    os << "[ ";
    bool isFirst = true;
    for(auto item : v) { // NOTE: do not use here the reference operator (&) to enable support for std::vector<bool> 
      std::stringstream ss;
      ss << boost::lexical_cast<ConfigManager::myValue<T>>(item); // use nicer formats for double precision and bool variables
      ///////// DEBUG
      //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ input VAL element: " << item << RST << std::endl;
      ///////// DEBUG
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
  friend std::istream& operator>>(std::istream & is, ConfigManager::myVector<T> & v) {
    std::string str;
    std::vector<std::string> val;
    while(is >> str) {
      val.push_back(str);
      ///////// DEBUG
      //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ input stream unfiltered element: " << str << RST << std::endl;
      ///////// DEBUG
    }
    
    // remove existing square brackets pair if present
    auto filteredValues = ConfigManager::filterSquareBrackets(val);
    
    // convert symbolic math expressions (if any) to numerical values
    auto filteredValues2 = ConfigManager::filterMathExpressions<T>(filteredValues, gInterpreter);

    // fill array with filtered values
    BOOST_FOREACH(auto& t, filteredValues2) {

      if(std::is_same<T, bool>::value) { // special case
      	t = std::regex_replace(t, std::regex("(?:true"), "1");
      	t = std::regex_replace(t, std::regex("(?:false"), "0");
      }

      ///////// DEBUG
      //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ input stream filtered element: " << t << RST << std::endl;
      ///////// DEBUG
      T val = boost::lexical_cast<T>(t);
      v.push_back(val);
      ///////// DEBUG
      //      std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ output VAL element: " << val << RST << std::endl;
      ///////// DEBUG
    }
    ///////// DEBUG
    //    std::cout << KRED << __FUNCTION__ << "(ConfigManager::myVector): ################################ CLEAN EXIT" << RST << std::endl;
    ///////// DEBUG
    return is;
    }
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace boost {
  namespace property_tree {
    template<typename T>
    struct translator_between<std::string, ConfigManager::myValue<T>>
    {
      struct type {
	typedef std::string internal_type;
	typedef ConfigManager::myValue<T> external_type;
	boost::optional<external_type> get_value(const internal_type& str) {
	  ////// DEBUG
	  //	  if(std::is_same<T, std::string>::value) {
	  //	    std::cout << KGRN << __FUNCTION__ << "<string>: internal=\"" << str << "\"" << RST << std::endl;
	  //	  }
	  ////// DEBUG

	  if(std::is_same<T, std::string>::value) return external_type(boost::lexical_cast<T>(str)); // special case, may contain blank spaces
	  external_type ee;
	  std::stringstream ss(str);
	  ss >> ee;
	  return ee;
	}
	boost::optional<internal_type> put_value(const external_type& obj) {
	  ////// DEBUG
	  //	  if(std::is_same<T, std::string>::value) {
	  //	    std::cout << KGRN << __FUNCTION__ << "<string>: external=\"" << obj << "\"" << RST << std::endl;
	  //	  }
	  ////// DEBUG
	  std::stringstream ss;
	  ss << obj;
	  return ss.str();
	}
      };
    };
  } // namespace property_tree
} // namespace boost
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
