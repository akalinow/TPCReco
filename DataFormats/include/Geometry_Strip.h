#ifndef __STRIPTPC_H__
#define __STRIPTPC_H__
#include <memory>

#include "TROOT.h"
#include "TMath.h"
#include "TVector2.h"
#include "TGraph.h"
#include "CommonDefinitions.h"
#include "GeometryTPC.h"

struct StripData {
    direction dir; // direction/group: 0=U / 1=V / 2=W / 3=FPN / -1=ERROR               
    int section;
    int num; // strip number: 1-1024 for U,V,W / 1-(4*ASAD_N*COBO_N) for FPN / -1=ERROR
    int coboId; // range [0-1]
    int asadId; // range [0-3]
    int agetId; // range [0-3]
    int agetCh; // range [0-63]
    int agetCh_raw; // range [0-67]
    TVector2 unit_vec;   // 2D directional unit vector (towards increasing pad numbers)
    TVector2 offset_vec; // 2D offset vector [mm] of the 1st pad wrt REF.POINT  
    int npads;  // length in npads
    double length() const { return npads * Geometry().GetPadPitch(); }
};

// Single UVW strip defined as a class
class Geometry_Strip {

private:

    const StripData data;
public:

    Geometry_Strip(direction dir_, int section, int number, int cobo_index, int asad_index, int aget_index, int aget_channel, int aget_channel_raw,
        TVector2 unit_vector, TVector2 offset_vector_in_mm, int number_of_pads);

    const decltype(data)& operator()() { return data; };
};

#endif
