#ifndef __GEOMETRYTPC_H__
#define __GEOMETRYTPC_H__

// TPC geometry class.
// VERSION: 05 May 2018

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream> // for: cout, cerr, endl
#include <sstream>
#include <iterator>
#include <fstream>
#include <utility>
#include <numeric>

#include "TROOT.h"
#include "TMath.h"
#include "TVector2.h"
#include "TGraph.h"
#include "TH2Poly.h"
#include "MultiKey.h"
#include "CommonDefinitions.h"
 
constexpr auto FPN_CH = projection(3);    // FPN channel type index
constexpr auto ERROR = -1;   // error result indicator
constexpr auto NUM_TOLERANCE = 1e-6;

class StripTPC;

// UVW strip geometry defined as a class

//class GeometryTPC : public TObject {
class GeometryTPC {

 private:

  bool initOK;                          // was geometry initialized properly?
  int COBO_N;                           // total # of COBO boards in the system
  int AGET_Nchips;                      // # of AGET chips per ASAD board
  int AGET_Nchan;                       // # of channels in AGET chip (without FPN channels)
  int AGET_Nchan_fpn;                   // # of FPN channels in AGET chip
  int AGET_Nchan_raw;                   // # of total channels in AGET chip (including FPN channels)
  int AGET_Ntimecells;                  // # of time cells (buckets) in AGET chip
  std::map<projection, int>         stripN{ {projection::DIR_U, 0},{projection::DIR_V, 0},{projection::DIR_W, 0} };    // pair=(directory_idx, number_of_strips=xxx,xxx,xxx,4*ASAD_N*COBO_N)                          
  std::map<projection, std::string> dir2name;  // pair=(directory_idx, group_name="U","V","W","FPN")
  std::map<std::string, projection> name2dir;  // pair=(group_name="U","V","W","FPN", directory_idx)
  std::array<std::array<std::array<std::array<std::shared_ptr<StripTPC>, 64 /*channel_idx[0-63]*/>, 4 /*AGET_idx[0-3]*/>, 4 /*ASAD_idx[0-3]*/>, 2 /*COBO_idx[0-1]*/> arrayByAget;
  std::map<projection, std::map<int, std::shared_ptr<StripTPC>>> stripArray; // key = {DIR_U,DIR_V,DIR_W}, STRIP_NUMBER [1-1024]
  std::vector<int> ASAD_N;       // pair=(COBO_idx, number of ASAD boards)
  std::vector<int> FPN_chanId;     // FPN channels in AGET chips
  double pad_size;                 // in [mm]
  double pad_pitch;                // in [mm]
  double strip_pitch;              // in [mm]
  TVector2 reference_point;        // XY offset in [mm] of the REFERENCE POINT used to define the geometry
  std::map<projection, TVector2> strip_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending pad numbers of the strip
  std::map<projection, TVector2> pitch_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending strip numbers 
  std::map<int /* TH2Poly bin index [1..1024] */, std::shared_ptr<StripTPC> /* TPC strip */> fStripMap; // maps TH2Poly bin to a given StripTPC object
  double vdrift;                   // electron drift velocity in [cm / micosecond]
  double sampling_rate;            // electronics sampling rate in [MHz]
  double trigger_delay;            // delay in [microseconds] of the external "t0" trigger signal (for accelerator beam) 
  double drift_zmin;               // lower drift cage acceptance limit along Z-axis [mm] (closest to readout PCB)
  double drift_zmax;               // upper drift cage acceptance limit along Z-axis [mm] (farthest from readout PCB)
  std::shared_ptr<TH2Poly> tp;                     // for internal storage of arbitrary strip shapes
  int grid_nx;                     // partition size of TH2Poly in X-dir
  int grid_ny;                     // partition size of TH2Poly in Y-dir
  bool _debug;                     // debug/verbose info flag
     
  // Setter methods 
  
  bool Load(std::string fname);                 // loads geometry from TXT config file
  bool InitTH2Poly();                           // define bins for the underlying TH2Poly histogram

  void SetTH2PolyStrip(int ibin, std::shared_ptr<StripTPC> s);  // maps TH2Poly bin to a given StripTPC object
  
 public:

	 GeometryTPC() = default;  // empty constructor for required by TObject
  //  virtual ~GeometryTPC();
  
  // Setter methods 
  
  GeometryTPC(std::string  fname, bool debug=false);
  inline void SetDebug(bool flag) { _debug = flag; }

  // Getter methods

  inline std::shared_ptr<TH2Poly> GetTH2Poly() { return tp; }   // returns pointer to the underlying TH2Poly
  
  inline bool IsOK() { return initOK; }
  int GetDirNstrips(projection dir);

  inline int GetAgetNchips() { return AGET_Nchips; }
  inline int GetAgetNchannels() { return AGET_Nchan; }
  inline int GetAgetNchannels_raw() { return AGET_Nchan_raw; }
  inline int GetAgetNchannels_fpn() { return AGET_Nchan_fpn; }
  inline int GetAgetNtimecells() { return AGET_Ntimecells; }
  inline int GetAsadNboards() { int n=0; for(int icobo=0; icobo<COBO_N; icobo++) { n+=ASAD_N[icobo]; } return n; }
  inline int GetCoboNboards() { return COBO_N; }

  std::string GetDirName(projection dir);

  std::shared_ptr<StripTPC> GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx);         // valid range [0-1][0-3][0-3][0-63]
  std::shared_ptr<StripTPC> GetStripByDir(projection dir, int num);                                                   // valid range [0-2][1-1024]
  decltype(stripArray)& GetStripArray() { return stripArray; };
  std::vector<std::shared_ptr<StripTPC>> GetStrips();

  // various helper functions for calculating local/global normal/raw channel index
  int Aget_normal2raw(int channel_idx);                      // valid range [0-63]
  int Aget_fpn2raw(int FPN_idx);                             // valid range [0-3]

  int Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx);   // valid range [0-1][0-3][0-3][0-63]

  bool GetCrossPoint(std::shared_ptr<StripTPC> strip1, std::shared_ptr<StripTPC> strip2, TVector2 &point);
  bool MatchCrossPoint(std::shared_ptr<StripTPC> strip1, std::shared_ptr<StripTPC> strip2, std::shared_ptr<StripTPC> strip3, double radius, TVector2 &point);

  inline double GetPadPitch() { return pad_pitch; } // [mm]
  inline double GetStripPitch() { return strip_pitch; } // [mm]
  inline TVector2 GetReferencePoint() { return reference_point; } // XY ([mm],[mm])
  TVector2 GetStripPitchVector(projection dir); // XY ([mm],[mm])

  double Strip2posUVW(projection dir, int number); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a given direction
  double Strip2posUVW(std::shared_ptr<StripTPC> strip); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a strip given direction

  double Timecell2pos(double position_in_cells, bool &err_flag); // [mm] output: position along Z-axis

  std::tuple<double, double, double, double> rangeXY(); //min/max X Y cartesian coordinates covered by strips in any direction
  
  //  ClassDef(GeometryTPC,1)
};

#include "StripTPC.h"

#endif
