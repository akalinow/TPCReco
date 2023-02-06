/**
 * @file GELIPrimaryGeneratorAction.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIPrimaryGeneratorAction class
 */

#ifndef GELIPrimaryGeneratorAction_h
#define GELIPrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"
#include "globals.hh"
/// \cond
#include <string>
/// \endcond
#include "CentralConfig.hh"



class G4ParticleGun;
class G4Event;
class G4DataVector;
class G4ParticleDefinition;


/**
 * @brief      Mandatory class that generates primary particles in the
 *             simulation
 *
 *             @detaild It is configured with central simulation config
 *             file.<br> Three primary generators are avaliable:
 *             <ul>
 * 	            <li> General Particle Source </li>
 *              <li> GammaBeam </li>
 *              <li> EventGenerator </li> 
 *             </ul> 
 *             Primary particles are not generated on the
 *             fly and they have to be prepared before the run. This is caused by
 *             usage of the ROOT package and thread safety in MT mode. Information
 *             about prepared primary particles is stored in vectors.
 */
class GELIPrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:

  GELIPrimaryGeneratorAction();    
  ~GELIPrimaryGeneratorAction();

  /**
   * @brief      Generates primary particles in the event
   * @details    Primary particles are generated accordingly to the config file
   */
  void GeneratePrimaries(G4Event*);
  
private:

	/**
	 * @brief      Prepares primary particles for GammaBeam generator
	 */
	void PrepareGammaPrimaries();

	/**
	 * @brief      Prepares primary particles for EventGenerator
	 */
	void PrepareGeneratorPrimaries();
	CentralConfig* config; ///< Pointer to CentralConfig object
	G4ParticleGun* particleGun; ///< Pointer to G4ParticleGun object
	G4GeneralParticleSource *GPSGun; ///< Pointer to G4GeneralParticleSource object
	std::string generatorType; ///< Type of the primary generator
	int gammaEnergy; ///<Enery index of the gamma source, see config file for details
	double sourcePositionOffset; ///< Offset of gamma source along X axis
	G4ParticleDefinition* gamma; ///< Definition of gamma particle
	//variables for gamma beam:
	static std::vector<G4double> energies; ///<Storage for gamma energies
	static std::vector<G4ThreeVector> positions; ///<Storage for gamma positions
	static std::vector<G4ThreeVector> momenta; ///<Storage for gamma momentum directions
	static bool gammaPrepared; ///< Flag to tell if gammas are prepared
	//variables for event generator:
	static std::vector<G4double> theta1;///<Storage for \f$\theta\f$ of first reaction product in EventGenerator
	static std::vector<G4double> theta2;///<Storage for \f$\theta\f$ of second reaction product in EventGenerator
	static std::vector<G4double> phi1;///<Storage for \f$\phi\f$ of first reaction product in EventGenerator
	static std::vector<G4double> phi2;///<Storage for \f$\phi\f$ of second reaction product in EventGenerator
	static std::vector<G4double> energy1;///<Storage for energy of first reaction product in EventGenerator
	static std::vector<G4double> energy2;///<Storage for energy of second reaction product in EventGenerator
	static bool generatorPrepared; ///< Flag to tell if custom events are prepared
	std::string generatorConfigName; ///< Path of the EventGenerator config file
	static int A1; ///< Mass number of the first reaction product
	static int A2; ///< Mass number of the second reaction product
	static int Z1; ///< Atomic number of the first reaction product
	static int Z2; ///< Atomic number of the second reaction product
	int nGammas; ///< Number of gammas to prepare
	int nEvents; ///<Number of events to prepare
};

#endif


