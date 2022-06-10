#include "PEventTPC.h"


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////  
void PEventTPC::Clear() { myChargeMap.clear(); }
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
bool PEventTPC::AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val){
  auto key = std::make_tuple(strip->Dir(), strip->Section(), strip->Num(), time_cell);
  myChargeMap[key] += val; //update hit or add new one
  return true;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& os, const PEventTPC& e) {
  os << "PEventTPC: " << e.GetEventInfo()<<"/n"
     <<" charge map size: "<<e.myChargeMap.size();
  return os;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
