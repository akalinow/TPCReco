//////////////////////////
//
// root
// root [0] .L generateFakeRecoEvents.cxx
// root [1] generateFakeRecoEvents("geometry.dat")
//
//
//////////////////////////
//////////////////////////
//
// Below are parameters and flags that control behaviour of
// the toy Monte Carlo generator:
//
#define SIMUL_CONTAINMENT_FLAG   false     // skip partially contained events?
#define SIMUL_TRUNCATE_FLAG      true      // truncate partially contained tracks?
#define SIMUL_EXT_TRG_FLAG       true      // simulate external trigger?
#define SIMUL_EXT_TRG_ARRIVAL    0.1       // trigger postion on time scale [0-1]
//#define SIMUL_BEAM_E           7.164     // [MeV] energy peak, O-16 threshold
#define SIMUL_BEAM_E             11.5      // [MeV] energy peak
//#define SIMUL_BEAM_E           12.3      // [MeV] energy peak
//#define SIMUL_BEAM_E_RESOL     0         // no energy smearing
//#define SIMUL_BEAM_E_RESOL     0.00369   // [0-1] 0.369% energy sigma, fwhm=100 keV
#define SIMUL_BEAM_E_RESOL       0.00554   // [0-1] 0.554% energy sigma, fwhm=150 keV
//#define SIMUL_BEAM_E_RESOL     0.00738   // [0-1] 0.738% energy sigma, fwhm=200 keV
//#define SIMUL_BEAM_E_RESOL     0.0107    // [0-1] 1.107% energy sigma, fwhm=300 keV
//#define SIMUL_BEAM_E_RESOL     0.0148    // [0-1] 1.480% energy sigma, fwhm=400 keV
#define SIMUL_BEAM_SPREAD_R      5.25      // [mm] flat top intentsity radius
#define SIMUL_BEAM_SPREAD_SIGMA  1.0       // [mm] intensity tail sigma
#define SIMUL_PRESSURE           190.0     // [mbar] CO2 pressure
#define SIMUL_OXYGEN_E1E2_FLAG   true      // use anisotropic theta distribution for O-16?
#define SIMUL_OXYGEN_E1E2_SIGMA1 0         // sigma_E1 cross section [nb]
#define SIMUL_OXYGEN_E1E2_SIGMA2 1         // sigma_E2 cross section [nb]
#define SIMUL_OXYGEN_E1E2_PHI12  0         // E1/E2 mixing phase [rad]
//#define SIMUL_OXYGEN_E1E2_SIGMA1 0.16    // sigma_E1 cross section [nb]
//#define SIMUL_OXYGEN_E1E2_SIGMA2 0.15    // sigma_E2 cross section [nb]
//#define SIMUL_OXYGEN_E1E2_PHI12  54./180.*TMath::Pi() // E1/E2 mixing phase [rad]
#define SIMUL_POLARISATION_FLAG  false     // use unpolarised or partially polarized beam?
#define SIMUL_POLARISATION_FRAC  0.33      // degree of linear polarization of gamma beam
#define SIMUL_POLARISATION_ANGLE 60./180.*TMath::Pi()  // polarization plane (CKW wrt horizontal axis)
#define SIMUL_PLOT3D_FLAG        false     // create 3D debug plot with all tracks?

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <tuple>
#include <iostream>
#include <vector>
#include "TROOT.h"
#include "TSystem.h"
#include "TRandom3.h"
#include "TVector3.h"
#include "TH2Poly.h"
#include "TGraph.h"
#include "TLorentzVector.h"
#include "TFile.h"
#include "TTree.h"
#include "TF2.h"
#include "TMath.h"
#include "TCanvas.h"
#include "TView.h"
#include "TPolyLine3D.h"

#include "Math/IFunction.h" // needed for ROOT::Math::legendre
#include "MultiKey.h"
#include "GeometryTPC.h"
#include "IonRangeCalculator.h"
#include "Track3D.h"
#include "UtilsMath.h"

