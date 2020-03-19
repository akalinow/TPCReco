/**
 * @file SimHit.hh
 * @author     Piotr Podlaski
 * @brief      Definition of SimHit class
 */

#ifndef SIMHIT_H
#define SIMHIT_H

#include "TObject.h"
#include "TVector3.h"
/// \cond
#include <vector>
/// \endcond


/**
 * @brief      Class holding information about single energy deposit cluster
 */
class SimHit
{
private:
	TVector3 position; ///< Position of the deposit
	double Edep; ///< Energy deposit of the hit in keV
public:
	SimHit();
	SimHit(TVector3 pos, double edep);
	SimHit(double x, double y, double z, double edep);
	void SetPosition(TVector3 pos);
	void SetEnergy(double E);
	TVector3 GetPosition();
	double GetEnergy();
	ClassDef(SimHit,1); ///< ROOT macro to register SimHit class
};

typedef std::vector<SimHit> SimHits;
typedef SimHits::iterator SimHitsIterator;

#endif