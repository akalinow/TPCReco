#include "TPCReco/PEventTPC.h"


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////  
void PEventTPC::Clear() {
    myChargeMap.clear();
    for (int iDir = 0; iDir < 3; ++iDir) {
        for (int iSection = 0; iSection < 3; ++iSection) {
            for (int iStrip = 0; iStrip < 256; ++iStrip) {
                for (int iCell = 0; iCell < 512; ++iCell) {
                    myChargeArray[iDir][iSection][iStrip][iCell] = 0;
                }
            }
        }
    }

}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
bool PEventTPC::AddValByStrip(const std::shared_ptr<StripTPC> &strip, int time_cell, double val) {
    auto key = std::make_tuple(strip->Dir(), strip->Section(), strip->Num(), time_cell);
    myChargeMap[key] += val;
    myChargeArray[strip->Dir()][strip->Section()][strip->Num()][time_cell] += val;
    return true;
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, const PEventTPC &e) {
    os << "PEventTPC: " << e.GetEventInfo() << "/n"
       << " charge map size: " << e.myChargeMap.size();
    return os;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
