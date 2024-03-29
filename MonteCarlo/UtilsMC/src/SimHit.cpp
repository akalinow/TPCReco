#include "TPCReco/SimHit.h"

#include <utility>


SimHit::SimHit() {
    position = TVector3();
    Edep = 0;
}

SimHit::SimHit(const TVector3& pos, double edep) {
    position = pos;
    Edep = edep;
}

SimHit::SimHit(double x, double y, double z, double edep) {
    position = TVector3{x, y, z};
    Edep = edep;
}

void SimHit::SetPosition(const TVector3 &pos) {
    position = pos;
}

void SimHit::SetEnergy(double &E) {
    Edep = E;
}

TVector3 SimHit::GetPosition() const {
    return position;
}

double SimHit::GetEnergy() const {
    return Edep;
}

void SimHit::SetInside(bool in) {
    isInside=in;
}

bool SimHit::IsInside() const {
    return isInside;
}
