/**
 * @file SolidDescriptor.hh
 * @author     Piotr Podlaski
 * @brief      Definition of SolidDescriptor struct
 */
#ifndef SOLIDDESCRIPTOR_H
#define SOLIDDESCRIPTOR_H

#include <G4Color.hh>
#include <G4ThreeVector.hh>
#include <G4RotationMatrix.hh>
#include <G4String.hh>
#include <G4Types.hh>
/// \cond
#include <vector>
#include <iostream>
/// \endcond

/**
 * @struct SolidDescriptor
 * @brief      Structure to hold information about solids inside detector
 *             geometry.
 */
struct SolidDescriptor {
public:
	G4String name; ///< name of the solid
	G4String filename; ///< filename for .stl model
	G4String material; ///< material name of the solid
	G4Color color; ///< color of the solid
};

#endif