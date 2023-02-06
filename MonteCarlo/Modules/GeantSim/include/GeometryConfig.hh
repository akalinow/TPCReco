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
/// \endcond
#include <SolidDescriptor.hh>
#include <G4String.hh>
#include <G4ThreeVector.hh>
#include <G4Color.hh>
#include <CentralConfig.hh>
#include "globals.hh"
/// \cond
#include "pugixml.hh"
/// \endcond

/**
 * @brief      Class handles parsing and interface to geometry config file
 * @details    Implemented as a singleton
 */
class GeometryConfig
{
public:

	/**
	 * @brief      Access to instance of the class
	 */
	static GeometryConfig* GetInstance();

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
	static GeometryConfig* instance; ///< Pointer to uniqe instance of GeometryConfig class
	std::vector<SolidDescriptor> solids; ///< Vector with solids
	//void ParseGeometry();
	std::string config_name="geometry.xml"; ///< Name of geometry config file
	CentralConfig* central_config; ///< Pointer to CentralConfig object
	pugi::xml_document config; ///< XML parser node for top level document
	pugi::xml_parse_result result; ///< Result of XML parser
	std::map<std::string, G4Color> material_colors; ///< Map with material colors 
	std::string path_to_stl; ///< Parth to directory with 3D model STL files
	
};

#endif