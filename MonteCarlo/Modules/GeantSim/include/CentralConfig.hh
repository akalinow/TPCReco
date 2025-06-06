/**
 * @file CentralConfig.hh
 * @author     Piotr Podlaski
 * @brief      Definition of CentralConfig class
 */

#ifndef CENTRALCONFIG_H
#define CENTRALCONFIG_H
/// \cond
#include <iostream>
#include "boost/property_tree/ptree.hpp"
#include "TPCReco/ConfigManager.h"

/// \endcond

/**
 * @brief      Class handles parsing central configuration file and access to
 *             all its fields.
 * @details    Central configuration is accessed from many places in the
 *             framework, so CentralConfig was implemented as a singleton, to
 *             parse configuration only once. Class provides interface for boost::property_tree::ptree
 *             as well as to: ConfigManager::getScalar<T>(ptree, "node"), ConfigManager::getVector<T>(ptree, "node")
 *             to allow parsing simple MATH expressions for arithemtic types: bool, int, unsigned int, float, double.
 */
class CentralConfig {
public:

    /**
     * @brief      Access to pointer to unique class instance
     */
    static CentralConfig *GetInstance();

    void SetTopNode(const boost::property_tree::ptree &node);

    template<typename T>
    T Get(std::string fieldName) {
        if (!initialized)
            throw std::runtime_error("CentralConfig is not initialized, call SetTopNode() first!");
        // return topNode.get<T>(fieldName); // without MATH expressions
        if(ConfigManager::is_std_vector_type<T>::value) { // vector type not supported
	  throw std::runtime_error("CentralConfig currently supports only scalar getter methods for BOOST ptree!");
        }
	return ConfigManager::getScalar<T>(topNode, fieldName);
    }

    boost::property_tree::ptree GetNode(const std::string& nodeName){
        return topNode.get_child(nodeName);
    }


private:

    /**
     * @brief      Constructor
     */
    explicit CentralConfig() = default;

    static CentralConfig *instance; ///< Pointer to unique instance of the class
    boost::property_tree::ptree topNode;
    bool initialized{false};
};

#endif
