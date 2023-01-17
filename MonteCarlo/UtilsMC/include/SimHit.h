/**
 * @file SimHit.h
 * @author     Piotr Podlaski
 * @brief      Definition of SimHit class
 */

#ifndef SIMHIT_H
#define SIMHIT_H

#include "TObject.h"
#include "Math/Point3D.h"
/// \cond
#include <vector>
/// \endcond


/**
 * @brief      Class holding information about single energy deposit cluster
 */
class SimHit
{
private:
	ROOT::Math::XYZPoint position; ///< Position of the deposit
	double Edep; ///< Energy deposit of the hit in MeV
public:
	SimHit();
    virtual ~SimHit()=default;
	SimHit(ROOT::Math::XYZPoint &pos, double &edep);
	SimHit(double x, double y, double z, double edep);
	void SetPosition(ROOT::Math::XYZPoint &pos);
	void SetEnergy(double &E);
    ROOT::Math::XYZPoint GetPosition();
	double GetEnergy();
	ClassDef(SimHit,1); ///< ROOT macro to register SimHit class
};

typedef std::vector<SimHit> SimHits;
typedef SimHits::iterator SimHitsIterator;

#endif