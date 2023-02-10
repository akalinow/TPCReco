/**
 * @file GELIPhysicsList.cc
 * @author     Piotr Podlaski
 * @brief      Implementation of GELIPhysicsList class
 */

#include "G4BuilderType.hh"
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4PhysicsListHelper.hh"

#include "G4ComptonScattering.hh"
#include "G4GammaConversion.hh"
#include "G4PhotoElectricEffect.hh"
#include "G4RayleighScattering.hh"
#include "G4KleinNishinaModel.hh"

#include "G4eIonisation.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"

#include "G4MuIonisation.hh"
#include "G4MuBremsstrahlung.hh"
#include "G4MuPairProduction.hh"

#include "G4hIonisation.hh"
#include "G4hBremsstrahlung.hh"
#include "G4hPairProduction.hh"

#include "G4ionIonisation.hh"
#include "G4IonParametrisedLossModel.hh"
#include "G4NuclearStopping.hh"

#include "G4LossTableManager.hh"
#include "G4UAtomicDeexcitation.hh"

#include "G4SystemOfUnits.hh"

#include "GELIPhysicsList.hh"

#include "G4SystemOfUnits.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleWithCuts.hh"
#include "G4ProcessManager.hh"
#include "G4ParticleTypes.hh"
#include "G4ParticleTable.hh"
#include "G4Material.hh"
#include "G4UnitsTable.hh"
#include "G4ios.hh"

#include "G4eMultipleScattering.hh"
#include "G4hMultipleScattering.hh"
// Bosons
#include "G4PhotoElectricEffect.hh"
#include "G4ComptonScattering.hh"
#include "G4GammaConversion.hh"
// Leptons
#include "G4eIonisation.hh"
#include "G4eBremsstrahlung.hh"
#include "G4eplusAnnihilation.hh"


GELIPhysicsList::GELIPhysicsList() : G4VUserPhysicsList() {
    defaultCutValue = 0 * micrometer;
    cutForGamma = defaultCutValue;
    cutForElectron = defaultCutValue;
    cutForPositron = defaultCutValue;
    cutForProton = defaultCutValue;

    SetVerboseLevel(1);
}


GELIPhysicsList::~GELIPhysicsList() {}


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
    ConstructGeneral();
}


void GELIPhysicsList::ConstructEM() {
    G4PhysicsListHelper *ph = G4PhysicsListHelper::GetPhysicsListHelper();
    auto particleIterator = GetParticleIterator();
    particleIterator->reset();
    while ((*particleIterator)()) {
        G4ParticleDefinition *particle = particleIterator->value();
        G4ProcessManager *pmanager = particle->GetProcessManager();
        G4String particleName = particle->GetParticleName();

        G4eIonisation *eIoni = new G4eIonisation();
        G4eBremsstrahlung *eBrem = new G4eBremsstrahlung();
        eIoni->SetStepFunction(0.000001, 500 * um);
        //eBrem->SetStepFunction(0.00001, 1*um);

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

            G4ionIonisation *ionIoni = new G4ionIonisation();
            ionIoni->SetStepFunction(0.000001, 50 * um);
            ph->RegisterProcess(ionIoni, particle);
            ph->RegisterProcess(new G4NuclearStopping(), particle);

        } else if (particleName == "GenericIon") {

            G4ionIonisation *ionIoni = new G4ionIonisation();
            ionIoni->SetEmModel(new G4IonParametrisedLossModel());
            ionIoni->SetStepFunction(0.000001, 50 * um);
            ph->RegisterProcess(ionIoni, particle);
            ph->RegisterProcess(new G4NuclearStopping(), particle);

        } else if (particleName == "anti_proton") {
            //antiproton
            pmanager->AddProcess(new G4hMultipleScattering, -1, 1, 1);
        }
    }
}


void GELIPhysicsList::ConstructGeneral() {}


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


void GELIPhysicsList::SetGammaLowLimit(G4double lowcut) {
    if (verboseLevel > 0) {
        G4cout << "GELIPhysicsList::SetCuts:";
        G4cout << "Gamma cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }

    SetGELowLimit(lowcut);
}


void GELIPhysicsList::SetElectronLowLimit(G4double lowcut) {
    if (verboseLevel > 0) {

        G4cout << "GELIPhysicsList::SetCuts:";
        G4cout << "Electron cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }
    SetGELowLimit(lowcut);
}


void GELIPhysicsList::SetPositronLowLimit(G4double lowcut) {
    if (verboseLevel > 0) {

        G4cout << "GELIPhysicsList::SetCuts:";
        G4cout << "Positron cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }

    G4cerr << "GELIPhysicsList::SetPositronLowLimit: Not currently able to set Positron LowLimit." << G4endl;
    G4Exception("GELIPhysicsList::SetPositronLowLimit()", "PurMag001",
                FatalException, "Positron Low Limit: not implemented in GELIPhysicsList");
    //
    // G4Positron::SetEnergyRange(lowcut,1e5);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void GELIPhysicsList::SetProtonLowLimit(G4double lowcut) {
    if (verboseLevel > 0) {

        G4cout << "GELIPhysicsList::SetCuts:";
        G4cout << "Proton cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }

    G4cerr << "GELIPhysicsList::SetProtonLowLimit: Not currently able to set Proton LowLimit." << G4endl;
    G4Exception("GELIPhysicsList::SetProtonLowLimit()", "PurMag002",
                FatalException, "Proton Low Limit: not implemented in GELIPhysicsList");
    //
    // G4Proton::SetEnergyRange(lowcut,1e5);
    // G4AntiProton::SetEnergyRange(lowcut,1e5);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void GELIPhysicsList::SetGEPLowLimit(G4double lowcut) {
    if (verboseLevel > 0) {
        G4cout << "GELIPhysicsList::SetGEPLowLimit:";
        G4cout << "Gamma and Electron cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }

    // G4Gamma::SetEnergyRange(lowcut,1e5);
    // G4Electron::SetEnergyRange(lowcut,1e5);
    // G4Positron::SetEnergyRange(lowcut,1e5);
    this->SetGELowLimit(lowcut);

    G4cerr << " SetGEPLowLimit : Uncertain whether setting Positron low limit " << G4endl;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....
void GELIPhysicsList::SetGELowLimit(G4double lowcut) {
    if (verboseLevel > 0) {
        G4cout << "GELIPhysicsList::SetGELowLimit:";
        G4cout << "Gamma and Electron cut in energy: " << lowcut * MeV << " (MeV)" << G4endl;
    }

    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(lowcut, 1e5);
}

void GELIPhysicsList::SetGammaCut(G4double val) {
    ResetCuts();
    cutForGamma = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void GELIPhysicsList::SetElectronCut(G4double val) {
    //  ResetCuts();
    cutForElectron = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void GELIPhysicsList::SetPositronCut(G4double val) {
    //  ResetCuts();
    cutForPositron = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void GELIPhysicsList::SetProtonCut(G4double val) {
    //ResetCuts();
    cutForProton = val;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....






