/**
 * @file GELIEventAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIEventAction class
 */

#ifndef GELIEventAction_h
#define GELIEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "DataBuffer.h"


/**
 * 
 *
 * @brief      Class to handle event action in the simulation
 */
class GELIEventAction : public G4UserEventAction {
public:

    /**
     * @brief      Constructor
     * @details    It takes a pointer to GELIAnalysisManager. It is important,
     *             that the GELIAnalysisManager class instance is created in
     *             GELIActionInitializer, only in this configuration it works
     *             properly in MT mode
     *
     */
    explicit GELIEventAction(DataBuffer &buf) : buffer{buf} {}

    /**
     * @brief      This method is called at the end of each event @detaild It is
     *             used to store information about the event to output file(s)
     *
     */
    void EndOfEventAction(const G4Event *) override;


private:
    DataBuffer &buffer;
};

#endif

    
