#include "StripTPC.h"
#include "GeometryTPC.h"
#include "colorText.h"


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
StripTPC::StripTPC(int direction, int section, int number, int cobo_index,
                   int asad_index, int aget_index, int aget_channel,
                   int aget_channel_raw, TVector2 unit_vector,
                   TVector2 offset_vector_in_mm, int number_of_pads,
                   GeometryTPC *g)
    : geo_ptr(g), dir(direction), section(section), num(number),
      coboId(cobo_index), asadId(asad_index), agetId(aget_index),
      agetCh(aget_channel), agetCh_raw(aget_channel_raw), unit_vec(unit_vector),
      offset_vec(offset_vector_in_mm), npads(number_of_pads) {}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
int StripTPC::GlobalCh() {
  if (geo_ptr && geo_ptr->IsOK())
    return geo_ptr->Global_normal2normal(coboId, asadId, agetId, agetCh);
  return ERROR;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
int StripTPC::GlobalCh_raw() {
  if (geo_ptr && geo_ptr->IsOK())
    return geo_ptr->Global_normal2raw(coboId, asadId, agetId, agetCh);
  return ERROR;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
double StripTPC::Length() { return geo_ptr->GetPadPitch()*npads; } // [mm]
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& os, const StripTPC& aStrip) {
  os << "StripTPC: dir:"<<aStrip.dir<<" sec: "<<aStrip.section<<" num: "<<aStrip.num
     <<" coboId: "<<aStrip.coboId<<" asadId: "<<aStrip.asadId<<" agetId: "<<aStrip.agetId
     <<" agetCh: "<<aStrip.agetCh<<" agetCh_raw: "<<aStrip.agetCh_raw
     <<" npads: "<<aStrip.npads;
  return os;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
TVector2 StripTPC::Start() {
  ////// DEBUG
  //  std::cout << __FUNCTION__ << ": dir=" << dir << ", sec=" << section << ", num=" << num << ", npads=" << npads << ": ";
  //  (offset_vec + geo_ptr->GetReferencePoint() - 0.5*unit_vec*geo_ptr->GetPadPitch()).Print();
  ////// DEBUG
  if (geo_ptr){ // skip strict checking to allow proper initialization of statistics during Load()
    return offset_vec + geo_ptr->GetReferencePoint() - 0.5*unit_vec*geo_ptr->GetPadPitch();
  }
  else{
    std::cerr << __FUNCTION__ << KRED <<": ERROR: Invalid geometry pointer!" << RST << std::endl;
    exit(-1);
  }
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
TVector2 StripTPC::End() {
  ////// DEBUG
  //  std::cout << __FUNCTION__ << ": dir=" << dir << ", sec=" << section << ", num=" << num << ", npads=" << npads << ": ";
  //  (offset_vec + geo_ptr->GetReferencePoint() + (npads-0.5)*unit_vec*geo_ptr->GetPadPitch()).Print();
  ////// DEBUG
  if (geo_ptr){ // skip strict checking to allow proper initialization of statistics during Load()
    return offset_vec + geo_ptr->GetReferencePoint() + (npads-0.5)*unit_vec*geo_ptr->GetPadPitch();
  }
  else {std::cerr << __FUNCTION__ << KRED <<": ERROR: Invalid geometry pointer!" << RST << std::endl;
    exit(-1);
  }
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
