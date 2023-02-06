/**
 * @file GELIActionInitializer.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIActionInitializer class
 */

#include "GELIActionInitializer.hh"
#include "GELIDetectorConstruction.hh"
#include "GELIPrimaryGeneratorAction.hh"
#include "GELIRunAction.hh"
#include "GELIEventAction.hh"
#include "GELISteppingAction.hh"
#include "GELITrackingAction.hh"
#include "GELISteppingVerbose.hh"
#include "GELIAnalysisManager.hh"
#include "G4RunManager.hh"


GELIActionInitializer::GELIActionInitializer() : 
  G4VUserActionInitialization()
{
}


void GELIActionInitializer::Build() const 
{

  SetUserAction(new GELIPrimaryGeneratorAction());
  GELIAnalysisManager * analysis = new GELIAnalysisManager();
  //Optional user classes
  SetUserAction(new GELIRunAction(analysis));
  SetUserAction(new GELIEventAction(analysis));
  SetUserAction(new GELISteppingAction(analysis));
  SetUserAction(new GELITrackingAction());
}


void GELIActionInitializer::BuildForMaster() const
{
  GELIAnalysisManager * analysis = new GELIAnalysisManager();
  SetUserAction(new GELIRunAction(analysis));
}

G4VSteppingVerbose* GELIActionInitializer::InitializeSteppingVerbose() const
{
  return new GELISteppingVerbose();
}

