/**
 * @file GELIEventAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIEventAction class
 */

#ifndef GELIEventAction_h
#define GELIEventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"
#include "GELIAnalysisManager.hh"


/**
 * 
 *
 * @brief      Class to handle event action in the simulation
 */
class GELIEventAction : public G4UserEventAction
{
  public:
  
    /**
     * @brief      Constructor
     * @details    It takes a pointer to GELIAnalysisManager. It is important,
     *             that the GELIAnalysisManager class instance is created in
     *             GELIActionInitializer, only in this configuration it works
     *             properly in MT mode
     *
     */
    GELIEventAction(GELIAnalysisManager *ana);
   ~GELIEventAction();

    /**
     * @brief      This method is called at the begining of each event
     * @details    It is used to print the number of current event to the
     *             console
     */
    void BeginOfEventAction(const G4Event*);
    /**
     * @brief      This method is called at the end of each event @detaild It is
     *             used to store information about the event to output file(s)
     *
     */
    void   EndOfEventAction(const G4Event*);
    
    /**
     * @brief      Sets the modulo value for printing current event number
     */
    void SetPrintModulo(G4int    val)  {printModulo = val;};
        
  private:
    GELIAnalysisManager* analysis; ///< Pointer to GELIAnalysisManager object that handles file output
    G4int printModulo; ///< Modulo printing value for event number
};

#endif

    