//////////////////////////
//////////////////////////
// 2D probability density function to be used by TF2::GetRandom2
// x[0] = x [mm]
// x[1] = y [mm]
// par[0] = R [mm]
// par[1] = flat-top intensity (arb.u.)
// par[2] = gaussian tail sigma [mm]
//
double beamProfile2D(double *x, double *par)
{
   double r = sqrt(x[0]*x[0]+x[1]*x[1]);
   if(r<par[0]) return par[1];
   return par[1]*exp(-0.5*(r-par[0])*(r-par[0])/par[2]/par[2]);
}
TF2 beamProfileTF2("beamProfileTF2", beamProfile2D, -100, 100, -100, 100, 3);
//
//////////////////////////
//////////////////////////
// 1D angular distribution to be used with TF1::GetRandom
// f=0 - circular polarized gamma beam
// f=1 - 100% linearly polarized gamma beam
//
// x[0] = phi_BEAM alpha-particle emission angle wrt horizontal axis, CKW [rad]
// par[0] = A - normalisation constant
// par[1] = f - degree of polaristation [0-1]
// par[2] = phi_0 - angle of polarization plane wrt horizontal axis, CKW [rad]
//
TF1 phiEmissionTF1("phiEmissionTF1", "[0]*(1+[1]*cos(2*(x+[2])))", 0.0, TMath::TwoPi());
//
//////////////////////////
//////////////////////////
// 1D angular distribution to be used with TF1::GetRandom
// Non-siotropic angular distrubution of alpha-particle emission angle wrt gamma beam
// with E1 abd E2 components for Oxygen-16 photodisintegration reaction.
// Ref: M.Assuncao et al., PRC 73 055801 (2006).
//
// x[0] = CMS theta_BEAM alpha-particle emission angle wrt gamma beam [rad]
// par[0] = normalisation constant
// par[1] = sigma_E1 - E1 cross section parameter [nb]
// par[2] = sigma_E2 - E2 cross section parameter [nb]
// par[3] = phase_12 - phase angle responsible for mixing E1/E2 [rad]
//
double thetaEmission(double *x, double *par)
{
  double c = cos(x[0]);
  double L[5];
  for(auto i=0; i<=4; i++) L[i]=ROOT::Math::legendre(i, c);
  double WE1 = L[0] - L[2];
  double WE2 = L[0] + (5./7.)*L[2] - (12./7.)*L[4];
  double W12 = 6./sqrt(5.0)*( L[1] - L[3] );
  return par[0]*( par[1]*WE1 + par[2]*WE2 + sqrt(par[1]*par[2])*cos(par[3])*W12 );
}
TF1 thetaEmissionTF1("thetaEmissionTF1", thetaEmission, 0.0, TMath::Pi(), 3);
//////////////////////////
//////////////////////////
std::shared_ptr<IonRangeCalculator> loadRangeCalculator(){

  std::shared_ptr<IonRangeCalculator> db = std::make_shared<IonRangeCalculator>();

  // set current conditions: gas=CO2, pressure=190 mbar, temperature=20C
  db->setGasConditions(IonRangeCalculator::CO2, SIMUL_PRESSURE, 273.15+20);
  
  return db;
}
/////////////////////////
/////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
//////////////////////////
//////////////////////////
bool isFullyContainedEvent(Track3D & aTrack){
  if(aTrack.getSegments().size()==0) return false;
  std::shared_ptr<GeometryTPC> aGeometry=aTrack.getSegments().front().getGeometry();
  for(auto & aSegment: aTrack.getSegments()) {
    if(!aGeometry->IsInsideActiveVolume(aSegment.getStart()) ||
       !aGeometry->IsInsideActiveVolume(aSegment.getEnd())) return false;
  }
  return true;
}
//////////////////////////
//////////////////////////
void truncateTracks(Track3D & aTrack, bool debug_flag){
  if(aTrack.getSegments().size()==0) return;
  std::shared_ptr<GeometryTPC> aGeometry=aTrack.getSegments().front().getGeometry();

  // construct list of active volume's faces
  // - first two represent (infinite) XY plane at different Z levels
  // - remaining ones represent (finite) vertical faces with orthogonal base vectors
  std::vector<TVector3> basepoint_vec, span1_vec, span2_vec;
  std::vector<bool> isHorizontal_vec;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=aGeometry->rangeXYZ();
  basepoint_vec.push_back(TVector3(xmin, ymin, zmin));
  basepoint_vec.push_back(TVector3(xmin, ymin, zmax));
  span1_vec.push_back(TVector3(1, 0, 0)*(xmax-xmin));
  span1_vec.push_back(span1_vec.back());
  span2_vec.push_back(TVector3(0, 1, 0)*(ymax-ymin));
  span2_vec.push_back(span2_vec.back());
  isHorizontal_vec.push_back(true);
  isHorizontal_vec.push_back(true);
  TGraph gr=aGeometry->GetActiveAreaConvexHull();
  for(auto iedge=0; iedge<gr.GetN()-1; iedge++) {
    basepoint_vec.push_back(TVector3(gr.GetX()[iedge], gr.GetY()[iedge], zmin));
    span1_vec.push_back(TVector3(gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmin)-basepoint_vec.back());
    span2_vec.push_back(TVector3(0, 0, 1)*(zmax-zmin));
    isHorizontal_vec.push_back(false);
  }
  const auto nwalls=basepoint_vec.size();

  std::set<unsigned int> eraseSet; // tracks marked for deletion
  for(auto index_seg=0u; index_seg<aTrack.getSegments().size(); index_seg++) {
    TrackSegment3D & aSegment=aTrack.getSegments().at(index_seg);
    auto offset=aSegment.getStart();
    auto tangent=aSegment.getEnd()-offset;
    const bool keep_start=aGeometry->IsInsideActiveVolume(offset);
    const bool keep_end=aGeometry->IsInsideActiveVolume(offset+tangent);

    // both inside => nothing to do
    if(keep_start && keep_end) {
      ///////// DEBUG
      //      std::cout << __FUNCTION__ << ": erasing track of index=" << index_seg << " from colllection of size=" << aTrack.getSegments().size() << std::endl << std::flush;
      //      eraseSet.insert(index_seg);  // just for DEBUG
      ///////// DEBUG
      continue;
    }
    auto isTruncated=false;
    // one inside, one outside => find one intersection point
    if(keep_start || keep_end) {
      TVector3 p;
      for(auto iwall=0u; iwall<nwalls; iwall++) { // loop through all detector's faces

	double t1=-9999, t2=-9999;
	if(!Utils::intersectionEdgePlane(offset, tangent, basepoint_vec[iwall],
					 span1_vec[iwall], span2_vec[iwall], p, t1, t2) ||
	   !( (isHorizontal_vec[iwall] && aGeometry->IsInsideActiveArea(TVector2(p.X(), p.Y()))) ||
	      (!isHorizontal_vec[iwall] && t1>=0 && t1<=1 && t2>=0 && t2<=1) ) ) {
	  continue; // next wall
	}
	///////// DEBUG
	//	bool res1=false, res2=false, res3=false, res4=false;
	//	res1=Utils::intersectionEdgePlane(offset, tangent,
	//					  basepoint_vec[iwall], span1_vec[iwall], span2_vec[iwall], p, t1, t2);
	//	res2=isHorizontal_vec[iwall];
	//	res3=(res1 && res2 && aGeometry->IsInsideActiveArea(TVector2(p.X(), p.Y())));
	//	res4=(res1 && !res2 && (t1>=0 && t1<=1 && t2>=0 && t2<=1));
	//	std::cout << __FUNCTION__ << ": detector face=" << iwall << ", found=" << res1 << ", inside_horizontal=" << res3 << ", inside_vertical=" << res4 << ", t1/t2=" << t1 << "/" << t2  << std::endl;
	//	if(!res1 || !(res3 || res4)) continue; // next wall
	///////// DEBUG

	if(keep_start) {
	  isTruncated=true;
	  aSegment.setStartEnd(offset, p); // truncate and keep start-point

	  ///////// DEBUG
	  if(debug_flag) std::cout<<__FUNCTION__<<": inside/outside, truncated new END=["
				  << aSegment.getEnd().X() << ", "
				  << aSegment.getEnd().Y() << ", "
				  << aSegment.getEnd().Z() << "]" << std::endl;
	  //	  eraseSet.insert(index_seg); // just for DEBUG
	  ///////// DEBUG
	} else {
	  isTruncated=true;
	  aSegment.setStartEnd(p, offset+tangent); // truncate and keep end-point

	  ///////// DEBUG
	  if(debug_flag) std::cout<<__FUNCTION__<<": inside/outside truncated new START=["
				  << aSegment.getStart().X() << ", "
				  << aSegment.getStart().Y() << ", "
				  << aSegment.getStart().Z() << "]" << std::endl;
	  //	  eraseSet.insert(index_seg); // just for DEBUG
	  ///////// DEBUG
	}
	break; // stop after finding first intersection point (works for convex volumes)
      }

      if(debug_flag && !isTruncated) {
	std::cerr << __FUNCTION__ << ": ERROR: inside/outside, no intersections found after checking "
		  << nwalls << " faces, "
		  << "START=[" << offset.X() << ", " << offset.Y() << ", " << offset.Z()
		  << "], END=[" << (offset+tangent).X() << ", " << (offset+tangent).Y() << ", " << (offset+tangent).Z() << "]" << std::endl;
	///////// DEBUG
	//    std::cout << __FUNCTION__ << ": erasing track of index=" << index_seg << " from colllection of size=" << aTrack.getSegments().size() << std::endl << std::flush;
	//	eraseSet.insert(index_seg); // just for DEBUG
	///////// DEBUG
      }
      continue; // next segment
    }

    ///////// DEBUG
    //    std::cout << __FUNCTION__ << ": erasing track of index=" << index_seg << " from colllection of size=" << aTrack.getSegments().size() << std::endl;
    //    eraseSet.insert(index_seg); // just for DEBUG
    //    continue;
    ///////// DEBUG

    // both outside => find 2 intersection points (if any)
    std::vector<TVector3> list;
    for(auto iwall=0u; iwall<nwalls; iwall++) { // loopt through all detector's faces

      // update track start/end points due to clipping to the previous detector face
      offset=aSegment.getStart();
      tangent=aSegment.getEnd()-offset;

      TVector3 p;
      double t1=9999, t2=9999;
      if(!Utils::intersectionEdgePlane(offset, tangent, basepoint_vec[iwall],
				       span1_vec[iwall], span2_vec[iwall], p, t1, t2) ||
	 !( (isHorizontal_vec[iwall] && aGeometry->IsInsideActiveArea(TVector2(p.X(), p.Y()))) ||
	    (!isHorizontal_vec[iwall] && t1>=0 && t1<=1 && t2>=0 && t2<=1) ) ) {
	continue; // next wall
      }
      list.push_back(p);
    }

    ///////// DEBUG
    if(debug_flag) std::cout << __FUNCTION__ << ": outside/outside, found " << list.size()
			     << " intersection(s) after checking "<< nwalls << " faces, START=["
			     << offset.X() << ", "
			     << offset.Y() << ", "
			     << offset.Z() << "], END=["
			     << (offset+tangent).X() << ", "
			     << (offset+tangent).Y() << ", "
			     << (offset+tangent).Z() << "]" << std::endl;
    ///////// DEBUG

    // reject track if there were not enough intersection points
    if(list.size()<2) {

      ///////// DEBUG
      if(debug_flag) std::cout << __FUNCTION__
			       << ": outside/outside, rejected, not enough intersections" << std::endl;
      ///////// DEBUG

      isTruncated=true;
      eraseSet.insert(index_seg);
      continue; // next segment
    }

    // sort list in ascending order according to distance from track's start-point
    std::sort(list.begin(), list.end(),
	      [&offset](const TVector3& a, const TVector3& b) {
		return (a-offset).Mag2() < (b-offset).Mag2();
	      });
    // pick the first and the last solution (works for convex volumes)
    isTruncated=true;
    aSegment.setStartEnd(list.front(), list.back());

    ///////// DEBUG
    if(debug_flag) std::cout << __FUNCTION__ << ": outside/outside, truncated new START=["
			     << aSegment.getStart().X() << ", "
			     << aSegment.getStart().Y() << ", "
			     << aSegment.getStart().Z() << "], new END=["
			     << aSegment.getEnd().X() << ", "
			     << aSegment.getEnd().Y() << ", "
			     << aSegment.getEnd().Z() << "]" << std::endl;
    ///////// DEBUG
  }

  // skip segments marked for deletion
  Track3D myTrack;
  for(auto index_seg=0u; index_seg<aTrack.getSegments().size(); index_seg++) {
    if(eraseSet.find(index_seg)==eraseSet.end())
      myTrack.addSegment(aTrack.getSegments().at(index_seg));
  }
  aTrack=myTrack;
  return;
}
//////////////////////////
//////////////////////////
Track3D generateFakeAlphaCarbonGenericEvent(std::shared_ptr<GeometryTPC> aGeometry, std::shared_ptr<IonRangeCalculator> aRangeCalculator, bool Oxygen18_flag=false, bool debug_flag=false){

  auto r=gRandom; // use global ROOT pointer

  Track3D empty_result;

  // define some constants
  const double atomicMassUnit = 931.49410242; // MeV/c^2
  const double alphaMass = 4*atomicMassUnit + 2.4249; // A*u + Delta MeV/c^2
  double carbonMass, oxygenMass;
  if(Oxygen18_flag) {
    carbonMass = 14.0032420*atomicMassUnit; // MeV/c^2, isotope C-14
    oxygenMass = 17.9991610*atomicMassUnit; // MeV/c^2, isotope O-18
  } else {
    carbonMass = 12*atomicMassUnit; // MeV/c^2, isotope C-12
    oxygenMass = 15.99491461956*atomicMassUnit; // MeV/c^2, isotope O-16
  }

  // detector reference frame (DET=LAB)
  double beamEnergyResolution=SIMUL_BEAM_E_RESOL; // LAB beam energy smearing factor
  double beamEnergy_DET=SIMUL_BEAM_E*r->Gaus(1, beamEnergyResolution); // smear beam energy by 5% // MeV
  TVector3 beamDir_DET(-1, 0, 0); // unit vector
  TLorentzVector photonP4_DET(beamDir_DET.Unit()*beamEnergy_DET, beamEnergy_DET); // [MeV/c, MeV/c, MeV/c, MeV/c^2]
  TLorentzVector oxygenP4_DET(0, 0, 0, oxygenMass); // [MeV/c, MeV/c, MeV/c, MeV/c^2]

  // speed of photon-oxygen CMS frame in DET frame
  double beta=beamEnergy_DET/(beamEnergy_DET+oxygenMass);
  TVector3 beta_DET=beamDir_DET.Unit()*beta;

  // boosting P4 from DET/LAB frame to CMS frame (see TlorentzVector::Boost() convention!)
  TLorentzVector photonP4_CMS(photonP4_DET);
  TLorentzVector oxygenP4_CMS(oxygenP4_DET);
  photonP4_CMS.Boost(-beta_DET);
  oxygenP4_CMS.Boost(-beta_DET);

  // check total energy in CMS frame
  double totalEnergy_CMS=(photonP4_CMS+oxygenP4_CMS).E();
  double totalEnergy_CMS_xcheck=(oxygenMass+beamEnergy_DET*(1-beta))/sqrt(1-beta*beta); // DEBUG
  double totalEnergy_CMS_xcheck2=sqrt( oxygenMass*(2*beamEnergy_DET+oxygenMass) ); // DEBUG

  // check beam energy in CMS frame
  double beamEnergy_CMS=photonP4_CMS.E();
  double beamEnergy_CMS_xcheck=beamEnergy_DET*sqrt(oxygenMass/(oxygenMass+2*beamEnergy_DET));

  // stationary excited oxygen state
  double oxygenMassExcited=totalEnergy_CMS;
  double oxygenMassExcited_xcheck=beamEnergy_CMS_xcheck+sqrt(oxygenMass*oxygenMass+beamEnergy_CMS*beamEnergy_CMS); //
  double Qvalue_CMS=oxygenMassExcited-alphaMass-carbonMass;
  if(Qvalue_CMS<0) {
    if(debug_flag) std::cout<<"Beam energy is too low to create C12+alpha pair!"<<std::endl;
    return empty_result;
  }

  // calculate momenta of particles in CMS frame
  double p_alpha_CMS=0.5*sqrt((pow(pow(totalEnergy_CMS, 2)
			       -pow(alphaMass, 2)-pow(carbonMass, 2), 2)
			   -4*pow(alphaMass*carbonMass, 2)))/totalEnergy_CMS;
  double T_alpha_CMS=sqrt(p_alpha_CMS*p_alpha_CMS+alphaMass*alphaMass)-alphaMass;
  double p_carbon_CMS=p_alpha_CMS;
  double T_carbon_CMS=sqrt(p_carbon_CMS*p_carbon_CMS+carbonMass*carbonMass)-carbonMass;

  double phi_BEAM, theta_BEAM;
  // pick polar alpha-particle emission angle according to specific model
  if(!Oxygen18_flag && SIMUL_OXYGEN_E1E2_FLAG) {
    // distribute anisotropically, mixture of E1 and E2 components for O-16
    theta_BEAM=thetaEmissionTF1.GetRandom();
  } else {
    // distribute isotropically alpha particles in CMS frame
    theta_BEAM=acos(r->Uniform(-1, 1)); // rad, dOmega=dPhi*d(cosTheta)
  }
  // pick azimuthal alpha-particle emission angle according to specific model
  if(SIMUL_POLARISATION_FLAG) {
    // distribute anisotropically, mix of circular and linear polarisation
    phi_BEAM=phiEmissionTF1.GetRandom();
  } else {
    // distribute isotropically alpha particles in CMS frame
    phi_BEAM=r->Uniform(0, TMath::TwoPi()); // rad
  }
  TLorentzVector alphaP4_CMS(p_alpha_CMS*beamDir_DET.Unit(), alphaMass+T_alpha_CMS); // initial = along gamma beam
  alphaP4_CMS.RotateZ(theta_BEAM); // rotate by theta_BEAM about detector Z axis (vertical)
  alphaP4_CMS.Rotate(phi_BEAM, beamDir_DET); // rotate by phi_BEAM about beam axis

  //  TLorentzVector alphaP4_CMS(p_alpha_CMS*TVector3(0, 0, 1), alphaMass+T_alpha_CMS); // initial = along Z
  //  alphaP4_CMS.RotateY(theta); // rotate by theta about Y axis
  //  alphaP4_CMS.RotateZ(phi); // rotate by phi about Z axis

  // calculate momentum of carbon recoil (back-to-back) in CMs frame
  TLorentzVector carbonP4_CMS;
  carbonP4_CMS.SetVectM( -1.0*alphaP4_CMS.Vect(), carbonMass);

  // boosting P4 from CMS frame to DET/LAB frame (see TlorentzVector::Boost() convention!)
  TLorentzVector alphaP4_DET(alphaP4_CMS);
  TLorentzVector carbonP4_DET(carbonP4_CMS);
  alphaP4_DET.Boost(beta_DET);
  carbonP4_DET.Boost(beta_DET);

  // print debug info
  //  bool debug_flag=true;
  if(debug_flag) {
    std::cout<<"Alpha kin.energy in CMS (2 methods): "<<T_alpha_CMS<<", "<<alphaP4_CMS.E()-alphaP4_CMS.M()<<std::endl;
    std::cout<<"Alpha momentum in CMS (2 methods): "<<p_alpha_CMS<<", "<<alphaP4_CMS.P()<<std::endl;
    std::cout<<"Carbon kin.energy in CMS (2 methods): "<<T_carbon_CMS<<", "<<carbonP4_CMS.E()-carbonP4_CMS.M()<<std::endl;
    std::cout<<"Carbon momentum in CMS (2 methods): "<<p_carbon_CMS<<", "<<carbonP4_CMS.P()<<std::endl;

    std::cout<<"T_alpha+T_carbon in CMS (2 methods): "<<T_alpha_CMS+T_carbon_CMS<<", "<<alphaP4_CMS.E()+carbonP4_CMS.E()-alphaP4_CMS.M()-carbonP4_CMS.M()<<std::endl;
    std::cout<<"p_alpha+p_carbon vector momentum balance in CMS:"<<std::endl;
    (alphaP4_CMS.Vect()+carbonP4_CMS.Vect()).Print();

    std::cout<<"|p_alpha+p_carbon| scalar momentum balance in CMS:"<<(alphaP4_CMS+carbonP4_CMS).P()<<std::endl;
    double invariantMass1 = (photonP4_CMS+oxygenP4_CMS).M();
    double invariantMass2 = (alphaP4_CMS+carbonP4_CMS).M();
    std::cout<<"Gamma beam energy in DET/LAB: "<<beamEnergy_DET<<std::endl;
    std::cout<<"Gamma beam energy in CMS (2 methods): "<<beamEnergy_CMS<<", "<<beamEnergy_CMS_xcheck<<std::endl;
    std::cout<<"Total energy in CMS (3 methods): "<<totalEnergy_CMS<<" "<<totalEnergy_CMS_xcheck<<" "<<totalEnergy_CMS_xcheck2<<std::endl; // DEBUG
    std::cout<<"Q-value in CMS: "<<Qvalue_CMS<<std::endl;
    std::cout<<"Photon+Oxygen system invariant mass = "<<invariantMass1<<std::endl;
    std::cout<<"Alpha+Carbon system invariant mass = "<<invariantMass2<<std::endl;
    std::cout<<"Oxygen ground state mass = "<<oxygenMass<<std::endl;
    std::cout<<"Oxygen excited state mass (2 methods)= "<<oxygenMassExcited<<", "<<oxygenMassExcited_xcheck<<std::endl;
    std::cout<<"Alpha kin.energy in DET/LAB: "<<alphaP4_DET.E()-alphaP4_DET.M()<<std::endl;
    std::cout<<"Alpha mass in DET/LAB: "<<alphaP4_DET.M()<<std::endl;
    std::cout<<"Carbon kin.energy in DET/LAB: "<<carbonP4_DET.E()-carbonP4_DET.M()<<std::endl;
    std::cout<<"Carbon mass in DET/LAB: "<<carbonP4_DET.M()<<std::endl;
  }

  // calculate alpha/carbon ranges [mm] in LAB/DET frame
  double rangeAlpha_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::ALPHA, alphaP4_DET.E()-alphaP4_DET.M()); // mm
  double rangeCarbon_DET;
  if(Oxygen18_flag) { // Alpha + Carbon-14
    rangeCarbon_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::CARBON_14, carbonP4_DET.E()-carbonP4_DET.M()); // mm
  } else { // Alpha + Carbon-12
    rangeCarbon_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::CARBON_12, carbonP4_DET.E()-carbonP4_DET.M()); // mm
  }

  // create list of tracks
  std::vector<TVector3> list;
  list.push_back(alphaP4_DET.Vect().Unit()*rangeAlpha_DET); // 1st track direction
  list.push_back(carbonP4_DET.Vect().Unit()*rangeCarbon_DET); // 2nd track direction

  // smear vertex position in XYZ
  // - assume gaussian spread of beam profile
  // - assume self-triggering mode
  // - assume trigger position at 10% of TIME scale
  double xmin, xmax, ymin, ymax, zmin, zmax;
  std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=aGeometry->rangeXYZ();

  // gaussian beam profile in YZ plane
  //  double beamSpreadSigma=1.0;   // gaussian tail sigma in [mm]
  //  TVector3 vertex( r->Uniform(xmin, xmax), r->Gaus(0, beamSpreadSigma), r->Gaus(0.5*(zmin+zmax), beamSpreadsigma) );

  // flat-top with gaussian tails beam profile in YZ plane
  // employs TF2::GetRandom2()
  double y_rnd=0, z_rnd=0;
  beamProfileTF2.GetRandom2(y_rnd, z_rnd); // newer ROOT: (y_rnd, z_rnd, r);

  //  TVector3 vertex( r->Uniform(xmin, xmax), y_rnd, z_rnd);
  ///////// DEBUG
  TVector3 vertex( r->Uniform(xmin-10, xmax+10), y_rnd, z_rnd); // extend by +/- 10 mm
  ///////// DEBUG

  // create TrackSegment3D collection
  Track3D aTrack;
  TrackSegment3D aSegment;
  for(auto & seg: list) {
    aSegment.setGeometry(aGeometry);
    aSegment.setStartEnd(vertex, vertex+seg);
    aTrack.addSegment(aSegment);
  }
  return aTrack;
}
//////////////////////////
//////////////////////////
Track3D generateFakeAlphaCarbon12Event(std::shared_ptr<GeometryTPC> aGeometry, std::shared_ptr<IonRangeCalculator> aRangeCalculator, bool debug_flag=false){
  return generateFakeAlphaCarbonGenericEvent(aGeometry, aRangeCalculator, false, debug_flag);
}
//////////////////////////
//////////////////////////
Track3D generateFakeAlphaCarbon14Event(std::shared_ptr<GeometryTPC> aGeometry, std::shared_ptr<IonRangeCalculator> aRangeCalculator, bool debug_flag=false){
  return generateFakeAlphaCarbonGenericEvent(aGeometry, aRangeCalculator, true, debug_flag);
}
//////////////////////////
//////////////////////////
Track3D generateFake3AlphaEvent(std::shared_ptr<GeometryTPC> aGeometry, std::shared_ptr<IonRangeCalculator> aRangeCalculator, bool debug_flag=false){
  //Track3D generateFake3AlphaEvent(std::shared_ptr<GeometryTPC> aGeometry, IonRangeCalculator *aRangeCalculator, bool debug_flag=false){

  //  auto r=new Trandom3();
  auto r=gRandom; // use global ROOT pointer

  Track3D empty_result;

  // define some constants
  const double atomicMassUnit = 931.49410242; //MeV/c^2
  const double alphaMass = 4*atomicMassUnit + 2.4249; //A*u + Delta MeV/c^2
  const double carbonMass = 12*atomicMassUnit;

  // detector reference frame (DET=LAB)
  double beamEnergyResolution=SIMUL_BEAM_E_RESOL; // LAB beam energy smearing factor
  double beamEnergy_DET=SIMUL_BEAM_E*r->Gaus(1, beamEnergyResolution); // smear beam energy by 5% // MeV
  TVector3 beamDir_DET(-1, 0, 0); // unit vector
  TLorentzVector photonP4_DET(beamDir_DET.Unit()*beamEnergy_DET, beamEnergy_DET); // [MeV/c, MeV/c, MeV/c, MeV/c^2]
  TLorentzVector carbonP4_DET(0, 0, 0, carbonMass); // [MeV/c, MeV/c, MeV/c, MeV/c^2]

  // speed of photon-carbon CMS frame in DET frame
  double beta=beamEnergy_DET/(beamEnergy_DET+carbonMass);
  TVector3 beta_DET=beamDir_DET.Unit()*beta;

  // boosting P4 from DET/LAB frame to CMS frame (see TlorentzVector::Boost() convention!)
  TLorentzVector photonP4_CMS(photonP4_DET); 
  TLorentzVector carbonP4_CMS(carbonP4_DET); 
  photonP4_CMS.Boost(-beta_DET);
  carbonP4_CMS.Boost(-beta_DET);

  // check total energy in CMS frame
  double totalEnergy_CMS=(photonP4_CMS+carbonP4_CMS).E();
  double totalEnergy_CMS_xcheck=(carbonMass+beamEnergy_DET*(1-beta))/sqrt(1-beta*beta); // DEBUG
  double totalEnergy_CMS_xcheck2=sqrt( carbonMass*(2*beamEnergy_DET+carbonMass) ); // DEBUG

  // check beam energy in CMS frame
  double beamEnergy_CMS=photonP4_CMS.E();
  double beamEnergy_CMS_xcheck=beamEnergy_DET*sqrt(carbonMass/(carbonMass+2*beamEnergy_DET));

  // stationary excited carbon state
  double carbonMassExcited=totalEnergy_CMS;
  double carbonMassExcited_xcheck=beamEnergy_CMS_xcheck+sqrt(carbonMass*carbonMass+beamEnergy_CMS*beamEnergy_CMS); //
  double Qvalue_CMS=carbonMassExcited-3*alphaMass;
  if(Qvalue_CMS<0) {
    if(debug_flag) std::cout<<"Beam energy is too low to create 3 alpha particles!"<<std::endl;
    return empty_result;
  }
    
  // assign randomly kinetic energies for 3 alpha particles:
  //
  // - matrix element is constant
  // - Q = T1+T2+T3 = height of a regular triangle (Dalitz plot for same-particle 3-body decay)
  // - inscribed circle of radius R=Q/3 is uniformely populated (non-relativistic case, Ti<<M)
  // - T3=R+r0*sin(f0), 0<=r0<=R, 0<=f0<2*pi
  // - T2=R+r0*sin(f0+2*pi/3)
  // - T1=R+r0*sin(f0+4*pi/3)=Q-T2-T3
  //
  double phi0=r->Uniform(0, TMath::TwoPi());
  double r0=sqrt(r->Uniform(0, pow(Qvalue_CMS/3, 2))); // dS=dPhi*d(r^2)/2
  double T1_CMS=Qvalue_CMS/3+r0*sin(phi0);
  double T2_CMS=Qvalue_CMS/3+r0*sin(phi0+TMath::TwoPi()/3);
  double T3_CMS=Qvalue_CMS-T1_CMS-T2_CMS;
  double T3_CMS_xcheck=Qvalue_CMS/3+r0*sin(phi0+2*TMath::TwoPi()/3);
  double p1_CMS=sqrt(T1_CMS*(T1_CMS+2*alphaMass)); // sqrt(2*T1_CMS*alphaMass); // MeV
  double p2_CMS=sqrt(T2_CMS*(T2_CMS+2*alphaMass)); // sqrt(2*T2_CMS*alphaMass); // MeV
  double p3_CMS=sqrt(T3_CMS*(T3_CMS+2*alphaMass)); // sqrt(2*T3_CMS*alphaMass); // MeV

  // distribute isotropically 1st alpha particle momentum:
  double phi1=r->Uniform(0, TMath::TwoPi()); // rad
  double theta1=acos(r->Uniform(-1, 1)); // rad, dOmega=dPhi*d(cosTheta)
  TLorentzVector alpha1_P4_CMS(p1_CMS*TVector3(0, 0, 1), alphaMass+T1_CMS); // initial = along Z
  alpha1_P4_CMS.RotateY(theta1); // rotate by theta1 about Y axis
  alpha1_P4_CMS.RotateZ(phi1); // rotate by phi1 about Z axis

  // distribute isotropically 2nd alpha particle momentum
  // - cos(theta12) is constrained by momentum conservation: p3^2 = p1^2 + p2^2 + 2*p1*p2*cos(theta12)
  double phi12=r->Uniform(0, TMath::TwoPi()); // rad
  double theta12=acos( 0.5*(p3_CMS*p3_CMS - p1_CMS*p1_CMS - p2_CMS*p2_CMS)/p1_CMS/p2_CMS ); // rad
  TLorentzVector alpha2_P4_CMS(p2_CMS*alpha1_P4_CMS.Vect().Unit(), alphaMass+T2_CMS); // initial = along ALPHA1
  alpha2_P4_CMS.Rotate(theta12, alpha1_P4_CMS.Vect().Orthogonal()); // rotate by theta12 about axis orthogonal to ALPHA1
  alpha2_P4_CMS.Rotate(phi12, alpha1_P4_CMS.Vect()); // rotate by phi12 about ALPHA1 axis

  // calculate 3rd alpha particle momentum: P3=-(P1+P2)
  //  TLorentzVector alpha3_P4_CMS=-alpha1_P4_CMS-alpha2_P4_CMS; // sets correct momentum but not energy
  //  alpha3_P4_CMS.SetE(alphaMass+T3_CMS); // sets correct energy
  TLorentzVector alpha3_P4_CMS;
  alpha3_P4_CMS.SetVectM( -(alpha1_P4_CMS.Vect()+alpha2_P4_CMS.Vect()), alphaMass);

  // boosting P4 from CMS frame to DET/LAB frame (see TlorentzVector::Boost() convention!)
  TLorentzVector alpha1_P4_DET(alpha1_P4_CMS); 
  TLorentzVector alpha2_P4_DET(alpha2_P4_CMS); 
  TLorentzVector alpha3_P4_DET(alpha3_P4_CMS); 
  alpha1_P4_DET.Boost(beta_DET);
  alpha2_P4_DET.Boost(beta_DET);
  alpha3_P4_DET.Boost(beta_DET);

  // print debug info
  //  bool debug_flag=true;
  if(debug_flag) {
    TLorentzVector alpha3_P4_CMS_xcheck( -(alpha1_P4_CMS+alpha2_P4_CMS).Vect(), alphaMass+T3_CMS);
    TLorentzVector alpha3_P4_CMS_xcheck2( -(alpha1_P4_CMS.Vect()+alpha2_P4_CMS.Vect()).Unit()*p3_CMS, alphaMass+T3_CMS);
    std::cout<<"Alpha[1] kin.energy in CMS (2 methods): "<<T1_CMS<<", "<<alpha1_P4_CMS.E()-alpha1_P4_CMS.M()<<std::endl;
    std::cout<<"Alpha[1] momentum in CMS (2 methods): "<<p1_CMS<<", "<<alpha1_P4_CMS.P()<<std::endl;
    std::cout<<"Alpha[2] kin.energy in CMS (2 methods): "<<T2_CMS<<", "<<alpha2_P4_CMS.E()-alpha2_P4_CMS.M()<<std::endl;
    std::cout<<"Alpha[2] momentum in CMS (2 methods): "<<p2_CMS<<", "<<alpha2_P4_CMS.P()<<std::endl;
    std::cout<<"Alpha[3] kin.energy in CMS (3 methods): "<<T3_CMS<<", "<<alpha3_P4_CMS.E()-alpha3_P4_CMS.M()<<", "<<T3_CMS_xcheck<<std::endl;
    std::cout<<"Alpha[3] momentum in CMS (3 methods): "<<p3_CMS<<", "<<alpha3_P4_CMS.P()<<", "<<alpha3_P4_CMS_xcheck.P()<<std::endl;
    std::cout<<"Alpha[3] P4 in CMS (3 methods): "<<std::endl;
    alpha3_P4_CMS_xcheck2.Print();
    alpha3_P4_CMS.Print();
    alpha3_P4_CMS_xcheck.Print();
    std::cout<<"Alpha T1+T2+T3 in CMS (2 methods): "<<T1_CMS+T2_CMS+T3_CMS<<", "<<alpha1_P4_CMS.E()+alpha2_P4_CMS.E()+alpha3_P4_CMS.E()-alpha1_P4_CMS.M()-alpha2_P4_CMS.M()-alpha3_P4_CMS.M()<<std::endl;
    std::cout<<"Alpha p1+p2+p3 momentum in CMS (2 methods):"<<std::endl;
    (alpha1_P4_CMS.Vect()+alpha2_P4_CMS.Vect()+alpha3_P4_CMS_xcheck2.Vect()).Print();
    (alpha1_P4_CMS.Vect()+alpha2_P4_CMS.Vect()+alpha3_P4_CMS.Vect()).Print();
    (alpha1_P4_CMS.Vect()+alpha2_P4_CMS.Vect()+alpha3_P4_CMS_xcheck.Vect()).Print();
    std::cout<<"Alpha |p1+p2| momentum in CMS (2 methods):"<<p3_CMS<<", "<<(alpha1_P4_CMS+alpha2_P4_CMS).P()<<std::endl;
    double invariantMass1 = (photonP4_CMS+carbonP4_CMS).M();
    double invariantMass2 = (alpha1_P4_CMS+alpha2_P4_CMS+alpha3_P4_CMS).M();
    std::cout<<"Gamma beam energy in DET/LAB: "<<beamEnergy_DET<<std::endl;
    std::cout<<"Gamma beam energy in CMS (2 methods): "<<beamEnergy_CMS<<", "<<beamEnergy_CMS_xcheck<<std::endl;
    std::cout<<"Total energy in CMS (3 methods): "<<totalEnergy_CMS<<" "<<totalEnergy_CMS_xcheck<<" "<<totalEnergy_CMS_xcheck2<<std::endl; // DEBUG
    std::cout<<"Q-value in CMS: "<<Qvalue_CMS<<std::endl;
    std::cout<<"Alpha[i] kin.energy in CMS: "<<T1_CMS<<", "<<T2_CMS<<", "<<T3_CMS<<std::endl;
    std::cout<<"Alpha[i] mass in CMS: "<<alpha1_P4_CMS.M()<<" "<<alpha2_P4_CMS.M()<<" "<<alpha3_P4_CMS.M()<<std::endl; // DEBUG
    std::cout<<"Photon+Carbon system invariant mass = "<<invariantMass1<<std::endl;
    std::cout<<"Triple-alpha system invariant mass = "<<invariantMass2<<std::endl;
    std::cout<<"Carbon ground state mass = "<<carbonMass<<std::endl;
    std::cout<<"Carbon excited state mass (2 methods)= "<<carbonMassExcited<<", "<<carbonMassExcited_xcheck<<std::endl;
    std::cout<<"Alpha kin.energy in DET/LAB: "<<alpha1_P4_DET.E()-alpha1_P4_DET.M()<<", "<<alpha2_P4_DET.E()-alpha2_P4_DET.M()<<", "<<alpha3_P4_DET.E()-alpha3_P4_DET.M()<<std::endl;
    std::cout<<"Alpha mass in DET/LAB: "<<alpha1_P4_DET.M()<<" "<<alpha2_P4_DET.M()<<" "<<alpha3_P4_DET.M()<<std::endl; // DEBUG
  }

  // calculate alpha ranges [mm] in LAB/DET frame
  double range1_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::ALPHA, alpha1_P4_DET.E()-alpha1_P4_DET.M()); // mm
  double range2_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::ALPHA, alpha2_P4_DET.E()-alpha2_P4_DET.M()); // mm
  double range3_DET=aRangeCalculator->getIonRangeMM(IonRangeCalculator::ALPHA, alpha3_P4_DET.E()-alpha3_P4_DET.M()); // mm

  // create list of tracks
  std::vector<TVector3> list;
  list.push_back(alpha1_P4_DET.Vect().Unit()*range1_DET); // 1st track direction
  list.push_back(alpha2_P4_DET.Vect().Unit()*range2_DET); // 2nd track direction
  list.push_back(alpha3_P4_DET.Vect().Unit()*range3_DET); // 3rd track direction

  // smear vertex position in XYZ
  // - assume gaussian spread of beam profile
  // - assume self-triggering mode
  // - assume trigger position at 10% of TIME scale
  double xmin, xmax, ymin, ymax, zmin, zmax;
  std::tie(xmin, xmax, ymin, ymax)=aGeometry->rangeXY();
  zmin=aGeometry->GetDriftCageZmin();
  zmax=aGeometry->GetDriftCageZmax();

  // gaussian beam profile in YZ plane
  //  double beamSpreadSigma=5.0;   // gaussian tail sigma in [mm]
  //  TVector3 vertex( r->Uniform(xmin, xmax), r->Gaus(0, beamSpreadSigma), r->Gaus(0.5*(zmin+zmax), beamSpreadsigma) );

  // flat-top with gaussian tails beam profile in YZ plane
  // employs TF2::GetRandom2()
  double y_rnd=0, z_rnd=0;
  beamProfileTF2.GetRandom2(y_rnd, z_rnd); // (y_rnd, z_rnf, r);
  TVector3 vertex( r->Uniform(xmin, xmax), y_rnd, z_rnd);

  // create TrackSegment3D collection
  Track3D aTrack;
  TrackSegment3D aSegment;
  for(auto & seg: list) {
    aSegment.setGeometry(aGeometry);
    aSegment.setStartEnd(vertex, vertex+seg);
    aTrack.addSegment(aSegment);
  }
  return aTrack;
}
//////////////////////////
//////////////////////////
void generateFakeRecoEvents(const std::string geometryName, unsigned int nEvents, double brOxygen16=1, double brOxygen18=0, double brCarbon12_3alpha=0, bool debug_flag=false){

  if (!gROOT->GetClass("Track3D")){
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("IonRangeCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  R__LOAD_LIBRARY(libUtilities.so);
  R__LOAD_LIBRARY(libMathMore.so);
  
  // construct partial sum of BRs, normalized to 1
  std::vector<double> brSum;
  brSum.push_back(fabs(brOxygen16));
  brSum.push_back(fabs(brOxygen18));
  brSum.push_back(fabs(brCarbon12_3alpha));
  for(auto i=1u; i<brSum.size(); i++) {
    brSum[i] += brSum[i-1];
  }
  if(brSum.back()==0) {
    std::cerr<<"ERROR: Wrong branching ratios!"<<std::endl;
    return;
  }
  // normalize sum to 1
  for(auto i=0u; i<brSum.size(); i++) {
    brSum[i] /= brSum.back();
  }

  std::cout << __FUNCTION__ << ": Reaction branching ratios" << std::endl;
  std::cout << __FUNCTION__ << ": BR(O-16 -> alpha + C-12) = " << brSum[0]*1e2 << " %" << std::endl;
  std::cout << __FUNCTION__ << ": BR(O-18 -> alpha + C-14) = " << (brSum[1]-brSum[0])*1e2 << " %" << std::endl;
  std::cout << __FUNCTION__ << ": BR(C-12 -> 3-alpha)      = " << (brSum[2]-brSum[1])*1e2 << " %" << std::endl;

  const std::string fileName="Reco_FakeEvents.root";
  auto myGeometry=loadGeometry(geometryName);
  if(!myGeometry->IsOK()) {
    std::cerr<<"ERROR: Wrong geometry file!"<<std::endl;
    return;
  }
  auto myRangeCalculator=loadRangeCalculator();
  if(!myRangeCalculator->IsOK()) {
    std::cerr<<"ERROR: Cannot initialize range/energy calculator!"<<std::endl;
    return;
  }
  
  //  TFile *aFile = new TFile(fileName.c_str(), "CREATE");
  TFile *aFile = new TFile(fileName.c_str(), "RECREATE");
  if(!aFile || !aFile->IsOpen()) {
    std::cerr<<"ERROR: Cannot create output file!"<<std::endl;
    return;
  }

  // auxiliary canvas with all generated tracks (optional)
  TCanvas *outputCanvas=0;
  if(SIMUL_PLOT3D_FLAG) {
    outputCanvas = new TCanvas("c", "all events", 500, 500);
    outputCanvas->cd();
    TView *view=TView::CreateView(1);
    double xmin, xmax, ymin, ymax, zmin, zmax;
    std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=myGeometry->rangeXYZ();
    auto view_span=0.8*std::max(std::max(xmax-xmin, ymax-ymin), zmax-zmin);
    view->SetRange(0.5*(xmax+xmin)-0.5*view_span, 0.5*(ymax+ymin)-0.5*view_span, 0.5*(zmax+zmin)-0.5*view_span,
		   0.5*(xmax+xmin)+0.5*view_span, 0.5*(ymax+ymin)+0.5*view_span, 0.5*(zmax+zmin)+0.5*view_span);
    // plot active volume's faces
    TGraph gr=myGeometry->GetActiveAreaConvexHull();
    TPolyLine3D l(5*(gr.GetN()-1));
    for(auto iedge=0; iedge<gr.GetN()-1; iedge++) {
      l.SetPoint(iedge*5+0, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
      l.SetPoint(iedge*5+1, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmin);
      l.SetPoint(iedge*5+2, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmax);
      l.SetPoint(iedge*5+3, gr.GetX()[iedge], gr.GetY()[iedge], zmax);
      l.SetPoint(iedge*5+4, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
    }
    l.SetLineColor(kBlue);
    l.DrawClone();
    outputCanvas->Update();
    outputCanvas->Modified();
  }

  aFile->cd();
  TTree *aTree = new TTree("TPCRecoData","");
  Track3D *aTrack = new Track3D();
  aTree->Branch("RecoEvent", "Track3D", &aTrack);

  //  auto r=new Trandom3(0);
  auto r=gRandom; // use global ROOT pointer
  r->SetSeed(0);

  // initialize beam profile parameters
  beamProfileTF2.SetParameters(SIMUL_BEAM_SPREAD_R, 1, SIMUL_BEAM_SPREAD_SIGMA);
  beamProfileTF2.SetNpx(500);
  beamProfileTF2.SetNpy(500);

  // initialize THETA anistoropic distribution in CMS frame
  if(SIMUL_OXYGEN_E1E2_FLAG) {
    thetaEmissionTF1.SetParameters(1.0, SIMUL_OXYGEN_E1E2_SIGMA1, SIMUL_OXYGEN_E1E2_SIGMA2, SIMUL_OXYGEN_E1E2_PHI12);
    thetaEmissionTF1.SetNpx(10000);
  }

  // initialize PHI anistoropic distribution in CMS frame
  if(SIMUL_POLARISATION_FLAG) {
    phiEmissionTF1.SetParameters(1.0, SIMUL_POLARISATION_FRAC, SIMUL_POLARISATION_ANGLE);
    phiEmissionTF1.SetNpx(10000);
  }

  Track3D (*func)(std::shared_ptr<GeometryTPC>, std::shared_ptr<IonRangeCalculator>, bool);

  for(auto i=0L; i<nEvents; i++) {

    int color=kBlack; // track color on 3D plot (optional)
    auto br = r->Uniform(brSum.back());
    if(br<brSum[0]) { // Oxygen-16 to alpha + Carbon-12
      func=generateFakeAlphaCarbon12Event;
      color=kRed-3;
      if(debug_flag) std::cout<<"a+C12"<<std::endl;
    } else if(br<brSum[1]) { // Oxygen-18 to alpha + Carbon-14
      func=generateFakeAlphaCarbon14Event;
      color=kGray+2; //Orange+10; //kMagenta-4;
      if(debug_flag) std::cout<<"a+C14"<<std::endl;
    } else { // Carbon-12 to three alphas
      func=generateFake3AlphaEvent;
      color=kGreen+3;
      if(debug_flag) std::cout<<"3a"<<std::endl;
    }

    if(debug_flag || (i<1000L && (i%100L)==0L) || (i%1000L)==0L ) std::cout<<"Generating fake track i="<<i<<std::endl;
    *aTrack = func(myGeometry, myRangeCalculator, debug_flag);

    // check if all segments are fully contained
    if( (SIMUL_CONTAINMENT_FLAG || SIMUL_TRUNCATE_FLAG) &&
	!isFullyContainedEvent(*aTrack) ) {

      if(debug_flag) { // DEBUG
	std::cout<<"Event is not fully contained inside the active volume!"<<std::endl;
      } // DEBUG

      if(SIMUL_CONTAINMENT_FLAG) {
	*aTrack=Track3D(); // return empty event
      } else {
	if(SIMUL_TRUNCATE_FLAG) {
	  Track3D copy(*aTrack);
	  truncateTracks(copy, debug_flag); // truncate tracks to detector's volume
	  *aTrack=copy;
	}
      }
    }

    // skip bad or parially contained events
    if(aTrack->getSegments().size()==0) {
      if(debug_flag) std::cout<<"Fake event i="<<i<<" rejected!"<<std::endl;
      continue; 
    }

    // simulates triggering on the 1st track (arrival at 10% of TIME scale)
    if(SIMUL_EXT_TRG_FLAG) {

      // sort a copy of the track collection in ascending Z-coordinate order
      auto list(aTrack->getSegments());
      std::sort(list.begin(), list.end(),
		[](const TrackSegment3D& a, const TrackSegment3D& b) {
		  return std::min(a.getEnd().Z(), a.getStart().Z()) < std::min(b.getEnd().Z(), b.getStart().Z());
		});
      auto zmin=myGeometry->GetDriftCageZmin();
      auto zmax=myGeometry->GetDriftCageZmax();
      const double true_zmin=std::min(list.front().getStart().Z(), list.front().getEnd().Z());
      for(auto &seg: aTrack->getSegments()) {
	seg.setStartEnd(TVector3(seg.getStart().X(), seg.getStart().Y(),
				 seg.getStart().Z()-true_zmin+zmin+SIMUL_EXT_TRG_ARRIVAL*(zmax-zmin)),
			TVector3(seg.getEnd().X(), seg.getEnd().Y(),
				 seg.getEnd().Z()-true_zmin+zmin+SIMUL_EXT_TRG_ARRIVAL*(zmax-zmin)));
      }
    }
    aTree->Fill();

    // make 3D plot (optional)
    if(SIMUL_PLOT3D_FLAG && outputCanvas) {
      auto list=aTrack->getSegments();
      const int ntracks=list.size();
      //      auto l = new TPolyLine3D(ntracks*3);
      for(auto i=0; i<ntracks; i++) {
        TPolyLine3D l(2);
	l.SetPoint(0, list.at(i).getStart().X(), list.at(i).getStart().Y(), list.at(i).getStart().Z());
	l.SetPoint(1, list.at(i).getEnd().X(), list.at(i).getEnd().Y(), list.at(i).getEnd().Z());
	l.SetLineColor(color);
	l.SetLineWidth(2);
	if(list.at(i).getLength()>1.0) l.DrawClone(); // skip tracks below 1mm
      }
    }

  } // end of event loop

  aTree->Write("",TObject::kOverwrite);
  aFile->Close();

  if(SIMUL_PLOT3D_FLAG && outputCanvas) {
    outputCanvas->Modified();
    outputCanvas->Update();
    outputCanvas->Print("GeneratorLevel_FakeEvents3D.C");
  }

}
//////////////////////////
//////////////////////////
int main (const int argc, const char *args[]) {

  if(argc!=6) return -1;
  generateFakeRecoEvents(args[1], atol(args[2]), atof(args[3]), atof(args[4]), atof(args[5]));
  return 0;
}
