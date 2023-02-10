/**
 * @file GELITrackingAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELITrackingAction class
 */

#ifndef GELITrackingAction_h
#define GELITrackingAction_h 1

#include "G4UserTrackingAction.hh"
#include "G4VPVParameterisation.hh"
#include "G4PVParameterised.hh"
#include "G4Tubs.hh"

class GELIDetectorConstruction;

class GELIAnalysisManager;


/**
 * @class      GELITrackingAction
 *
 * @brief      Class handles action executed during track steps
 */
class GELITrackingAction : public G4UserTrackingAction {
public:


    /**
     * @brief      Constructor
     *
     * @details    It takes a pointer to GELIAnalysisManager. It is important,
     *             that the GELIAnalysisManager class instance is created in
     *             GELIActionInitializer, only in this configuration it works
     *             properly in MT mode
     */
    GELITrackingAction();

    ~GELITrackingAction();

    /**
     * @brief      Method invoked at each step
     *
     * @details    It is used to store energy deposit information into files
     */
    void PreUserTrackingAction(const G4Track *aTrack);

    void PostUserTrackingAction(const G4Track *aTrack);


};

#endif




