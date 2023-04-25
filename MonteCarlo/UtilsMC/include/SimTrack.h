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
#include "TVector3.h"
#include "CommonDefinitions.h"
#include "PrimaryParticle.h"
/// \cond
#include <utility>
#include <vector>
/// \endcond

struct UVWTpos {
    double U{-1};
    double V{-1};
    double W{-1};
    double T{-1};
};


/**
 * @brief      Class to hold information about primary tracks generated in GEANT4 simulation
 */
class SimTrack {
public:
    /**
     * @brief      Constructor
     */
    SimTrack();

    explicit SimTrack(PrimaryParticle p, const TVector3 &start) : startPos{start}, prim{std::move(p)} {}

    virtual ~SimTrack() = default;
    // setters:

    void SetStop(const TVector3 &stop);

    void SetStart(const TVector3 &start);

    void SetTruncatedStop(const TVector3 &stop);

    void SetTruncatedStart(const TVector3 &start);

    void SetTruncatedStopUVWT(const UVWTpos &stop);

    void SetTruncatedStartUVWT(const UVWTpos &start);

    void SetPrimaryParticle(PrimaryParticle &p) { prim = p; }

    void SetOutOfActiveVolume(bool out) { isOutOfActiveVolume = out; }

    void SetFullyContained(bool cont) { isFullyContained = cont; }

    void Shift(TVector3 &offset);

    /**
     * @brief      Inserts SimHit object to a vector of hits
     */
    void InsertHit(const SimHit &hit);

    void SortHits();

    void RecalculateStopPosition();

    //getters:
    TVector3 GetStart() const { return startPos; }

    TVector3 GetStop() const { return stopPos; }

    TVector3 GetTruncatedStart() const { return truncatedStartPos; }

    TVector3 GetTruncatedStop() const { return truncatedStopPos; }

    UVWTpos GetTruncatedStartUVWT() const { return truncatedStartPosUVWT; }

    UVWTpos GetTruncatedStopUVWT() const { return truncatedStopPosUVWT; }

    double GetRange() const;

    double GetTruncatedRange() const;

    double GetEnergyDeposit() const; ///< Energy deposit in gas volume of a given track
    unsigned int GetNHits() const { return hits.size(); }

    SimHits GetHits() const { return hits; }

    const PrimaryParticle &GetPrimaryParticle() const { return prim; }

    SimHitsIterator HitsBegin() { return hits.begin(); }

    SimHitsIterator HitsEnd() { return hits.end(); }

    bool IsOutOfActiveVolume() const { return isOutOfActiveVolume; }

    bool IsFullyContained() const { return isFullyContained; }

private:
    TVector3 stopPos; ///< Position of particle stop/exit point from simulation volume in mm
    TVector3 startPos; ///< Position of particle production mm
    //Same just truncated to active volume:
    TVector3 truncatedStopPos;
    TVector3 truncatedStartPos;
    UVWTpos truncatedStartPosUVWT;
    UVWTpos truncatedStopPosUVWT;
    SimHits hits; ///<Vector of simulated hits in detector volume
    PrimaryParticle prim;
    bool hasStopPos{false};
    bool isOutOfActiveVolume{false};
    bool isFullyContained{false};

ClassDef(SimTrack, 1); ///< ROOT macro to register SimTrack class

};

typedef std::vector<SimTrack> SimTracks;
typedef SimTracks::iterator SimTracksIterator;

#endif