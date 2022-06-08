#ifndef __STRIPTPC_H__
#define __STRIPTPC_H__

#include "TVector2.h"

class GeometryTPC;

class StripTPC {

  friend class GeometryTPC;

 private:

  GeometryTPC *geo_ptr; 
  int dir; // direction/group: 0=U / 1=V / 2=W / 3=FPN / -1=ERROR  
  int section; // 0 =- / 1 =A / 2=B                                                    
  int num; // strip number: 1-1024 for U,V,W / 1-(4*ASAD_N*COBO_N) for FPN / -1=ERROR
  int coboId; // range [0-1]
  int asadId; // range [0-3]
  int agetId; // range [0-3]
  int agetCh; // range [0-63]
  int agetCh_raw; // range [0-67]
  TVector2 unit_vec;   // 2D directional unit vector (towards increasing pad numbers)
  TVector2 offset_vec; // 2D offset vector [mm] of the 1st pad wrt REF.POINT
 // double length;  // strip length [mm]
  int npads; //strip length in pads
 public:

  StripTPC(){};
  StripTPC(int direction, int section, int number, int cobo_index, int asad_index, int aget_index, int aget_channel, int aget_channel_raw, 
	   TVector2 unit_vector, TVector2 offset_vector_in_mm, int number_of_pads, GeometryTPC *geo_ptr);

  inline int Dir() { return dir; }
  inline int Section() { return section; }
  inline int Num() { return num; }
  inline int CoboId() { return coboId; }
  inline int AsadId() { return asadId; }
  inline int AgetId() { return agetId; }
  inline int AgetCh() { return agetCh; }
  inline int AgetCh_raw() { return agetCh_raw; }
  int GlobalCh();
  int GlobalCh_raw();
  inline TVector2 Unit() { return unit_vec; } // ([mm],[mm])
  inline TVector2 Offset() { return offset_vec; } // ([mm],[mm])
  double Length();
  inline int Npads() {return npads;}

};

#endif
