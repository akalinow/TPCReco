#ifndef _HIGS_trees_dataFormat_H_
#define _HIGS_trees_dataFormat_H_

#include <TVector3.h>
#include <TLorentzVector.h>
#include <TH1F.h>

struct Event_1prong{ // hypothesis: background alpha particle
  long runId;
  unsigned int eventId;
  unsigned long eventType;
  double unixTimeSec; // [s] absolute unix timestamp since 1970/1/1 epoch with millisecond precision (combined runID and GET counter)
  double runTimeSec; // [s] elapsed time since start of the runID with 10ns precision from GET counter (100 MHz clock)
  double deltaTimeSec; // [s] diference with previous event from GET counter converted to seconds (prone to glitches while switching between runIDs)
  TVector3 vertexPos; // [mm]
  TVector3 endPos; // [mm]
  float length; // [mm]
  float phiDET; // [rad]
  float cosThetaDET;
  float phiBEAM; // [rad]
  float cosThetaBEAM;
  TH1F chargeProfile; // charge profile along the track
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
struct Event_2prong{ // hypothesis: gamma + O-16 -> alpha + C-12
  long runId;
  unsigned int eventId;
  unsigned long eventType;
  double unixTimeSec; // [s] absolute unix timestamp since 1970/1/1 epoch with millisecond precision (combined runID and GET counter)
  double runTimeSec; // [s] elapsed time since start of the runID with 10ns precision from GET counter (100 MHz clock)
  double deltaTimeSec; // [s] diference with previous event from GET counter converted to seconds (prone to glitches while switching between runIDs)
  TVector3 vertexPos; // [mm]
  TVector3 alpha_endPos; // [mm]
  float alpha_length; // [mm]
  float alpha_energy; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
  float alpha_energyCMS; // [MeV], alpha track hypothesis, kinetic energy from range, CMS frame
  float alpha_phiDET; // [rad]
  float alpha_cosThetaDET;
  float alpha_phiBEAM; // [rad], LAB frame (invariant), BEAM coords
  float alpha_cosThetaBEAM; // LAB frame, BEAM coords
  TH1F alpha_chargeProfile; // charge profile along the track
  /*
  float alpha_phiBEAMcms; // [rad], CMS frame (invariant), BEAM coords
  float alpha_cosThetaBEAMcms; // CMS frame, BEAM coords
  TLorentzVector alpha_p4BEAM; // [MeV], LAB frame, BEAM coords
  TLorentzVector alpha_p4BEAMcms; // [MeV], CMS frame, BEAM coords
  */
  TVector3 carbon_endPos; // [mm]
  float carbon_length; // [mm]
  float carbon_energy; // [MeV], C-12 track hypothesis, kinetic energy from range, LAB frame
  float carbon_energyCMS; // [MeV], C-12 track hypothesis, kinetic energy from range, CMS frame
  float carbon_phiDET; // [rad]
  float carbon_cosThetaDET;
  float carbon_phiBEAM; // [mm]
  float carbon_cosThetaBEAM;
  TH1F carbon_chargeProfile; // charge profile along the track
  /*
  float carbon_phiBEAMcms; // [rad], CMS frame (invariant), BEAM coords
  float carbon_cosThetaBEAMcms; // CMS frame, BEAM coords
  TLorentzVector carbon_p4BEAM; // [MeV], C-12 track hypothesis, LAB frame, BEAM coords
  TLorentzVector carbon_p4BEAMcms; // [MeV], C-12 track hypothesis, CMS frame, BEAM coords
  */
  bool XYmargin2mm; // all XY in UVW area convex hull with 2mm veto band
  bool XYmargin5mm; // all XY in UVW area convex hull with 5mm veto band
  bool XYmargin10mm; // all XY in UVW area convex hull with 10mm veto band
  bool Zmargin2mm; // global Z-span <196mm and >=2mm lower/upper margins on Z/time scale
  bool Zmargin5mm; // global Z-span <196mm and >=5mm lower/upper margins on Z/time scale
  bool Zmargin10mm; // global Z-span <196mm and >=10mm lower/upper margins on Z/time scale
  /*
  float Qvalue_cms; // [MeV], CMS frame, reaction Q-value = E_total_cms-(alpha_mass-carbon_mass)
  float Eexcitation_cms; // [MeV], CMS frame, oxygen excitation energy = E_total_cms - (oxygen g.s.mass)
  */
};
struct Event_3prong{ // hypothesis: gamma + C-12 -> 3-alpha
  long runId;
  unsigned int eventId;
  unsigned long eventType;
  double unixTimeSec; // [s] absolute unix timestamp since 1970/1/1 epoch with millisecond precision (combined runID and GET counter)
  double runTimeSec; // [s] elapsed time since start of the runID with 10ns precision from GET counter (100 MHz clock)
  double deltaTimeSec; // [s] diference with previous event from GET counter converted to seconds (prone to glitches while switching between runIDs)
  TVector3 vertexPos; // [mm]
  TVector3 alpha1_endPos; // [mm]
  float alpha1_length; // [mm]
  float alpha1_phiDET; // [rad]
  float alpha1_cosThetaDET;
  float alpha1_phiBEAM; // [rad]
  float alpha1_cosThetaBEAM;
  TH1F alpha1_chargeProfile; // charge profile along the track
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
  TH1F alpha2_chargeProfile; // charge profile along the track
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
  TH1F alpha3_chargeProfile; // charge profile along the track
  /*
  float alpha3_E; // [MeV], alpha track hypothesis, kinetic energy from range, LAB frame
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
  /*
  float Qvalue_cms; // [MeV], CMS frame, reaction Q-value = E_total_cms-3*alpha_mass
  float Eexcitation_cms; // [MeV], CMS frame, carbon excitation energy = E_total_cms - (carbon g.s.mass)
  */
};

#endif
