/**
 * @file GELIPrimaryGeneratorAction.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIPrimaryGeneratorAction class
 */

#include "GELIPrimaryGeneratorAction.hh"

#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"
#include "G4Event.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Electron.hh"
#include "G4Alpha.hh"
#include "G4Gamma.hh"
#include "G4String.hh"
#include "G4GeneralParticleSource.hh"
/// \cond
#include <vector>
#include <cmath>
/// \endcond

#ifdef USE_GAMMA_BEAM_GENERATOR
  #include "GammaSource.h"
#endif
#ifdef USE_EVENT_GENERATOR
  #include "GELIEventGenerator.hh"
#endif

//storage and flag for gamma beam:
std::vector<G4double> GELIPrimaryGeneratorAction::energies;
std::vector<G4ThreeVector> GELIPrimaryGeneratorAction::positions;
std::vector<G4ThreeVector> GELIPrimaryGeneratorAction::momenta;
bool GELIPrimaryGeneratorAction::gammaPrepared=false;


//storage and flag for event generator:
std::vector<G4double> GELIPrimaryGeneratorAction::theta1;
std::vector<G4double> GELIPrimaryGeneratorAction::theta2;
std::vector<G4double> GELIPrimaryGeneratorAction::phi1;
std::vector<G4double> GELIPrimaryGeneratorAction::phi2;
std::vector<G4double> GELIPrimaryGeneratorAction::energy1;
std::vector<G4double> GELIPrimaryGeneratorAction::energy2;
bool GELIPrimaryGeneratorAction::generatorPrepared=false;

int GELIPrimaryGeneratorAction::A1;
int GELIPrimaryGeneratorAction::A2;
int GELIPrimaryGeneratorAction::Z1;
int GELIPrimaryGeneratorAction::Z2;

G4Mutex mPreparePrimaries = G4MUTEX_INITIALIZER;


GELIPrimaryGeneratorAction::GELIPrimaryGeneratorAction()
{
  gamma=G4Gamma::GammaDefinition();
  config=CentralConfig::GetInstance();
  sourcePositionOffset=config->GetD("primary_generator","GammaBeam","position_offset");
  gammaEnergy=config->GetI("primary_generator","GammaBeam","gamma_energy");
  generatorType=config->Get("primary_generator", "generator_type");
  nGammas=config->GetI("primary_generator","GammaBeam","n_gammas_to_prepare");

  nEvents=config->GetI("primary_generator","EventGenerator","n_events_to_prepare");

  generatorConfigName=config->Get("EventGenerator","config_file");
  if(generatorType=="GPS")
  {
    GPSGun=new G4GeneralParticleSource(); 
  }
  #ifdef USE_GAMMA_BEAM_GENERATOR
  else if(generatorType=="GammaBeam")
  {
    particleGun=new G4ParticleGun(1);
    particleGun->SetParticleDefinition(gamma);
    PrepareGammaPrimaries();
  }
  #endif
  #ifdef USE_EVENT_GENERATOR
  else if(generatorType=="EventGenerator")
  {
    particleGun=new G4ParticleGun(1);
    PrepareGeneratorPrimaries();
  }
  #endif
  else
    G4Exception("GELIPrimaryGeneratorAction::GELIGELIPrimaryGeneratorAction","ELITPC",FatalException,
      "Undefined primary generator, check config file. If GammaBeam or EventGenerator is selected make sure that software is compiled with support for those packages");
}


void GELIPrimaryGeneratorAction::PrepareGammaPrimaries()
{
  #ifdef USE_GAMMA_BEAM_GENERATOR
  G4AutoLock l(&mPreparePrimaries);
  if(gammaPrepared)
    return;
  GammaSource* gammaSource = new GammaSource(gammaEnergy,sourcePositionOffset);
  positions.clear();
  momenta.clear();
  positions.reserve(nGammas);
  momenta.reserve(nGammas);
  for(int i=0;i<nGammas;i++)
  {
    std::vector<double> momentum(3);
    std::vector<double> position(3);
    gammaSource->GetRandomMomentum(momentum, position);
    G4double energy= gammaSource->GetRandomEnergy()*MeV;
    positions.push_back(G4ThreeVector(position.at(0)*mm,position.at(1)*mm,position.at(2)*mm));
    momenta.push_back(G4ThreeVector(momentum.at(0),momentum.at(1),momentum.at(2)));
    energies.push_back(energy);
  }
  gammaPrepared=true;
  #endif 
}

