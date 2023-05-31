#include <iostream>
#include <vector>
#include <map>
#include <iterator>

#include <TH1D.h>
#include <TH3F.h>
#include <TVector2.h>
#include <TVector3.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/TrackSegmentTPC.h"
#include "TPCReco/SigClusterTPC.h"

/* ============= 3D TRACK SEGMENT CLASS ===========*/
TrackSegment3D::TrackSegment3D(const TVector3 & p1,
			       const TVector3 & p2,
			       const int nbins) :  
  charge_proj_nbins(1),
  sum_distance(0.0){

  SetStartEndPoints(p1, p2);
  
}

// resets accummulated statistics
void TrackSegment3D::ResetStat() {
  sum_distance = 0.0;
  charge_proj.clear();
  charge_proj.resize(charge_proj_nbins, 0.0); // initialize elements with zeros
  
}

void TrackSegment3D::SetStartEndPoints(const TVector3 & p1, const TVector3 & p2){

  start_point = p1;
  end_point = p2;
  sum_distance = 0.0;
  length = (start_point-end_point).Mag(); // mm
  if(length>0.0) unit_vec = (end_point-start_point).Unit();
  else           unit_vec = TVector3(0., 0., 0.); // zero-length vector
  ResetStat();

}

void TrackSegment3D::AddHit3D(TVector3 hit_pos, double hit_charge) {

  // sanity check
  if(hit_charge==0.0) return;

  // TVector3: HIT position relative to the START point
  const TVector3 hit_vec(hit_pos-start_point);
  
  // calc distance from START point to the HIT projection on the track segment
  const double proj_dist = hit_vec*unit_vec;

  // calc weight
  const double weight = fabs(hit_charge);
  
  // calc charge-weighted distance of the HIT from the track segment
  // for three possible cases:
  
  // CASE 1: outside hit, but closer to the START point:  x---[1]======[2]
  if(proj_dist<0.0) {
    // charge-weighted squared DISTANCE from the START point
    sum_distance += hit_vec.Mag2() * weight;
    charge_proj[0] += hit_charge;
    return;
  }

  // CASE 2: outside hit, but closer to the END point:  [1]======[2]---x
  if(proj_dist>length) {
    // charge-weighted squared DISTANCE from the END point
    sum_distance += (hit_vec-unit_vec*length).Mag2() * weight;
    charge_proj[charge_proj_nbins-1] += hit_charge;
    return;
  }

  // CASE 3: inside hit, between START and END points:  [1]===x==[2]
  // charge-weighted squared DISTANCE from the straight line defined by START and STOP points
  sum_distance += (hit_vec.Mag2()-proj_dist*proj_dist) * weight;
  if(length==0.0) charge_proj[0] += hit_charge;
  else            charge_proj[ (unsigned)((proj_dist/length)*(charge_proj_nbins-0.5)) ] += hit_charge;

}

// Sets the current comparison cluster from UVW(t) clustered data
bool TrackSegment3D::SetComparisonCluster(SigClusterTPC &cluster) { // cluster = UVW clustered data

  // reset statistics
  ResetStat();

  // sanity checks
  if(!cluster.IsOK()) return false;
  GeometryTPC* geo_ptr = cluster.GetEvtPtr()->GetGeoPtr();
  if(!geo_ptr || !geo_ptr->IsOK()) return false;

  // create UZ, VZ, WZ projections of the parent 3D track segment
  
  std::map<int, TrackSegment2D> trkMap; // 1-key map: strip_dir [0-2]
  trkMap[definitions::projection_type::DIR_U] = GetTrack2D(geo_ptr, definitions::projection_type::DIR_U);
  trkMap[definitions::projection_type::DIR_V] = GetTrack2D(geo_ptr, definitions::projection_type::DIR_V);
  trkMap[definitions::projection_type::DIR_W] = GetTrack2D(geo_ptr, definitions::projection_type::DIR_W);
  
  // Loop over 2D cluster hits and update statistics
  for(auto it=trkMap.begin(); it!=trkMap.end(); it++) {
    it->second.SetCluster(cluster, it->first); // DIR=it->first
    sum_distance += it->second.GetChi2();
    //std::vector<double> clust_charge_proj( it->second.GetChargeProjectionData() );
    //for(unsigned int i=0; i<clust_charge_proj.size(); i++) {
    //if(i<charge_proj_nbins) charge_proj[i] += clust_charge_proj[i];
    //}
  } // end of for(it=...
  
  return true;
}

