/**
 * @file GELISteppingVerbose.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELISteppingVerbose class
 */

#ifndef GELISteppingVerbose_h
#define GELISteppingVerbose_h 1

#include "G4SteppingVerbose.hh"

/**
 * @brief      Class handles steping verbose
 */
class GELISteppingVerbose : public G4SteppingVerbose {

public:   
  
  /**
   * @brief      Constructor
   */
  GELISteppingVerbose();

  /**
   * @brief      Destructor
   */
  ~GELISteppingVerbose();
  
  /**
   * @brief      Drop step info
   */
  void StepInfo();

  /**
   * @brief      Drop info on tracking start
   */
  void TrackingStarted();

};

#endif
