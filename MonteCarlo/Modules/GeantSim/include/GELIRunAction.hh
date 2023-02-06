/**
 * @file GELIRunAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIRunAction class
 */

#ifndef GELIRunAction_h
#define GELIRunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"
/// \cond
#include <iostream>
/// \endcond
#include "GELIAnalysisManager.hh"


class G4Run;

/**
 * @brief      Class handles action at the begening and at the end of the run
 */
class GELIRunAction : public G4UserRunAction
{
public:
  
  /**
   * @brief      Constructor
   * @details    It takes a pointer to GELIAnalysisManager. It is important,
   *             that the GELIAnalysisManager class instance is created in
   *             GELIActionInitializer, only in this configuration it works
   *             properly in MT mode
   */
  GELIRunAction(GELIAnalysisManager* ana);
  ~GELIRunAction();

  /**
   * @brief      Method invoked at the begining of each run
   * @details    It is used to prepare analysis output
   */
  void BeginOfRunAction(const G4Run*);
  /**
   * @brief      Method invoked at the end of each run
   * @details    It is used to finish he analysis and save files
   */
  void EndOfRunAction(const G4Run*);
    
  /**
   * @brief      Set saving of random engine parameters
   */
  void  SetRndmFreq(G4int   val)  {saveRndm = val;}

  /**
   * @brief      Get information about saving random engine parameters
   */
  G4int GetRndmFreq()             {return saveRndm;}


private:
  G4int saveRndm; ///< Flag for saving random engine parameters at the end of the run
  GELIAnalysisManager *analysis; ///< Pointer to GELIAnalysisManager handling file storage

};

#endif













