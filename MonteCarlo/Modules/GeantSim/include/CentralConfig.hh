/**
 * @file CentralConfig.hh
 * @author     Piotr Podlaski
 * @brief      Definition of CentralConfig class
 */

#ifndef CENTRALCONFIG_H
#define CENTRALCONFIG_H
/// \cond
#include <iostream>
#include "pugixml.hh"

#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <string>
/// \endcond

/**
 * @brief      Class handles parsing central configuration file and access to
 *             all its fields.
 * @details    Central configuration is accessed from many places in the
 *             framework, so CentralConfig was implemented as a singleton, to
 *             parse configuration only once. Class provides interface for XML
 *             parser
 */
class CentralConfig
{
public:

	/**
	 * @brief      Access to pointer to unique class instance
	 */
	static CentralConfig* GetInstance(std::string configFileName="");

	/**
	 * @brief      Variadic function template to allow access to string value of
	 *             multi-level XML config file
	 * @details    Accepts arbitraty number of std::string arguments to acces
	 *             any level of the config file
	 */
	template<typename... Ts> //varidadic template to allow multi-level config parsing
	std::string Get(std::string field_name,Ts... args)
	{
		std::string res;
		res=GetChild(config, field_name,args...).text().as_string();
		//std::cout<<res<<std::endl;
		return res;
	}
	/**
	 * @brief      Variadic function template to allow access to integer value
	 *             of multi-level XML config file
	 * @details    Accepts arbitraty number of std::string arguments to acces
	 *             any level of the config file
	 */
	template<typename... Ts> //varidadic template to allow multi-level config parsing
	int GetI(std::string field_name,Ts... args)
	{
		std::string res;
		res=GetChild(config, field_name,args...).text().as_string();
		//std::cout<<res<<std::endl;
		return std::stoi(res);
	}
	/**
	 * @brief      Variadic function template to allow access to float value of
	 *             multi-level XML config file
	 * @details    Accepts arbitraty number of std::string arguments to acces
	 *             any level of the config file
	 */
	template<typename... Ts> //varidadic template to allow multi-level config parsing
	double GetD(std::string field_name,Ts... args)
	{
		std::string res;
		res=GetChild(config, field_name,args...).text().as_string();
		//std::cout<<res<<std::endl;
		return std::stof(res);
	}

	/**
	 * @brief      Check if field exists in onfig file
	 */
	bool Has(std::string field_name);

private:

	/**
	 * @brief      Constructor
	 */
	CentralConfig(std::string configFileName="");

	/**
	 * @brief      Acess to child node in the document
	*/
	pugi::xml_node GetChild(pugi::xml_node parent, std::string field_name);
	/**
	 * @brief      Variadic function template to allow access to XML node in
	 *             multi level config file
	 */
	template<typename... Ts>
	pugi::xml_node GetChild(pugi::xml_node parent, std::string field_name, Ts... args)
	{
		return GetChild(parent.child(field_name.c_str()),args...);
	}

	static CentralConfig* instance; ///< Pointer to unique instance of the class
	pugi::xml_document config; ///< Top level XML node
	pugi::xml_parse_result result; ///< XML parsing result
};

#endif
