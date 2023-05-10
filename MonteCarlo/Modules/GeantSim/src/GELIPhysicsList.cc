/**
 * @file GELIPhysicsList.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIPhysicsList class
 */

#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4PhysicsListHelper.hh"
#include "G4hMultipleScattering.hh"
#include "G4ComptonScattering.hh"
#include "G4GammaConversion.hh"
#include "G4PhotoElectricEffect.hh"
#include "G4eIonisation.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"
#include "G4ionIonisation.hh"
#include "G4IonParametrisedLossModel.hh"
#include "G4LossTableManager.hh"
#include "G4UAtomicDeexcitation.hh"
#include "G4SystemOfUnits.hh"
#include "GELIPhysicsList.hh"
#include "G4SystemOfUnits.hh"
#include "G4ParticleTypes.hh"
#include "G4UnitsTable.hh"
#include "G4ios.hh"

#include "G4eMultipleScattering.hh"



GELIPhysicsList::GELIPhysicsList() : G4VUserPhysicsList() {
    defaultCutValue = 0* millimeter;
    cutForGamma = defaultCutValue;
    cutForElectron = defaultCutValue;
    cutForPositron = defaultCutValue;
    cutForProton = defaultCutValue;

    SetVerboseLevel(1);
}


GELIPhysicsList::~GELIPhysicsList() = default;


void GELIPhysicsList::ConstructParticle() {
    // In this method, static member functions should be called
    // for all particles which you want to use.
    // This ensures that objects of these particle types will be
    // created in the program.

    ConstructBosons();
    ConstructLeptons();
    ConstructBarions();
    ConstructIons();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
void GELIPhysicsList::ConstructIons() {

    G4Deuteron::DeuteronDefinition();
    G4Triton::TritonDefinition();
    G4Alpha::AlphaDefinition();
    G4GenericIon::GenericIonDefinition();
}

void GELIPhysicsList::ConstructBosons() {

    // gamma
    G4Gamma::GammaDefinition();

    // optical photon
    G4OpticalPhoton::OpticalPhotonDefinition();
}

void GELIPhysicsList::ConstructLeptons() {
    // leptons
    G4Electron::ElectronDefinition();
    G4Positron::PositronDefinition();
}


void GELIPhysicsList::ConstructBarions() {
    //  barions
    G4Proton::ProtonDefinition();
    G4AntiProton::AntiProtonDefinition();
}


void GELIPhysicsList::ConstructProcess() {
    AddTransportation();
    ConstructEM();

}


void GELIPhysicsList::ConstructEM() {
    G4PhysicsListHelper *ph = G4PhysicsListHelper::GetPhysicsListHelper();
    auto particleIterator = GetParticleIterator();
    particleIterator->reset();
    while ((*particleIterator)()) {
        G4ParticleDefinition *particle = particleIterator->value();
        G4ProcessManager *pmanager = particle->GetProcessManager();
        G4String particleName = particle->GetParticleName();

        auto eIoni = new G4eIonisation();
        auto eBrem = new G4eBremsstrahlung();
        eIoni->SetStepFunction(0.000001, 500 * um);

        if (particleName == "gamma") {
            //gamma
            ph->RegisterProcess(new G4PhotoElectricEffect(), particle);
            ph->RegisterProcess(new G4ComptonScattering(), particle);
            ph->RegisterProcess(new G4GammaConversion(), particle);
        } else if (particleName == "e-") {
            //electron
            ph->RegisterProcess(eIoni, particle);
            ph->RegisterProcess(eBrem, particle);
            ph->RegisterProcess(new G4eMultipleScattering(), particle);
        } else if (particleName == "e+") {
            //positron
            ph->RegisterProcess(eIoni, particle);
            ph->RegisterProcess(eBrem, particle);
            ph->RegisterProcess(new G4eMultipleScattering(), particle);
            ph->RegisterProcess(new G4eplusAnnihilation(), particle);
        } else if (particleName == "proton") {
            //proton
            pmanager->AddProcess(new G4hMultipleScattering, -1, 1, 1);


        } else if (particleName == "alpha" ||
                   particleName == "He3") {

            auto ionIoni = new G4ionIonisation();
            ionIoni->SetStepFunction(0.000001, 50 * um);
            ph->RegisterProcess(ionIoni, particle);
            //pmanager->AddProcess(new G4hMultipleScattering,-1, 1,1);

        } else if (particleName == "GenericIon") {

            auto ionIoni = new G4ionIonisation();
            ionIoni->SetStepFunction(0.000001, 50 * um);
            pmanager->AddProcess(ionIoni,     -1, 2,2);


        } else if (particleName == "anti_proton") {
            //antiproton
            pmanager->AddProcess(new G4hMultipleScattering, -1, 1, 1);
        }
    }
}


void GELIPhysicsList::SetCuts() {
    if (verboseLevel > 0) {
        G4cout << "GELIPhysicsList::SetCuts:";
        G4cout << "CutLength : " << G4BestUnit(defaultCutValue, "Length") << G4endl;
    }

    // set cut values for gamma at first and for e- second and next for e+,
    // because some processes for e+/e- need cut values for gamma
    SetCutValue(cutForGamma, "gamma");
    SetCutValue(cutForElectron, "e-");
    SetCutValue(cutForPositron, "e+");

    // set cut values for proton and anti_proton before all other hadrons
    // because some processes for hadrons need cut values for proton/anti_proton
    SetCutValue(cutForProton, "proton");
    SetCutValue(cutForProton, "anti_proton");

    //  SetCutValueForOthers(defaultCutValue);

    if (verboseLevel > 0) DumpCutValuesTable();
}


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....






