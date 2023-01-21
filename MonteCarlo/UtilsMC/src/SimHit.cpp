#include "SimHit.h"

#include <utility>


SimHit::SimHit() {
    position = ROOT::Math::XYZPoint(0, 0, 0);
    Edep = 0;
}

SimHit::SimHit(ROOT::Math::XYZPoint &pos, double &edep) {
    position = std::move(pos);
    Edep = edep;
}

SimHit::SimHit(double x, double y, double z, double edep) {
    position = ROOT::Math::XYZPoint(x, y, z);
    Edep = edep;
}

void SimHit::SetPosition(ROOT::Math::XYZPoint &pos) {
    position = std::move(pos);
}

void SimHit::SetEnergy(double &E) {
    Edep = E;
}

ROOT::Math::XYZPoint SimHit::GetPosition() {
    return position;
}

double SimHit::GetEnergy() {
    return Edep;
}