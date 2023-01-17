/**
 * @file SimTrack.hh
 * @author     Piotr Podlaski
 * @brief      Definition of SimTrack class
 */

#ifndef SIMTRACK_H
#define SIMTRACK_H

#include "TObject.h"
#include "TVector3.h"
#include "SimHit.h"
#include "Math/Point3D.h"
#include "Math/Vector3D.h"
#include "CommonDefinitions.h"
/// \cond
#include <vector>
/// \endcond



/**
 * @brief      Class to hold information about primary tracks generated in GEANT4 simulation
 */
class SimTrack
{
private:
    ROOT::Math::XYZPoint startPos; ///< Position of particle emission in mm
    ROOT::Math::XYZPoint stopPos; ///< Position of particle stop/exit point from simulation volume in mm
	unsigned int A; ///< Mass number of a particle
	unsigned int Z; ///< Atomic number of a particle
	unsigned int pdgID; ///< PDG ID of a particle (to allow identification of electrons, gammas etc. as primary particles)
	pid_type particleID; ///< Particle ID from enum in TPCreco
	double energy; ///< Energy of a particle in MeV
	double length; ///< Length of a particle's track in mm
	ROOT::Math::XYZVector momentum; ///< Momentum of a particle in MeV/c
	SimHits hits; ///<Vector of simulated hits in detector volume
public:
	/**
	 * @brief      Constructor
	 */
	SimTrack();
	// setters:
	
	void SetStart(ROOT::Math::XYZPoint start);
	void SetStart(double x, double y, double z);
	void SetStop(ROOT::Math::XYZPoint stop);
	void SetStop(double x, double y, double z);
	void SetMomentum(ROOT::Math::XYZVector mom);
	void SetMomentum(double px, double py, double pz);
	void SetA(unsigned int a);
	void SetZ(unsigned int z);
	void SetPDG(unsigned int ID);
    void SetID(pid_type id);
	void SetEnergy(double E);
	
	/**
	 * @brief      Inserts SimHit object to a vector of hits
	 */
	void InsertHit(const SimHit& hit);
	//getters:
    ROOT::Math::XYZPoint GetStart() const { return startPos; }
    ROOT::Math::XYZPoint GetStop() const { return stopPos; }
    ROOT::Math::XYZVector GetMomentum() const { return momentum; }
    double GetEnergy() const { return energy; }
	unsigned int GetA() const { return A; }
	unsigned int GetZ() const { return Z; }
	unsigned int GetPDG() const { return pdgID; }
    pid_type GetID() const { return particleID; }
	double GetLength() const {return length;};
	double GetEdep() const; ///< Energy deposit in gas volume of a given track
	unsigned int GetNHits() const { return hits.size(); }
	SimHits GetHits() const { return hits; }
	SimHitsIterator HitsBegin() {return hits.begin(); }
	SimHitsIterator HitsEnd() { return hits.end(); }
	ClassDef(SimTrack,1); ///< ROOT macro to register SimTrack class

};

typedef std::vector<SimTrack> SimTracks;
typedef SimTracks::iterator SimTracksIterator;

#endif