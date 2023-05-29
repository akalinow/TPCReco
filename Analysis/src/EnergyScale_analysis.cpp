/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// Analysis class for tuning reconstructed track lengths
// by applying linear correction (scale + offset) in IonRangeCalculator:
//
//    CORRECTED_LENGTH = UNCORRECTED_LENGTH * SCALE + OFFSET.
// 
// Several collections of tracks corresponding to different 2-body decay
// hypotheses can be supplied as input.
// Provides operator() for use with external minimization fitter.
//
// Fitting options:
// - Use nominal gamma beam energy for LAB-to-CMS boost instead of reconstructed event-by-event one
// - Use only the leading track (e.g. PID=ALPHA) for caclulating properties in the CMS frame.
//   The second component will be deduced from energy and momentum conservation.
//   In this case the correction factors will be tuned only for PID corresponding to the leading track.
// - Use only multiplicative scaling factors instead of scale + offset.
//
//
// Mikolaj Cwiok (UW) - 27 May 2023
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#include <cstdlib>
#include <vector>
#include <iostream>
#include <algorithm>

#include <TVector3.h>
#include <TLorentzVector.h>

#include "TPCReco/colorText.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/EnergyScale_analysis.h"

#define DEBUG_CHI2 true

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EnergyScale_analysis::EnergyScale_analysis(const FitOptionType &aOption,
					   const std::vector<EventCollection> &aCollection,
					   const std::shared_ptr<GeometryTPC> &aGeometry,
					   const std::shared_prt<IonRangeCalculator> &aRangeCalc)
  : myOptions(aOption), myCollection(aCollection), myGeometryPtr(aGeometry) {
  
  setCalculator(aRangeCalc);
  reset();
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EnergyScale_analysis::setCalculator(const std::shared_ptr<IonRangeCalculator> &aRangeCalc) {

  // sanity check
  if(!aRangeCalc || !aRangeCalc->IsOK() ) {
    throw std::runtime_error("Wrong ion range calcualtor");
  }
  myRangeCalc=aRangeCalc;
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EnergyScale::reset() {
  
  // sanity checks
  myNparams = myOptions.tuned_pid_list.size() * 2; // scale + offset
  if(!myNparams) {
    throw std::runtime_error("Empty list of PIDs to be tuned");
  }
  if(!myCollection.size()) {
    throw std::runtime_error("Empty list of reaction hypotheses to be fitted");
  }
  for(auto &coll : myCollection) {
    if(coll.reaction != C12_ALPHA && coll.reaction != C14_ALPHA && coll.reaction != THREE_ALPHA_BE) {
      throw std::runtime_error("Unsupported reaction type to be fitted");
    }
    if(!coll.events.size()) {
      throw std::runtime_error("Empty list of events to be fitted");
    }
    if(coll.expectedGammaSigmaInMeV_LAB<=0.0 || coll.expectedGammaPeakInMeV_LAB<=0.0) {
      throw std::runtime_error("Wrong parameters of the expected gamma energy peak");
    }
  }
  
  // reset fitted parameters
  for(auto &pid : myOptions.tuned_pid_list) {
    initialPar.push_back(0.0); // [mm], offset
    isFixedPar.push_back(myOptions.use_scale_only);
    initialPar.push_back(1.0); // scale
    isFixedPar.push_back(false);
  }
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// dimensionless speed vector (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in BEAM coordinate system
TVector3 EnergyScale_analysis::getBetaVectorOfCMS_BEAM(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const {
  return getBetaOfCMS(nucleusMassInMeV, photonEnergyInMeV_LAB)*TVector3(0,0,1);
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame
double EnergyScale_analysis::getBetaOfCMS(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const {
  return photonEnergyInMeV_LAB/(photonEnergyInMeV_LAB+nucleusMassInMeV);
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int EnergyScale_analysis::getNparams() const { return myNparams; }
  
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// reconstruct 2-body decay event
double EnergyScale_analysis::getGammaEnergyInMeV_LAB(double nominalBeamEnergyInMeV_LAB,
						     definitions::reaction_type reaction,
						     TrackCollection &list) const {

  // determine parent nucleus
  auto parentPID=pid_type::UNKNOWN; // parent particle to be decayed
  auto leadingPID=pid_type::UNKNOWN; // leading particle from 2-body decay
  auto trailingPID=pid_type::UNKNOWN; // next-to-leading particle from 2-body decay
  const int ntracks = list.size(); // number of observed final tracks
  switch(ntracks) {
  case 2:
    if(list.front().pid==pid_type::ALPHA &&
	 list.back().pid==pid_type::CARBON_12 &&
       reaction==reaction_type::C12_ALPHA) {
      parentPID=pid_type::OXYGEN_16; // O-16 breakup
      leadingPID=list.front().pid;
      trailingPID=list.back().pid;
      break;
    }
    if(list.front().pid==pid_type::ALPHA &&
       list.back().pid==pid_type::CARBON_14 &&
       reaction==reaction_type::C14_ALPHA) {
      parentPID=pid_type::OXYGEN_18; // O-18 breakup
      leadingPID=list.front().pid;
      trailingPID=list.back().pid;
      break;
    }
  case 3:
    if(list.at(0).pid==pid_type::ALPHA &&
       list.at(1).pid==pid_type::ALPHA &&
       list.at(2).pid==pid_type::ALPHA &&
       reaction==reaction_type::THREE_ALPHA_BE) {
      parentPID=pid_type::CARBON_12; 
      leadingPID=list.front().pid;
      trailingPID=pid_type::BERYLLIUM_8; // C-12 breakup via intermediate Be-8 ground state
      break;
    }
  default:
    throw std::runtime_error("Unsupported combination of PIDs and reaction type");
  };

  // get momentum and energy of the leading particle from 2-body decay in BEAM/LAB frame
  auto leading_T_LAB = myRangeCalc->getIonEnergyMeV(leadingPID, list.front().length); // kinetic energy in LAB frame
  auto leading_mass = myRangeCalc->getIonMassMeV(leadingPID); // mass
  auto leading_p_LAB = sqrt(leading_T_LAB*(leading_T_LAB+2*leading_mass)); // scalar momentum
  TVector3 leadingP3_BEAM_LAB; // momentum in LAB frame in BEAM coordinate system
  leadingP3_BEAM_LAB.SetMagThetaPhi(leading_p_LAB, list.front().theta_BEAM, list.front().phi_BEAM);
  auto leadingP4_BEAM_LAB=TLorentzVector(leadingP3_BEAM_LAB, leading_mass+leading_T_LAB);

  // get momentum and energy of the next-to-leading particle from 2-body decay in BEAM/LAB frame
  auto trailing_mass = myRangeCalc->getIonMassMeV(trailingPID); // [MeV/c^2] - ground state mass
  if(myOptions.excitedRecoilFlag) trailing_mass += myOptions.excitedRecoilEnergyInMeV; // excited state mass (e.g. Be-8)
  // consider special case for Be-8 that almost instanly decays into 2 alphas
  double trailing_T_LAB=0;
  double trailing_p_LAB=0;
  TVector3 trailingP3_BEAM_LAB{0,0,0};
  if(trailingPID==BERYLLIUM_8) {
    for(auto &track: list) {
      static isFirst=true;
      if(isFirst) { isFirst=false; continue; } // skip the leading particle
      auto T_LAB = myRangeCalc->getIonEnergyMeV(track.pid, track.length); // [MeV] - kinetic energy in LAB
      auto mass = myRangeCalc->getIonMassMeV(track.pid); // [MeV/c^2] - ground state mass
      auto p_LAB = sqrt(T_LAB*(T_LAB+2*mass)); // [MeV/c] - scalar momentum in LAB
      TVector3 p3_BEAM_LAB{0,0,0};
      p3_BEAM_LAB.SetMagThetaPhi(p_LAB, track.theta_BEAM, track.phi_BEAM);
      trailingP3_BEAM_LAB += p3_BEAM_LAB;
      trailing_T_LAB += T_LAB;
    }
    trailing_p_LAB = trailingP3_BEAM_LAB.Mag();
  } else {
    trailing_T_LAB = myRangeCalc->getIonEnergyMeV(trailingPID, list.back().length); // [MeV] - kinetic energy in LAB
    trailing_p_LAB = sqrt(trailing_T_LAB*(trailing_T_LAB+2*trailing_mass)); // [MeV/c] - scalar momentum in LAB
    trailingP3_BEAM_LAB.SetMagThetaPhi(trailing_p_LAB, list.back().theta_BEAM, list.back().phi_BEAM);
  }
  auto trailingP4_BEAM_LAB=TLorentzVector(trailingP3_BEAM_LAB, trailing_mass+trailing_T_LAB);
    
  // boost P4 from BEAM/LAB frame to BEAM/CMS frame (see TLorentzVector::Boost() convention!)
  auto parentMassGroundState=myRangeCalc->getIonMassMeV(parentPID); // MeV/c^2, isotopic mass
  TVector3 beta_BEAM_LAB;
  if(myOptions.use_nominal_beam_energy) {
    beta_BEAM_LAB=getBetaVectorOfCMS_BEAM(parentMassGroundState, nominalBeamEnergyinMeV_LAB); // assume nominal direction and nominal gamma beam energy
  } else {
    auto photon_E_LAB=sumP4_BEAM_LAB.E()-parentMassGroundState; // reconstructed gamma beam energy in LAB
    beta_BEAM_LAB=getBetaVectorOfCMS_BEAM(parentMassGroundState, photon_E_LAB);
  }  
  auto leadingP4_BEAM_CMS(leadingP4_BEAM_LAB);
  auto trailingP4_BEAM_CMS(trailingP4_BEAM_LAB);
  leadingP4_BEAM_CMS.Boost(-1.0*beta_BEAM_LAB);
  trailingP4_BEAM_CMS.Boost(-1.0*beta_BEAM_LAB);
  
  // get momentum and energy in BEAM/CMS frame
  auto leading_p_CMS = leadingP4_BEAM_CMS.Vect().Mag(); // [MeV/c] - scalar momentum in CMS
  auto leading_T_CMS = leadingP4_BEAM_CMS.E()-leading_mass; // [MeV] - kinetic energy in CMS

  auto trailing_p_CMS = trailingP4_BEAM_CMS.Vect().Mag(); // [MeV/c] - scalar momentum in CMS
  auto trailing_T_CMS = trailingP4_BEAM_CMS.E()-trailing_mass; // [MeV] - kinetic energy in CMS

  // if requested, correct next-to-leading particle properties in CMS
  if(myOptions.use_leading_track_only) {
    trailing_p_CMS = leading_p_CMS; // assume momentum conservation
    trailing_T_CMS.SetVectM(-leadingP4_BEAM_CMS.Vect(), trailing_mass);
  }

  // calculate photon energy in CMS
  double total_E_CMS=(leadingP4_BEAM_CMS+trailingP4_BEAM_CMS).E(); // [MeV] - mass of parent's stationary excited state
  double excitation_E_CMS=total_E_CMS-parentMassGroundState; // [MeV] - parent's excitation energy above g.s. in CMS
  auto photon_E_CMS = excitation_E_CMS * (1.0 - 0.5*excitation_E_CMS/parentMassGroundState); // [MeV]

  // calculate photon energy in LAB
  auto photon_E_LAB = photon_E_CMS * sqrt(1.0 + 2.0*photon_E_CMS/parentMassGroundState); // [MeV]
  
  return photon_E_LAB;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// helper function to be called before each fit iteration
void EnergyScale_analysis::initializeCorrections(const int npar, const double *par) {

  // sanity check
  if (npar != myNparams || par==NULL) {
    throw std::runtime_error("Wrong input NPAR or PAR array");
  }

  // apply effective length corrections to ion range calculator
  int ipar=0;
  for(auto &pid : myOptions.tuned_pid_list) {
    ionRangeCalculator.setEffectiveLengthCorrection(pid, par[ipar+1], par[ipar]);
    ipar += 2;
  }
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// helper function to calculate global chi^2
void EnergyScale_analysis::getSelectionChi2(EventCollection &selection) const {

  double chi2 = 0.0;
  const auto factor = pow( selection.expectedGammaSigmaInMeV_LAB, 2.0) + pow( myOption.expectedResolutionInMeV, 2.0);
  for(auto &event: coll.events) {
    chi2 += pow( getGammaEnergyInMeV_LAB - selection.expectedGammaPeakInMeV_LAB, 2.0) / factor;
  }
  return chi2;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// global chi^2 from all event collections corresponding
// to different reaction channels
double EnergyScale_analysis::getChi2() const {

  double sumChi2=0.0;
  for(auto &selection: myCollection) {
    auto npoints = selection.events.size();
    sumChi2 += getSelectionChi2(selection) / npoints; // reduced chi^2, expectation value=1 per reaction channel
  }
  //  sumChi2 /= myCollection.size(); // reduced chi^2, expectation value = 1

#if(DEBUG_CHI2)
  std::cout<<__FUNCTION__<<": Sum of chi2 from "<<myCollection.size()<<" reaction channels = "<<sumChi2<<std::endl;
#endif

  //  return sumChi2;
  return sumChi2 * 0.5; // NOTE: for FUMILI minimizer the returned function should be: 0.5 * chi^2
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// actual implementation of the function to be minimized
double EnergyScale_analysis::operator() (const double *par) {

  initializeCorrections(myNparams, par);
  return getChi2();
}