// Sets the current comparison cluster from XYZ 3D charge distribution
bool TrackSegment3D::SetComparisonCluster(TH3F *h3) { // XYZ histogram in mm x mm x mm

  // reset statistics
  ResetStat();

  // sanity checks
  if(!h3) return false;
    
  const double xmin = h3->GetXaxis()->GetXmin();
  const double ymin = h3->GetYaxis()->GetXmin();
  const double zmin = h3->GetZaxis()->GetXmin();

  const double dx = (h3->GetXaxis()->GetXmax() - xmin)/h3->GetNbinsX();
  const double dy = (h3->GetYaxis()->GetXmax() - ymin)/h3->GetNbinsY();
  const double dz = (h3->GetZaxis()->GetXmax() - zmin)/h3->GetNbinsZ();

  // Loop over histogram cells and update statistics
  for(Int_t ix=1; ix<=h3->GetNbinsX(); ix++) {
    const double x = xmin + (ix-0.5)*dx; // X center of the cell
    for(Int_t iy=1; iy<=h3->GetNbinsY(); iy++) {
      const double y = ymin + (iy-0.5)*dy; // Y center of the cell
      for(Int_t iz=1; iz<=h3->GetNbinsZ(); iz++) {	
	const double z = zmin + (iz-0.5)*dz; // Z center of the cell
	const double val = h3->GetBinContent(ix, iy, iz);

	AddHit3D(x, y, z, val); 

      } // end of for(Int_t iz=... 
    } // end of for(Int_t iy=... 
  } // end of for(Int_t ix=... 
    
  return true;
}

// 1D projection of the current comparison cluster on this track segment
std::vector<double> TrackSegment3D::GetClusterProjectionVec() {
  return charge_proj;
}

// 1D projection of the current comparison cluster on this track segment
TH1D *TrackSegment3D::GetClusterProjection() {

  if( length==0.0 ) return NULL;
  TH1D *h1 = new TH1D("h_track_charge", "Charge along the track segment;Length [mm];Charge/bin [arb.u.]",
		      charge_proj_nbins, 0.0, length);
  for(unsigned int i=0; i<charge_proj_nbins; i++) {
    h1->Fill( length*(i+0.5)/charge_proj_nbins, charge_proj[i] );
  }
  return h1;
}

// Sum of charges for all hits of the current comparison cluster
double TrackSegment3D::GetClusterCharge() {
  double sum_charge=0.0;
  for(unsigned int i=0; i<charge_proj_nbins; i++) {
    sum_charge += charge_proj[i];
  }
  return sum_charge;
}

// project 3D track segment onto U-Z, V-Z or W-Z plane
TrackSegment2D TrackSegment3D::GetTrack2D(GeometryTPC *geo_ptr, int dir) {

  static TrackSegment2D empty(TVector2(), TVector2(), 0); // empty track

  // sanity check
  if(!geo_ptr || !(geo_ptr->IsOK())) return empty;

  // project START and STOP points
  switch(dir) {
  case definitions::projection_type::DIR_U:
  case definitions::projection_type::DIR_V:
  case definitions::projection_type::DIR_W:
    bool err_flag;
    return
      TrackSegment2D( TVector2( geo_ptr->Cartesian2posUVW(start_point.X(), start_point.Y(), dir, err_flag), // U or V or W position [mm]
				start_point.Z() ), // Z position [mm]
		      TVector2( geo_ptr->Cartesian2posUVW(end_point.X(), end_point.Y(), dir, err_flag),     // U or V or W position [mm]
				end_point.Z() ),   // Z position [mm]
		      charge_proj_nbins ); // same binning as parent track
  };
  return empty;
}


/* ============= 2D TRACK SEGMENT CLASS ===========*/
TrackSegment2D::TrackSegment2D(){

  cluster_hits.clear();
  Update();
  
}

TrackSegment2D::TrackSegment2D(const TVector2 p1,
			       const TVector2 p2,
			       const unsigned int nbins) :
  start_point(p1),
  end_point(p2),
  charge_proj_nbins(nbins),
  charge_proj_total(0.0),
  sum0_weight(0.0),
  sum0_val2(0.0),
  sum0_resolution2(0.0),
  sum1_weight(0.0),
  sum1_val2(0.0),
  sum1_resolution2(0.0),
  charge_OK(false),
  chi2_OK(false),
  _debug(false)
{
  cluster_hits.clear();
  Update();
}

