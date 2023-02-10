/**
 * @file GeometryConfig.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GeometryConfig class
 */

#ifndef GEOMETRYCONFIG_H
#define GEOMETRYCONFIG_H

/// \cond
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include "boost/property_tree/ptree.hpp"
/// \endcond
#include <SolidDescriptor.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <G4Color.hh>
#include <CentralConfig.hh>
#include "globals.hh"

/**
 * @brief      Class handles parsing and interface to geometry config file
 * @details    Implemented as a singleton
 */
class GeometryConfig {
public:

    /**
     * @brief      Access to instance of the class
     */
    static GeometryConfig *GetInstance();

    /**
     * @brief      Access to information about solids in the detector geometry
     */
    std::vector<SolidDescriptor> GetSolids();

private:

    /**
     * @brief      Constructor
     */
    GeometryConfig();

    /**
     * @brief      Parses information about solids from geometry config file
     */
    void ParseGeometry();

    /**
     * @brief      Parses information about colors of the materials from
     *             geometry config file
     */
    void ParseMaterialColors();

    static GeometryConfig *instance; ///< Pointer to uniqe instance of GeometryConfig class
    std::vector<SolidDescriptor> solids; ///< Vector with solids
    CentralConfig *central_config; ///< Pointer to CentralConfig object
    std::map<std::string, G4Color> material_colors; ///< Map with material colors
    std::string path_to_stl; ///< Parth to directory with 3D model STL files

    boost::property_tree::ptree geometryNode;


};

#endif