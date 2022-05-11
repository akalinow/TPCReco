#ifndef _HIGS_trees_dataFormat_H_
#define _HIGS_trees_dataFormat_H_

#include "TVector3.h"

struct Event_1prong{
     time_t runID;
     unsigned int eventID;
     uint64_t timestamp; // internal GET electronics counter 10ns units
     float delta_timestamp; // in seconds
     TVector3 vertexPos;
     TVector3 endPos;
     float length;
     float phiDET;
     float cosThetaDET;
     float phiBEAM;
     float cosThetaBEAM;
  };
  struct Event_2prong{
     time_t runID;
     unsigned int eventID;
     uint64_t timestamp; // internal GET electronics counter 10ns units
     float delta_timestamp; // in seconds
     TVector3 vertexPos;
     TVector3 alpha_endPos;
     float alpha_length;
     float alpha_phiDET;
     float alpha_cosThetaDET;
     float alpha_phiBEAM;
     float alpha_cosThetaBEAM;
     TVector3 carbon_endPos;
     float carbon_length;
     float carbon_phiDET;
     float carbon_cosThetaDET;
     float carbon_phiBEAM;
     float carbon_cosThetaBEAM;
  };
  struct Event_3prong{
     time_t runID;
     unsigned int eventID;
     uint64_t timestamp; // internal GET electronics counter 10ns units
     float delta_timestamp; // in seconds
     TVector3 vertexPos;
     TVector3 alpha1_endPos;
     float alpha1_length;
     float alpha1_phiDET;
     float alpha1_cosThetaDET;
     float alpha1_phiBEAM;
     float alpha1_cosThetaBEAM;
     TVector3 alpha2_endPos;
     float alpha2_length;
     float alpha2_phiDET;
     float alpha2_cosThetaDET;
     float alpha2_phiBEAM;
     float alpha2_cosThetaBEAM;
     TVector3 alpha3_endPos;
     float alpha3_length;
     float alpha3_phiDET;
     float alpha3_cosThetaDET;
     float alpha3_phiBEAM;
     float alpha3_cosThetaBEAM;
  };

#endif
