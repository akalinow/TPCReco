#ifndef _HIGS_trees_dataFormat_H_
#define _HIGS_trees_dataFormat_H_

#include "TVector3.h"
#include "TLorentzVector.h"

struct Event_1prong{ // hypothesis: background alpha particle
  time_t runID;
  unsigned int eventID;
  uint64_t timestamp; // internal GET electronics counter 10ns units
  float delta_timestamp; // [s]
  TVector3 vertexPos; // [mm]
  TVector3 endPos; // [mm]
  float length; // [mm]
  float phiDET; // [rad]
  float cosThetaDET;
  float phiBEAM; // [rad]
  float cosThetaBEAM;
  /*
  float alpha_E; // [MeV], alpha track, kinetic energy from range, LAB frame
  TLorentzVector alpha_p4BEAM; // [MeV], alpha track, LAB frame, BEAM coords
  */
  bool XYmargin2mm; // all XY in UVW area convex hull with 2mm veto band
  bool XYmargin5mm; // all XY in UVW area convex hull with 5mm veto band
  bool XYmargin10mm; // all XY in UVW area convex hull with 10mm veto band
  bool Zmargin2mm; // global Z-span <196mm and >=2mm lower/upper margins on Z/time scale
  bool Zmargin5mm; // global Z-span <196mm and >=5mm lower/upper margins on Z/time scale
  bool Zmargin10mm; // global Z-span <196mm and >=10mm lower/upper margins on Z/time scale
};
struct Event_2prong{ // hypothesis: gamma + C-12 -> 3-alpha, isotropic in CMS
  time_t runID;
  unsigned int eventID;
  uint64_t timestamp; // internal GET electronics counter 10ns units
  float delta_timestamp; // in seconds
  TVector3 vertexPos; // [mm]
  TVector3 alpha_endPos; // [mm]
  float alpha_length; // [mm]
  float alpha_phiDET; // [rad]
  float alpha_cosThetaDET;
  float alpha_phiBEAM; // [rad], LAB frame (invariant), BEAM coords
  float alpha_cosThetaBEAM; // LAB frame, BEAM coords
  /*
  float alpha_phiBEAMcms; // [rad], CMS frame (invariant), BEAM coords
  float alpha_cosThetaBEAMcms; // CMS frame, BEAM coords
  float alpha_E; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
  float alpha_Ecms; // [MeV], alpha track hypothesis, kinetic energy from range, CMS frame
  TLorentzVector alpha_p4BEAM; // [MeV], LAB frame, BEAM coords
  TLorentzVector alpha_p4BEAMcms; // [MeV], CMS frame, BEAM coords
  */
  TVector3 carbon_endPos; // [mm]
  float carbon_length; // [mm]
  float carbon_phiDET; // [rad]
  float carbon_cosThetaDET;
  float carbon_phiBEAM; // [mm]
  float carbon_cosThetaBEAM;
  /*
  float carbon_phiBEAMcms; // [rad], CMS frame (invariant), BEAM coords
  float carbon_cosThetaBEAMcms; // CMS frame, BEAM coords
  float carbon_E; // [MeV], C-12 track hypothesis, kinetic energy from range, LAB frame
  float carbon_Ecms; // [MeV], C-12 track hypothesis, kinetic energy from range, CMS frame
  TLorentzVector carbon_p4BEAM; // [MeV], C-12 track hypothesis, LAB frame, BEAM coords
  TLorentzVector carbon_p4BEAMcms; // [MeV], C-12 track hypothesis, CMS frame, BEAM coords
  */
  bool XYmargin2mm; // all XY in UVW area convex hull with 2mm veto band
  bool XYmargin5mm; // all XY in UVW area convex hull with 5mm veto band
  bool XYmargin10mm; // all XY in UVW area convex hull with 10mm veto band
  bool Zmargin2mm; // global Z-span <196mm and >=2mm lower/upper margins on Z/time scale
  bool Zmargin5mm; // global Z-span <196mm and >=5mm lower/upper margins on Z/time scale
  bool Zmargin10mm; // global Z-span <196mm and >=10mm lower/upper margins on Z/time scale
};
struct Event_3prong{ // hypothesis: gamma + C-12 -> 3-alpha, isotropic in CMS
  time_t runID;
  unsigned int eventID;
  uint64_t timestamp; // internal GET electronics counter 10ns units
  float delta_timestamp; // [s]
  TVector3 vertexPos; // [mm]
  TVector3 alpha1_endPos; // [mm]
  float alpha1_length; // [mm]
  float alpha1_phiDET; // [rad]
  float alpha1_cosThetaDET;
  float alpha1_phiBEAM; // [rad]
  float alpha1_cosThetaBEAM;
  /*
  float alpha1_E; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
  float alpha1_Ecms; // [MeV], alpha track hypothesis, kinetic energy from range, CMS frame
  TLorentzVector alpha1_p4BEAM; // [MeV], LAB frame, BEAM coords
  TLorentzVector alpha1_p4BEAMcms; // [MeV], CMS frame, BEAM coords
  */
  TVector3 alpha2_endPos; // [mm]
  float alpha2_length; // [mm]
  float alpha2_phiDET; // [mm]
  float alpha2_cosThetaDET;
  float alpha2_phiBEAM; // [rad]
  float alpha2_cosThetaBEAM;
  /*
  float alpha2_E; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
  float alpha2_Ecms; // [MeV], alpha track hypothesis, kinetic energy from range, CMS frame
  TLorentzVector alpha2_p4BEAM; // [MeV], LAB frame, BEAM coords
  TLorentzVector alpha2_p4BEAMcms; // [MeV], CMS frame, BEAM coords
  */
  TVector3 alpha3_endPos; // [mm]
  float alpha3_length; // [mm]
  float alpha3_phiDET; // [rad]
  float alpha3_cosThetaDET;
  float alpha3_phiBEAM; // [rad]
  float alpha3_cosThetaBEAM;
  /*  float alpha3_E; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
  float alpha3_Ecms; // [MeV], alpha track hypothesis, kinetic energy from range, CMS frame
  TLorentzVector alpha3_p4BEAM; // [MeV], LAB frame, BEAM coords
  TLorentzVector alpha3_p4BEAMcms; // [MeV], CMS frame, BEAM coords
  */
  bool XYmargin2mm; // all XY in UVW area convex hull with 2mm veto band
  bool XYmargin5mm; // all XY in UVW area convex hull with 5mm veto band
  bool XYmargin10mm; // all XY in UVW area convex hull with 10mm veto band
  bool Zmargin2mm; // global Z-span <196mm and >=2mm lower/upper margins on Z/time scale
  bool Zmargin5mm; // global Z-span <196mm and >=5mm lower/upper margins on Z/time scale
  bool Zmargin10mm; // global Z-span <196mm and >=10mm lower/upper margins on Z/time scale
};

#endif
