/**
 * @file CentralConfig.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of CentralConfig class
 */

#include <CentralConfig.hh>

namespace pt = boost::property_tree;

CentralConfig *CentralConfig::instance = nullptr;

CentralConfig *CentralConfig::GetInstance() {
    if (instance == nullptr)
        instance = new CentralConfig();
    return instance;
}


void CentralConfig::SetTopNode(const boost::property_tree::ptree &node) {
    topNode = node;
    initialized = true;
}

