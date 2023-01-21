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
#include "PrimaryParticle.h"
/// \cond
#include <vector>
/// \endcond



/**
 * @brief      Class to hold information about primary tracks generated in GEANT4 simulation
 */
class SimTrack {
private:
    ROOT::Math::XYZPoint stopPos; ///< Position of particle stop/exit point from simulation volume in mm
    SimHits hits; ///<Vector of simulated hits in detector volume
    PrimaryParticle prim;
public:
    /**
     * @brief      Constructor
     */
    SimTrack();
    virtual ~SimTrack() = default;
    // setters:

    void SetStop(ROOT::Math::XYZPoint stop);

    /**
     * @brief      Inserts SimHit object to a vector of hits
     */
    void InsertHit(const SimHit &hit);

    //getters:
    ROOT::Math::XYZPoint GetStart() const { return prim.GetEmissionPoint(); }

    ROOT::Math::XYZPoint GetStop() const { return stopPos; }

    double GetLength() const { return (stopPos - GetStart()).R(); }

    double GetEdep() const; ///< Energy deposit in gas volume of a given track
    unsigned int GetNHits() const { return hits.size(); }

    SimHits GetHits() const { return hits; }

    PrimaryParticle &GetPrimaryParticle() { return prim; }

    SimHitsIterator HitsBegin() { return hits.begin(); }

    SimHitsIterator HitsEnd() { return hits.end(); }

ClassDef(SimTrack, 1); ///< ROOT macro to register SimTrack class

};

typedef std::vector<SimTrack> SimTracks;
typedef SimTracks::iterator SimTracksIterator;

#endif