TrackSegment2D::TrackSegment2D(const double x1,
			       const double y1,
			       const double x2,
			       const double y2,
			       const unsigned int nbins) :
  TrackSegment2D( TVector2(x1, y1), TVector2(x2, y2), nbins)
{ ; }

// updates geometry
void TrackSegment2D::Update() {

  static unsigned int old_charge_proj_nbins = 0; // ERROR
  static TVector2 old_start_point = TVector2(0., 0.); // EMPTY
  static TVector2 old_end_point = TVector2(0., 0.); // EMPTY

  // always update these variables:
  length = (start_point-end_point).Mod(); // mm
  if(length>0.0) unit_vec = (end_point-start_point).Unit();
  else           unit_vec = TVector2(0., 0.); // zero-length vector

  // conditional update of selected variables:
  if(old_start_point.X()!=start_point.X() || old_end_point.X()!=end_point.X() ||
     old_start_point.Y()!=start_point.Y() || old_end_point.Y()!=end_point.Y()) {
    charge_OK = false;
    chi2_OK = false;
  }

  if(old_charge_proj_nbins!=charge_proj_nbins) {
    charge_OK = false;
  }

  if(!charge_OK) ResetChargeProjection();
  if(!chi2_OK) ResetChi2();

  // store current values
  old_charge_proj_nbins = charge_proj_nbins;
  old_start_point = start_point;
  old_end_point = end_point;

  if(_debug) Print();
}

// prints some info
void TrackSegment2D::Print() {

  std::cout << "TRK segment START [mm]: [" << start_point.X()
	    << ", " << start_point.Y() << "]" << std::endl
	    << "TRK segment END [mm]: [" << end_point.X()
	    << ", " << end_point.Y() << "]" << std::endl
	    << "TRK segment LENGTH [mm]: " << length << std::endl
    	    << "TRK segment NHITS: " << cluster_hits.size() << std::endl
	    << "TRK segment CHARGE [arb.u.]: " << GetCharge() << std::endl;

}

double TrackSegment2D::GetChi2(double expected_resolution, // mm
			       int method) {
  // try to use cached data if possible
  if(!chi2_OK && cluster_hits.size()) {
    UpdateChi2();
  }

  sum0_resolution2 = expected_resolution * expected_resolution;
  // observable:     H(x) = x*(L-x)
  // expected value: E(H) = (L^2)/6
  // variance:       V(H) = (L^4)/180 = Sigma(H)^2 
  sum1_resolution2 = length * length * length * length / 180.;

  // debug
  if(_debug) {
    std::cout << "TrackSegment2D::GetChi2: Method-0: sum0_resolution2=" << sum0_resolution2 << " mm^2\n";
    std::cout << "                         Method-1: sum1_resolution2=" << sum1_resolution2 << " mm^4\n";
  }
      
  //
  // compute CHI2 depending on selected METHOD = 0/1/2:
  //
  double chi2 = -1.0; // ERROR
  switch(method) {
  case 0: // use only "perpendicular" distances of hits from the track segment 
    if(sum0_weight>0.0 && sum0_resolution2>0.0) {
      chi2 = sum0_val2 / ( sum0_resolution2 * sum0_weight );
      chi2_OK = true;
    }
    break;
  case 1: // use only "parallel" distances of hits from START and END points of the track segment 
    if(sum1_weight>0.0 && sum1_resolution2>0.0) {
      chi2 = sum1_val2 / ( sum1_resolution2 * sum1_weight );
      chi2_OK = true;
    }
    break;
  case 2: // combination of method 0 and 1
    if(sum0_weight>0.0 && sum1_weight>0.0 && sum0_resolution2>0.0 && sum1_resolution2>0.0) {
      chi2 = sum0_val2 / ( sum0_resolution2 * sum0_weight ) + 
	     sum1_val2 / ( sum1_resolution2 * sum1_weight );
      chi2_OK = true;
    }
    break;
  default: chi2_OK = false; // ERROR
  }

  // debug
  if(_debug) {
    std::cout << "TrackSegment2D::GetChi2: chi2=" << chi2 << "\n\n";
  }

  return chi2;

}

