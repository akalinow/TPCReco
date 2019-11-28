#ifndef __TRACK_SEGMENT_TPC_H__
#define __TRACK_SEGMENT_TPC_H__

// 3D track segemnt definition
// VERSION: 06 May 2018

//#include <cstdlib>
#include <vector>
#include <iostream>
#include <map>
#include <iterator>
#include <memory>
#include <algorithm>

#include "root/include/TH1D.h"
#include "root/include/TH3F.h"
#include "root/include/TVector3.h"
#include "root/include/TVector2.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"
#include "GeometryTPC.h"

#define TRACKSEGMENTTPC_DEFAULT_FIT_SIGMA  1.5 // default expected position resolution [mm] for CHI2 definition (used in: 0,2)
#define TRACKSEGMENTTPC_DEFAULT_FIT_METHOD 2   // 0 = sum of "perpendicular" distances 
                                               // 1 = sum of "parallel" distances assuming evenly distributed hits along the track
                                               // 2 = linear combination of method 0 and 1
class TrackSegment2D;

// 3D-track segment class

class TrackSegment3D {

 private:

  // track direction:
  TVector3 start_point;             // XYZ point in (mm,mm,mm)
  TVector3 end_point;               // XYZ point in (mm,mm,mm)
  TVector3 unit_vec;                // unit vector pointing from START point to END point
  double   length;                  // track segment length [mm]

  // charge distribution along the track:
  unsigned int charge_proj_nbins;            // number of bins of 1D charge distribution
  std::vector<double> charge_proj;  // 1D charge distribution along the track

  // various variables used for fitting:
  /*
  double sum1_weight;    // observable #1: sum of weights
  double sum1_val;       // observable #1: sum of weighted values
  double sum1_val2;      // observable #1: sum of weighted squared values
  double sum1_exp_sigma; // observable #1: expected std. deviation for CHI2 definition

  double sum2_weight;    // observable #2: sum of weights
  double sum2_val;       // observable #2: sum of weighted values
  double sum2_val2;      // observable #2: sum of weighted squared values
  double sum2_exp_sigma; // observable #1: expected std. deviation for CHI2 definition
  */
  double sum_distance;              // sum of weighted distances of all cluster's hits from this track segment
  
 protected:

  void AddHit3D(TVector3 hit_pos, double hit_charge);
  inline void AddHit3D(double hit_x, double hit_y, double hit_z, double hit_charge) {
    return AddHit3D( TVector3(hit_x, hit_y, hit_z), hit_charge );
  }
  void ResetStat(); // resets accummulated statistics
  
 public:

  // Setter methods
  
  TrackSegment3D(const TVector3 & p1, const TVector3 & p2, const int nbins=5);

  void SetStartEndPoints(const TVector3 & p1, const TVector3 & p2);

  // Getter methods
  
  inline TVector3 GetStartPoint() { return start_point; }  // (mm,mm,mm)
  inline TVector3 GetEndPoint() { return end_point; }      // (mm,mm,mm)
  inline double GetLength() { return length; } // mm
  inline TVector3 GetUnitVector() { return unit_vec; }
  bool SetComparisonCluster(SigClusterTPC &cluster); // cluster = UVW clustered data
  bool SetComparisonCluster(TH3F *h3); // cluster = 3D distribution (mm,mm,mm,charge)
  inline double GetClusterDistance() { return sum_distance; } // sum of distances of all hits of the current comparison cluster from the track segment
  TH1D *GetClusterProjection();  // binned 1D charge distribution along the track
  std::vector<double> GetClusterProjectionVec(); // binned 1D charge distribution along the track
  double GetClusterCharge();     // ADC units

  TrackSegment2D GetTrack2D(GeometryTPC *geo_ptr, projection dir); // project 3D track segment onto U-Z, V-Z or W-Z plane
  
};


// 2D-track segment class

class TrackSegment2D {

 private:

