/**
 * @file GELIAnalysisManager.hh
 * @author     Piotr Podlaski
 * @brief      Definition of GELIAnalysisManager class
 */

#ifndef GELIANALYSISMANAGER_H
#define GELIANALYSISMANAGER_H

#include "globals.hh"
/// \cond
#include <vector>
#include <algorithm>
/// \endcond
#include "G4ThreeVector.hh"
#include "g4root.hh"
#include "CentralConfig.hh"
#include "SimEvent.h"
#include "TFile.h"
#include "TTree.h"



/**
 * @brief      Class to handle simulation output to files
 */
class GELIAnalysisManager
{
public:

	/**
	 * @brief      Constructor
	 */
	GELIAnalysisManager();

 	/**
 	 * @brief      Destructor
 	 */
 	~GELIAnalysisManager();
 	//For MT mode output files have to be opened in RunAction constructor,
 	//not in AnalysisManager constructor - without it merging of the Ntuples does not work
 	
 	/**
 	 * @brief      Configures simulation output
 	 */
 	static void ConfigureOutput();

 	/**
 	 * @brief      Prepare simulation output
 	 * @details    Opens output files, creates Ntuples and ROOT tree accordignly
 	 *             to config file
 	 */
 	void book();

 	/**
 	 * @brief      Finishes simulation output
 	 * @details    Saves Ntuples and ROOT tree to file and closes the file.
 	 */
 	void finish(G4int eventID);

 	/**
 	 * @brief      Fills output with single energy deposit
 	 * @details    Method is called in each  tracking step. It adds energy
 	 *             deposit to Ntuples and custom SimEvent objects. Energy
 	 *             deposit Ntuple is made of vectors to hold deposit positions
 	 *             and value. Each event has single entry in energy deposit
 	 *             NTuple. This method adds deposit info to vectors.
 	 */
 	void Fill(G4double x, G4double y, G4double z,G4double eDep, G4int event_number, G4int primID=0, bool isPrim=true);

 	/**
 	 * @brief      Saves current event to file
 	 * @details    For Ntuple it adds one Ntuple row, in SimEvent mode it fills
 	 *             the ROOT tree with current event object
 	 */
 	void SaveEvent(G4int eventID);


 	/**
 	 * @brief      Adds information about primary particle to current event
 	 */
 	void AddPrimary(G4double x, G4double y, G4double z,
 					G4double px, G4double py, G4double pz,
 					G4double energy, G4int id, G4int event_number, G4int A=0, G4int Z=0,G4int primID=0);

  
private:
	TFile* file;///< Output file for SimEvent storage
	TTree* tree;///<TTree for SimEvent storage
	CentralConfig *config; ///< CentralConfig pointer
	bool saveNtuple; ///< Flag for Ntuple storage
	bool HasEdepLimits; ///< Flag for presence of Edep limits
	bool isBackground; ///< Flag to tel if event is a backgroun event
	double xL; ///< X low limit for energy deposit storage
	double xU; ///< X up limit for energy deposit storage
	double yL; ///< Y low limit for energy deposit storage
	double yU; ///< Y up limit for energy deposit storage
	double zL; ///< Z low limit for energy deposit storage
	double zU; ///< Z up limit for energy deposit storage
	std::vector<G4double> *vx; ///< Storage for x coordinate of energy deposit in single event
	std::vector<G4double> *vy; ///< Storage for y coordinate of energy deposit in single event
	std::vector<G4double> *vz; ///< Storage for z coordinate of energy deposit in single event
	std::vector<G4int> *vID; ///< Storage for particle ID that generate energy deposit in single event
	std::vector<G4double> *vEdep; ///< Storage for value of energy deposit in single event
	std::vector<G4int> primIDs;
	std::map<int, G4ThreeVector> stops;
	int event_thread;

};
#endif




