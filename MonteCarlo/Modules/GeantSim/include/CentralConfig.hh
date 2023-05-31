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

/// \endcond

/**
 * @brief      Class handles parsing central configuration file and access to
 *             all its fields.
 * @details    Central configuration is accessed from many places in the
 *             framework, so CentralConfig was implemented as a singleton, to
 *             parse configuration only once. Class provides interface for boost::property_tree::ptree
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
        return topNode.get<T>(fieldName);
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
