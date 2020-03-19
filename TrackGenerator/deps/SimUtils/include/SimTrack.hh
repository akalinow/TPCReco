/**
 * @file SimTrack.hh
 * @author     Piotr Podlaski
 * @brief      Definition of SimTrack class
 */

#ifndef SIMTRACK_H
#define SIMTRACK_H

#include "TObject.h"
#include "TVector3.h"
#include "SimHit.hh"
/// \cond
#include <vector>
/// \endcond



/**
 * @brief      Class to hold information about primary tracks generated in GEANT4 simulation
 */
class SimTrack
{
private:
	TVector3 startPos; ///< Position of particle emission in mm
	TVector3 stopPos; ///< Position of particle stop/exit point from simulation volume in mm
	unsigned int A; ///< Mass number of a particle
	unsigned int Z; ///< Atomic number of a particle
	unsigned int particleID; ///< PDG ID of a particle (to allow identification of electrons, gammas etc. as primary particles)
	double energy; ///< Energy of a particle in keV
	double length; ///< Length of a particle's track in mm
	TVector3 momentum; ///< Momentum of a particle in keV/c
	SimHits hits; ///<Vector of simulated hits in detector volume
public:
	/**
	 * @brief      Constructor
	 */
	SimTrack();
	// setters:
	
	void SetStart(TVector3 start);
	void SetStart(double x, double y, double z);
	void SetStop(TVector3 stop);
	void SetStop(double x, double y, double z);
	void SetMomentum(TVector3 mom);
	void SetMomentum(double px, double py, double pz);
	void SetA(unsigned int a);
	void SetZ(unsigned int z);
	void SetID(unsigned int ID);
	void SetEnergy(double E);
	
	/**
	 * @brief      Inserts SimHit object to a vector of hits
	 */
	void InsertHit(SimHit hit);
	//getters:
	TVector3 GetStart() { return startPos; }
	TVector3 GetStop() { return stopPos; }
	TVector3 GetMomentum() { return momentum; }
	double GetEnergy();
	unsigned int GetA() { return A; }
	unsigned int GetZ() { return Z; }
	unsigned int GetID() { return particleID; }
	double GetLength();
	double GetEdep(); ///< Energy deposit in gas volume of a given track
	unsigned int GetNHits();
	SimHits GetHits();
	SimHitsIterator HitsBegin();
	SimHitsIterator HitsEnd();
	ClassDef(SimTrack,1); ///< ROOT macro to register SimTrack class

};

typedef std::vector<SimTrack> SimTracks;
typedef SimTracks::iterator SimTracksIterator;

#endif