void GELIPrimaryGeneratorAction::PrepareGeneratorPrimaries()
{
  #ifdef USE_EVENT_GENERATOR
  G4AutoLock l(&mPreparePrimaries);
  if(generatorPrepared)
    return;
  GELIEventGenerator* eventGenerator=new GELIEventGenerator();
  A1=eventGenerator->GetFirstProdMassNumber();
  A2=eventGenerator->GetSecondProdMassNumber();
  Z1=eventGenerator->GetFirstProdAtomicNumber();
  Z2=eventGenerator->GetSecondProdAtomicNumber();
  for(int i=0;i<nEvents;i++)
  {
    eventGenerator->FillVectors(theta1, theta2, phi1, phi2, energy1, energy2);
  }
  generatorPrepared=true;
  #endif

}


GELIPrimaryGeneratorAction::~GELIPrimaryGeneratorAction()
{


  if(generatorType=="GPS")
  {
    delete GPSGun;
  }
  #ifdef USE_GAMMA_BEAM_GENERATOR
  else if(generatorType=="GammaBeam")
  {
    delete particleGun;
  }
  #endif
  #ifdef USE_EVENT_GENERATOR
  else if(generatorType=="EventGenerator")
  {
    delete particleGun;
  }
  #endif
}

 
void GELIPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{

  if(generatorType=="GPS")
  {
    GPSGun->GeneratePrimaryVertex(anEvent);
  }
  #ifdef USE_GAMMA_BEAM_GENERATOR
  else if(generatorType=="GammaBeam")
  {
    int eventId=anEvent->GetEventID();
    eventId=eventId%nGammas;
    particleGun->SetParticleEnergy(energies.at(eventId));
    particleGun->SetParticlePosition(positions.at(eventId));
    particleGun->SetParticleMomentumDirection(momenta.at(eventId));
    particleGun->GeneratePrimaryVertex(anEvent);
  }
  #endif
  #ifdef USE_EVENT_GENERATOR
  else if(generatorType=="EventGenerator")
  {
    int eventId=anEvent->GetEventID();
    eventId=eventId%nEvents;
    double th1=theta1.at(eventId);
    double th2=theta2.at(eventId);
    double ph1=phi1.at(eventId);
    double ph2=phi2.at(eventId);
    
    G4ThreeVector direction1(cos(th1), sin(th1)*cos(ph1),sin(th1)*sin(ph1));
    G4ThreeVector direction2(cos(th2), sin(th2)*cos(ph2),sin(th2)*sin(ph2));
    G4IonTable* iTable = G4IonTable::GetIonTable();
    G4ParticleDefinition *firstProduct = iTable->GetIon(Z1, A1, 0.);
    G4ParticleDefinition *secondProduct = iTable->GetIon(Z2, A2, 0.);
    particleGun->SetParticleDefinition(firstProduct);
    particleGun->SetParticleEnergy(energy1.at(eventId));
    particleGun->SetParticlePosition(G4ThreeVector(0.,0.,0.));
    particleGun->SetParticleMomentumDirection(direction1);
    particleGun->GeneratePrimaryVertex(anEvent);
    particleGun->SetParticleDefinition(secondProduct);
    particleGun->SetParticleEnergy(energy2.at(eventId));
    particleGun->SetParticlePosition(G4ThreeVector(0.,0.,0.));
    particleGun->SetParticleMomentumDirection(direction2);
    particleGun->GeneratePrimaryVertex(anEvent);
  }
  #endif



}



