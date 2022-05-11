#ifndef _HIGS_trees_dataFormat_H_
#define _HIGS_trees_dataFormat_H_

#include "TVector3.h"

struct Event_1prong{
     int EventID;
     TVector3 vertexPos;
     TVector3 endpointPos;
     float length;
     float phiDET;
     float thetaDET;
     float phiBEAM;
     float thetaBEAM;
  };
  struct Event_2prong{
     int EventID;
     TVector3 vertexPos;
     TVector3 alpha_endpointPos;
     float alpha_length;
     float alpha_phiDET;
     float alpha_thetaDET;
     float alpha_phiBEAM;
     float alpha_thetaBEAM;
     TVector3 carbon_endpointPos;
     float carbon_length;
     float carbon_phiDET;
     float carbon_thetaDET;
     float carbon_phiBEAM;
     float carbon_thetaBEAM;
  };
  struct Event_3prong{
     int EventID;
     TVector3 vertexPos;
     TVector3 alpha1_endpointPos;
     float alpha1_length;
     float alpha1_phiDET;
     float alpha1_thetaDET;
     float alpha1_phiBEAM;
     float alpha1_thetaBEAM;
     TVector3 alpha2_endpointPos;
     float alpha2_length;
     float alpha2_phiDET;
     float alpha2_thetaDET;
     float alpha2_phiBEAM;
     float alpha2_thetaBEAM;
     TVector3 alpha3_endpointPos;
     float alpha3_length;
     float alpha3_phiDET;
     float alpha3_thetaDET;
     float alpha3_phiBEAM;
     float alpha3_thetaBEAM;
  };

#endif