void TrackSegment2D::UpdateChi2() {

  ResetChi2();
  
  std::vector< Hit2D >::const_iterator it;
  for(it=cluster_hits.cbegin(); it!=cluster_hits.cend(); it++) {
      
    // TVector2: HIT position
    const TVector2 hit_pos( (*it).hit_x, (*it).hit_y );
    
    // TVector2: HIT position relative to the START point
    const TVector2 hit_vec(hit_pos-start_point); 
    
    // distance from the START point to the HIT projection parallel to the track segment
    const double proj_dist = hit_vec*unit_vec;
    
    // HIT weight
    const double weight = fabs( (*it).hit_charge );
    
    //
    // Calculate partial sums for METHOD 0:
    // - charge-weighted distance of the HIT from the track segment
    //   for three possible cases:
    //
    // CASE 1: 
    // - outside hit, but closer to the START point:  x---[1]======[2]
    // - charge-weighted square of the DISTANCE from the START point
    if(proj_dist<0.0) {
      sum0_val2      += hit_vec.Mod2() * weight;
      sum0_weight    += weight;
      charge_proj[0] += (*it).hit_charge;
    } else {
      // CASE 2: 
      // - outside hit, but closer to the END point:  [1]======[2]---x
      // - charge-weighted square of the DISTANCE from the END point
      if(proj_dist>length) {      
	sum0_val2   += (hit_vec-unit_vec*length).Mod2() * weight;
	sum0_weight += weight;
	charge_proj[charge_proj_nbins-1] += (*it).hit_charge;
      } else {
	// CASE 3: 
	// - inside hit, between START and END points:  [1]===x==[2]
	// - charge-weighted square of the DISTANCE from the straight line defined by START and STOP points
	sum0_val2   += (hit_vec.Mod2()-proj_dist*proj_dist) * weight;
	sum0_weight += weight; 
	if(length==0.0) charge_proj[0] += (*it).hit_charge;
	else            charge_proj[ (unsigned int)((proj_dist/length)*(charge_proj_nbins-0.5)) ] += (*it).hit_charge;
      }
    }

    //
    // Calculate partial sums for METHOD 1:
    // - charge-weighted square of the geometric average distance of HIT projection on the track segment 
    //   from the START point and from the END point
    //    
    sum1_val2      += ( proj_dist*proj_dist*(length-proj_dist)*(length-proj_dist) - length*length/6 ) * weight;
    sum1_weight    += weight;

  } // end of for(it=...

  if(_debug) {
    std::cout << "TrackSegment2D::UpdateChi2: Method-0: sum0_val2=" << sum0_val2 << ", sum0_weight=" << sum0_weight << std::endl;
    std::cout << "                            Method-1: sum1_val2=" << sum1_val2 << ", sum1_weight=" << sum1_weight << std::endl;
  }

}

void TrackSegment2D::AddHit2D(TVector2 hit_pos, double hit_charge) {

  // sanity check
  if(hit_charge==0.0) return;

  // add new 2D HIT to the list
  cluster_hits.push_back( Hit2D(hit_pos.X(), hit_pos.Y(), hit_charge) );

  // debug
  if(_debug) {
    std::cout << "TrackSegment2D: Adding hit " << cluster_hits.size() << ": x=" << hit_pos.X() << " mm, y=" << hit_pos.Y() << " mm, Q=" << hit_charge << std::endl;
  }
}

// Sets the current comparison cluster from UVW(t) clustered data
bool TrackSegment2D::SetCluster(SigClusterTPC &cluster, int dir) { // cluster = UVW clustered data
  
  // clear list of hits
  cluster_hits.clear();
  
  // clear caches
  ResetChi2();
  ResetChargeProjection();
  
  // sanity checks
  if(!cluster.IsOK()) return false;
  GeometryTPC* geo_ptr = cluster.GetEvtPtr()->GetGeoPtr();
  if(!geo_ptr || !geo_ptr->IsOK()) return false;

  // Loop over 2D cluster hits and update statistics
  // key=(TIME_CELL [0-511], STRIP_NUM [1-1024])
  std::vector<MultiKey2> cluster_hits=cluster.GetHitListByDirMerged(dir); // all sections
  std::vector<MultiKey2>::iterator it;
  bool err_flag;

  for(it=cluster_hits.begin(); it!=cluster_hits.end(); it++) {

    const double uvw_proj =
      geo_ptr->Strip2posUVW(dir, std::get<1>(*it), err_flag); // U/V/W [mm]
    const double z =
      geo_ptr->Timecell2pos( (std::get<0>(*it)+0.5), err_flag); // Z [mm] - middle of the time cell
    const double val =
      cluster.GetEvtPtr()->GetValByStripMerged(dir, std::get<1>(*it), std::get<0>(*it)); // all sections

    AddHit2D( uvw_proj, z, val);
    
  } // end of for(it=...
  
  return true;
}

