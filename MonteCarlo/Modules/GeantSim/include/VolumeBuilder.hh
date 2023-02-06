/**
 * @file VolumeBuilder.hh
 * @author     Piotr Podlaski
 * @brief      Definition of VolumeBuilder class
 */

#ifndef VOLUMEBUILDER_H
#define VOLUMEBUILDER_H

/// \cond
#include <iostream>
#include <memory>
#include <map>
/// \endcond
#include "G4String.hh"
#include "G4Material.hh"
#include "CADMesh.hh"


class G4VSolid;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;
class G4VisAttributes;
class MaterialBuilder;
struct SolidDescriptor;



/**
 * @class      VolumeBuilder
 *
 * @brief      Creates volumes in detector geometry
 * @details    Class handles creation of solids, physical and logical volumes in
 *             the detector geometry. It assingnes materials to the volumes and
 *             does placement in the mother volume
 */
class VolumeBuilder
{
public:

	/**
	 * @brief      Constructs the object.
	 */
	VolumeBuilder();

	/**
	 * @brief      Builds volumes.
	 */
	void BuildVolumes(G4LogicalVolume* world_phys);
private:

	/**
	 * @brief      Builds a solid.
	 */
	G4VSolid* BuildSolid(SolidDescriptor solid);

	/**
	 * @brief      Builds a logical.volume
	 */
	G4LogicalVolume* BuildLogical(SolidDescriptor solid);

	std::map<G4String,G4VSolid*> solids; ///< Map to hold pointers to created solids
	std::map<G4String,G4LogicalVolume*> logical_volumes;///< Map to hold created logical volumes
	std::map<G4String,G4VPhysicalVolume*> physical_volumes; ///< Map to hold created physical volumes
	std::map<G4String,G4VisAttributes*> vis_attributes; ///< Map to hold created visualization attributes
	/**
	 * Pointer to instance of MaterialBuilder class
	 */
	MaterialBuilder* material_builder;


};

#endif