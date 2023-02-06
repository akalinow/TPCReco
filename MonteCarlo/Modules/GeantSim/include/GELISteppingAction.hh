/**
 * @file GELISteppingAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELISteppingAction class
 */

#ifndef GELISteppingAction_h
#define GELISteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "G4VPVParameterisation.hh"
#include "G4PVParameterised.hh"
#include "G4Tubs.hh"
#include "GELIAnalysisManager.hh"

class GELIDetectorConstruction;
class GELIAnalysisManager;


/**
 * @class      GELISteppingAction
 *
 * @brief      Class handles action executed during track steps
 */
class GELISteppingAction : public G4UserSteppingAction
{
public:

  
  /**
   * @brief      Constructor
   *
   * @details    It takes a pointer to GELIAnalysisManager. It is important,
   *             that the GELIAnalysisManager class instance is created in
   *             GELIActionInitializer, only in this configuration it works
   *             properly in MT mode
   */
  GELISteppingAction(GELIAnalysisManager* ana);
  
  ~GELISteppingAction();
  
  /**
   * @brief      Method invoked at each step
   *
   * @details    It is used to store energy deposit information into files
   */
  void UserSteppingAction(const G4Step*);
  
private: 
  GELIAnalysisManager* analysis; ///< Pointer to GELIAnalysisManager handling file storage
  
};

#endif