  // track direction:
  TVector2 start_point;             // XY point in (mm,mm)
  TVector2 end_point;               // XY point in (mm,mm)
  TVector2 unit_vec;                // unit vector pointing from START point to END point
  double   length;                  // track segment length [mm]

  // charge distribution along the track:
  unsigned int charge_proj_nbins;   // number of bins of 1D charge distribution
  std::vector<double> charge_proj;  // cached 1D charge distribution along the track
  double charge_proj_total;         // cached integral of 1D charge distribution along the track 

  // various variables used for fitting:
  double sum0_weight;     // observable #1: sum of obs. weights
  double sum0_val2;       // observable #1: sum of weighted squares of obs. value
  double sum0_resolution2;// observable #1: expected std. deviation (variance) for CHI2 definition

  double sum1_weight;     // observable #2: sum of obs. weights
  double sum1_val2;       // observable #2: sum of weighted squares of obs. value
  double sum1_resolution2;// observable #2: expected square of std. deviation (variance) for CHI2 definition

 protected:

  bool charge_OK;  // trigger recalculation of charge_proj upon next call of: GetChargeProjection, GetChargeProjectionData?
  bool chi2_OK;    // trigger recalculation of CHI2 upon next call of GetChi2?
  bool _debug;

  void AddHit2D(TVector2 hit_pos, double hit_charge);
  inline void AddHit2D(double hit_x, double hit_y, double hit_charge) {
    return AddHit2D( TVector2(hit_x, hit_y), hit_charge );
  }
  void Update();                   // update geometry, clear cache when needed
  void UpdateChargeProjection();   // update 1D charge projecion cache
  void UpdateChi2();               // update CHI2 cache
  void ResetChi2();                // clear CHI2 cache
  void ResetChargeProjection();    // clear charge projection cache

  // Nested helper class definitions
  class Hit2D {
  public:
    double hit_x;
    double hit_y;
    double hit_charge;
    Hit2D(double x, double y, double charge) : hit_x(x), hit_y(y), hit_charge(charge) { }
  };
  std::vector< Hit2D > cluster_hits; // collection of 2D hits for calculating CHI2

 public:

  // Setter methods
  TrackSegment2D();  
  TrackSegment2D(const TVector2 p1, const TVector2 p2, const unsigned int nbins=5);
  TrackSegment2D(const double x1, const double y1, const double x2, const double y2, const unsigned int nbins=5);
  inline void SetStartPoint(TVector2 p1) { start_point=p1; Update(); }
  inline void SetStartPoint(double x, double y) { start_point=TVector2(x, y); Update(); }
  inline void SetEndPoint(TVector2 p2) { end_point=p2; Update(); }
  inline void SetEndPoint(double x, double y) { end_point=TVector2(x, y); Update(); }
  inline void SetChargeNbins(unsigned int nbins) { charge_proj_nbins=nbins; Update(); }
  inline void SetDebug(bool flag) { _debug=flag; }

  // Getter methods
  
  inline unsigned int GetChargeNbins() { return charge_proj_nbins; } 
  inline TVector2 GetStartPoint() { return start_point; }  // (mm,mm)
  inline TVector2 GetEndPoint() { return end_point; }      // (mm,mm)
  inline double GetLength() { return length; } // mm
  inline TVector2 GetUnitVector() { return unit_vec; }
  bool SetCluster(SigClusterTPC &cluster, projection dir); // cluster = UVW clustered data for a given direction DIR
  bool SetCluster(TH2D *h2); // cluster = 2D distribution (mm,mm,charge)
  double GetChi2(double expected_resolution=TRACKSEGMENTTPC_DEFAULT_FIT_SIGMA, // mm
                 int method=TRACKSEGMENTTPC_DEFAULT_FIT_METHOD);
  TH1D *GetChargeProjection();  // binned 1D charge distribution along the track
  std::vector<double> GetChargeProjectionData(); // binned 1D charge distribution along the track
  double GetCharge(); // ADC units
  void Print(); // prints some info

};


#endif