// Sets the current comparison cluster from XYZ 3D charge distribution
bool TrackSegment2D::SetCluster(TH2D *h2) { // XY histogram in mm x mm

  // clear list of hits
  cluster_hits.clear();
  
  // reset statistics
  ResetChi2();
  ResetChargeProjection();
  
  // sanity checks
  if(!h2) return false;
    
  const double xmin = h2->GetXaxis()->GetXmin();
  const double ymin = h2->GetYaxis()->GetXmin();

  const double dx = (h2->GetXaxis()->GetXmax() - xmin)/h2->GetNbinsX();
  const double dy = (h2->GetYaxis()->GetXmax() - ymin)/h2->GetNbinsY();

  // Loop over histogram cells and update statistics
  for(Int_t ix=1; ix<=h2->GetNbinsX(); ix++) {
    const double x = xmin + (ix-0.5)*dx; // X center of the cell
    for(Int_t iy=1; iy<=h2->GetNbinsY(); iy++) {	
      const double y = ymin + (iy-0.5)*dy; // Y center of the cell
      const double val = h2->GetBinContent(ix, iy);
      
      AddHit2D(x, y, val); 
      
    } // end of for(Int_t iy=... 
  } // end of for(Int_t ix=... 
  
  return true;
}

// resets CHI2 cache
void TrackSegment2D::ResetChi2() {
  chi2_OK = false;
  sum0_weight = 0.0;
  sum0_val2 = 0.0;
  sum0_resolution2 = 0.0;
  sum1_weight = 0.0;
  sum1_val2 = 0.0;
  sum1_resolution2 = 0.0;
}

// resets charge projection cache
void TrackSegment2D::ResetChargeProjection() {
  charge_OK = false;
  if(charge_proj_nbins<1) charge_proj_nbins = 1;
  charge_proj.clear();
  charge_proj.resize(charge_proj_nbins, 0.0); // initialize elements with zeros
  charge_proj_total=0.0;
}

// triggers update of 1D projection of the current comparison cluster on this track segment
void TrackSegment2D::UpdateChargeProjection() {
  charge_OK = false;
  std::vector< Hit2D >::const_iterator it;
  for(it=cluster_hits.cbegin(); it!=cluster_hits.cend(); it++) {
    unsigned int index = 0;
    if(length>0) index = (int) (((TVector2( (*it).hit_x, (*it).hit_y )-start_point)*unit_vec)/length*charge_proj_nbins);
    if(index<0) index=0;
    else if(index>=charge_proj_nbins) index=charge_proj_nbins;
    charge_proj[index] += (*it).hit_charge;
    charge_proj_total += (*it).hit_charge;
  }
  charge_OK = true;
}

// Calculates 1D projection of the current comparison cluster on this track segment
// Return value: vector with binned charge distribution along the track.
// Any underflows (overflows) are stored in the first (last) bin.
// When length=0 returns 1 bin.
std::vector<double> TrackSegment2D::GetChargeProjectionData() {

  // try to use cached data if possible
  if(!charge_OK) {
    UpdateChargeProjection();
  }

  return charge_proj;
}

// Calculates 1D projection of the current comparison cluster on this track segment
// Return value: pointer to a new TH1D histogram.
// Any underflows (overflows) are stored in the first (last) bin.
// When length=0 returns NULL.
TH1D *TrackSegment2D::GetChargeProjection() {

  // try to use cached data if possible
  if(!charge_OK) {
    UpdateChargeProjection();
  }

  if( length==0.0 ) return NULL;
  TH1D *h1 = new TH1D("h_dEdx", "Charge along the track segment;Length [mm];Charge/bin [arb.u.]",
		      charge_proj_nbins, 0.0, length);
  for(unsigned int index=0; index<charge_proj_nbins; index++) {
    h1->Fill( length*(index+0.5)/charge_proj_nbins, charge_proj[index] );
  }
  return h1;
}

// Calculates integral of 1D projection of the current comparison cluster on this track segment
double TrackSegment2D::GetCharge() {

  // try to use cached data if possible
  if(!charge_OK) {
    UpdateChargeProjection();
  }

  return charge_proj_total;
}
