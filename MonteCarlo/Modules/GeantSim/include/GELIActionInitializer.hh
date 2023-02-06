/**
 * @file GELIActionInitializer.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIActionInitializer class
 */

#ifndef GELIACTIONINITIALIZER_H
#define GELIACTIONINITIALIZER_H

#include "G4VUserActionInitialization.hh"
#include "G4VSteppingVerbose.hh"
#include "globals.hh"


/**
 * @class      GELIActionInitializer
 *
 * @brief      Action initializer for Geant4 simulation
 */
class GELIActionInitializer : public G4VUserActionInitialization
{
public:
  /**
   * @brief      Constructor
   */
  GELIActionInitializer();
  /**
   * @brief      Destructor
   */
  ~GELIActionInitializer(){;};
  /**
   * @brief      Action initializer for worker threads
   * @details    Implememntation of G4VUserActionInitialization virtual method
   */
  void Build() const;
  /**
   * @brief      Action initializer for master thread, implementation of virtual
   *             method
   * @details    Implememntation of G4VUserActionInitialization virtual method
   */
  void BuildForMaster() const;

  /**
   * @brief      Initializes stepping verbose
   */
  G4VSteppingVerbose* InitializeSteppingVerbose() const;
};


#endif

