/**
 * @file GELIPhysicsList.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIPhysicsList class
 */

#ifndef GELIPhysicsList_h
#define GELIPhysicsList_h 1

#include "G4VUserPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "globals.hh"

class G4GammaConversion;

class G4ComptonScattering;

class G4PhotoElectricEffect;

class G4eIonisation;

class G4eBremsstrahlung;


class GELIPhysicsList : public G4VUserPhysicsList {
public:
    GELIPhysicsList();

    ~GELIPhysicsList() override;

protected:
    // Construct particle and physics
    void ConstructParticle() override;

    void ConstructProcess() override;

    void SetCuts() override;

private:

    G4double cutForGamma;
    G4double cutForElectron;
    G4double cutForPositron;
    G4double cutForProton;

protected:
    // these methods Construct particles
    static void ConstructBosons();

    static void ConstructLeptons();

    static void ConstructBarions();

    static void ConstructIons();

protected:

    void ConstructEM();

private:

};

#endif



