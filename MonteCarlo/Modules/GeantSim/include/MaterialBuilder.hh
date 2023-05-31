/**
 * @file MaterialBuilder.hh
 * @author     Piotr Podlaski
 * @brief      Definition of MaterialBuilder class
 */

#ifndef MATERIALBUILDER_H
#define MATERIALBUILDER_H

#include "G4String.hh"
#include "G4Material.hh"
/// \cond
#include <map>
/// \endcond

class G4NistManager;

/**
 * @class      MaterialBuilder
 *
 * @brief      Builds materials used in geometry
 * @details    Singleton class used to build and get access to the materials
 *             that are used in the detector geometry.
 */


class MaterialBuilder {
    /**
     * Type to hold map of the materials
     */
    typedef std::map<G4String, G4Material *> Materials;
public:
    /**
     * @brief      Access to instance of the class
     *
     * @return     Pointer to the unique instance
     */
    static MaterialBuilder *GetInstance();

    /**
     * @brief      Access to the material map
     *
     * @return     Map of the defined materials
     */
    Materials GetMaterials();

    /**
     * @brief      Acess to single material
     */
    G4Material *GetMaterial(const G4String &material_name);

private:
    /**
     * Holds the pointer to unqe class instance
     */
    static MaterialBuilder *instance;

    /**
     * @brief      Constructor
     */
    MaterialBuilder();

    /**
     * @brief      Builds materials and puts their pointers into materials map
     */
    void BuildMaterials();

    /**
     * Map with materials
     */
    Materials materials;
    /**
     * Pointer to G4NistManager
     */
    G4NistManager *nist_manager;
};

#endif