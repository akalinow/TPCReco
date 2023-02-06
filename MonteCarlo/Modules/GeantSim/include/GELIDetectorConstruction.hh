/**
 * @file GELIDetectorConstruction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIDetectorConstruction class
 */
#ifndef GELIDETECTORCONSTRUCTION_H
#define GELIDETECTORCONSTRUCTION_H
/// \cond
#include <string>
/// \endcond
#include "G4MagneticField.hh"

#include "G4ThreeVector.hh"
#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4Cache.hh"



class G4VSolid;
class G4LogicalVolume;
class G4VPhysicalVolume;


/**
 * @class      GELIDetectorConstruction
 *
 * @brief      Mandatory DetectorConstruction class
 */
class GELIDetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    /** Constructor*/
    GELIDetectorConstruction();
    /** Destructor */
    ~GELIDetectorConstruction();

    /**
     * @brief      Constructs detector geometry @detaild Mandatory
     *             implementation of G4VUserDetectorConstruction virtual method
     */
    G4VPhysicalVolume* Construct();

    /**
     * @brief      Constructs sensitive detectors and fields inside geometry
     *             @detaild This method condtructs magnetic field used in the
     *             simulation, no sensitive detectors are used
     */
    void ConstructSDandField();

  private:
    G4VSolid * world_solid; ///< Pointer to solid of world volume
    G4LogicalVolume* world_logical; ///< Pointer to world logical volume
    G4VPhysicalVolume* world_physical; ///< Pointer to world physical volume
    G4Cache<G4MagneticField*> magneticField; ///< Thread local magnetic field handler
};

#endif

