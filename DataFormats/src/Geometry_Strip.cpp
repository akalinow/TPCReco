#include "Geometry_Strip.h"

Geometry_Strip::Geometry_Strip(projection direction, int number,
    int cobo_index, int asad_index, int aget_index, int aget_channel, int aget_channel_raw,
    TVector2 unit_vector, TVector2 offset_vector_in_mm,
    double length_in_mm) :
    data{ direction,
    number,
    cobo_index,
    asad_index,
    aget_index,
    aget_channel,
    aget_channel_raw,
    unit_vector,
    offset_vector_in_mm,
    length_in_mm } { }
