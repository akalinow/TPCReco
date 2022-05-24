#ifndef __GEOMETRYTPC_H__
#define __GEOMETRYTPC_H__

// TPC geometry class.
// VERSION: 30 Apr 2022

#include <cstdlib>
#include <cstddef> // for NULL
#include <vector>
#include <map>
#include <string>

#include "TROOT.h"
#include "TVector2.h"
#include "TVector3.h"
#include "TH2Poly.h"
#include "TGraph.h"
#include "MultiKey.h"
#include "CommonDefinitions.h"
#include "RunConditions.h"
#include "GeometryStats.h"
#define FPN_CH   3    // FPN channel type index
#define ERROR    -1   // error result indicator                                                                         
#define MINIMUM(A,B) ((A)<(B) ? (A) : (B))
#define MAXIMUM(A,B) ((A)>(B) ? (A) : (B))
#define NUM_TOLERANCE 1e-6

class StripTPC;

// UVW strip geometry defined as a class

//class GeometryTPC : public TObject {
class GeometryTPC {

  friend class UVWprojector;

 private:
  mutable RunConditions runConditions;
  GeometryStats geometryStats;
  bool initOK;                          // was geometry initialized properly?
  int COBO_N;                           // total # of COBO boards in the system
  int AGET_Nchips;                      // # of AGET chips per ASAD board
  int AGET_Nchan;                       // # of channels in AGET chip (without FPN channels)
  int AGET_Nchan_fpn;                   // # of FPN channels in AGET chip
  int AGET_Nchan_raw;                   // # of total channels in AGET chip (including FPN channels)
  int AGET_Ntimecells;                  // # of time cells (buckets) in AGET chip
  std::map<int, int>         stripN;    // pair=(directory_idx, number_of_strips=xxx,xxx,xxx,4*ASAD_N*COBO_N)                          
  std::map<int, std::string> dir2name;  // pair=(directory_idx, group_name="U","V","W","FPN")
  std::map<std::string, int> name2dir;  // pair=(group_name="U","V","W","FPN", directory_idx)
  std::map<MultiKey4, StripTPC*, multikey4_less> mapByAget;     // key=(COBO_idx[0-1], ASAD_idx[0-3], AGET_idx[0-3], channel_idx[0-63])
  std::map<MultiKey4, StripTPC*, multikey4_less> mapByAget_raw; // key=(COBO_idx[0-1], ASAD_idx[0-3], AGET_idx [0-3], raw_channel_idx [0-67] )
  std::map<MultiKey3, StripTPC*, multikey3_less> mapByStrip;    // key=(STRIP_DIRECTION [0-2], STRIP_SECTION [0-2], STRIP_NUMBER [1-1024])
  std::map<int, int> ASAD_N;       // pair=(COBO_idx, number of ASAD boards)
  std::vector<int> FPN_chanId;     // FPN channels in AGET chips
  double pad_size;                 // in [mm]
  double pad_pitch;                // in [mm]
  double strip_pitch;              // in [mm]
  TVector2 reference_point;        // XY offset in [mm] of the REFERENCE POINT used to define the geometry
  TVector2 empty;                  //dummy vector returned for invalid input data
  std::map<int, TVector2> strip_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending pad numbers of the strip
  std::map<int, TVector2> pitch_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending strip numbers 
  std::map<int /* TH2Poly bin index [1..1024] */, StripTPC* /* TPC strip */> fStripMap; // maps TH2Poly bin to a given StripTPC object
  double drift_zmin;               // lower drift cage acceptance limit along Z-axis [mm] (closest to readout PCB)
  double drift_zmax;               // upper drift cage acceptance limit along Z-axis [mm] (farthest from readout PCB)
  TH2Poly* tp;                     // for internal storage of arbitrary strip shapes
  int grid_nx;                     // partition size of TH2Poly in X-dir
  int grid_ny;                     // partition size of TH2Poly in Y-dir
  bool isOK_TH2Poly;               // is TH2Poly already initialized?
  bool _debug;                     // debug/verbose info flag
  //  TGraph* tp_convex;           // for internal storage of the convex hull for UVW active area
  TH2PolyBin* tp_convex;           // for internal storage of the convex hull for UVW active area

  // Setter methods 
  
  bool Load(const char *fname);                 // loads geometry from TXT config file
  bool LoadAnalog(std::istream &f);                            //subrutine. Loads analog channels from geometry TXT config file
  bool InitTH2Poly();                           // define bins for the underlying TH2Poly histogram

  void SetTH2PolyStrip(int ibin, StripTPC *s);  // maps TH2Poly bin to a given StripTPC object

  bool InitActiveAreaConvexHull(TGraph *g);     // calculates convex hull from cloud of 2D points [mm]
  
 public:
  void Debug();
  GeometryTPC() { ; }  // empty constructor required by TObject
  //  virtual ~GeometryTPC();
  
  // Setter methods 
  
  GeometryTPC(const char* fname, bool debug=false);
  void SetTH2PolyPartition(int nx, int ny); // change cartesian binning of the underlying TH2Poly
  inline int GetTH2PolyPartitionX() { return grid_nx; }
  inline int GetTH2PolyPartitionY() { return grid_ny; }
  inline void SetDebug(bool flag) { _debug = flag; }

  void setDriftVelocity(double v);
  void setSamplingRate(double r);
  void setTriggerDelay(double d);
  
  // Getter methods
  const RunConditions & getRunConditions() const { return runConditions;}

  inline TH2Poly *GetTH2Poly() { return tp; }   // returns pointer to the underlying TH2Poly
  StripTPC *GetTH2PolyStrip(int ibin);          // returns pointer to StripTPC object corresponding to TH2Poly bin 
  
  inline bool IsOK() { return initOK; }
  
  //returns total number of strips in sections
  int GetDirNstrips(int dir);
  int GetDirNstrips(std::string name);
  int GetDirNstrips(const char *name);
  int GetDirNstrips(StripTPC *s);

  inline SectionIndexList GetDirSectionIndexList(int dir){return geometryStats.GetDirSectionIndexList(dir);} // added by MC - 4 Aug 2021
  inline int GetDirNStrips(int dir,int section){return geometryStats.GetDirNStrips(dir,section);}
  inline int GetDirMinStrip(int dir,int section){return geometryStats.GetDirMinStrip(dir,section);}
  inline int GetDirMaxStrip(int dir,int section){return geometryStats.GetDirMaxStrip(dir,section);}
  inline int GetDirNStripsMerged(int dir){return geometryStats.GetDirNStripsMerged(dir);}
  inline int GetDirMinStripMerged(int dir){return geometryStats.GetDirMinStripMerged(dir);}
  inline int GetDirMaxStripMerged(int dir){return geometryStats.GetDirMaxStripMerged(dir);}

  inline int GetAgetNchips() { return AGET_Nchips; }
  inline int GetAgetNchannels() { return AGET_Nchan; }
  inline int GetAgetNchannels_raw() { return AGET_Nchan_raw; }
  inline int GetAgetNchannels_fpn() { return AGET_Nchan_fpn; }
  inline int GetAgetNtimecells() { return AGET_Ntimecells; }
  inline int GetAsadNboards(int COBO_idx) { return (ASAD_N.find(COBO_idx)==ASAD_N.end() ? 0 : ASAD_N[COBO_idx]); }
  inline int GetAsadNboards() { int n=0; for(int icobo=0; icobo<COBO_N; icobo++) { n+=ASAD_N[icobo]; } return n; }
  inline int GetCoboNboards() { return COBO_N; }

  int GetDirIndex(const char *name); 
  int GetDirIndex(std::string name);
  int GetDirIndex(int global_channel_idx);
  int GetDirIndex(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx);
  int GetDirIndex_raw(int global_raw_channel_idx);
  int GetDirIndex_raw(int COBO_idx, int ASAD_idx, int AGET_idx, int raw_channel_idx);

  const char* GetDirName(int dir);
  const char* GetStripName(StripTPC *s);

  StripTPC *GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx);         // valid range [0-1][0-3][0-3][0-63]
  StripTPC *GetStripByGlobal(int global_channel_idx);                                          // valid range [0-1023]
  StripTPC *GetStripByAget_raw(int COBO_idx, int ASAD_idx, int AGET_idx, int raw_channel_idx); // valid range [0-1][0-3][0-3][0-67]
  StripTPC *GetStripByGlobal_raw(int global_raw_channel_idx);                                  // valid range [0-(1023+4*ASAD_N*COBO_N)]
  StripTPC *GetStripByDir(int dir, int section, int num);                                      // valid range [0-2][0-2][1-1024]
  //  StripTPC *GetStripByDir(int dir, int num);   //legacy for section=0, valid range [0-2][1-1024]

  // various helper functions for calculating local/global normal/raw channel index
  int Aget_normal2raw(int channel_idx);                      // valid range [0-63]
  int Aget_raw2normal(int raw_channel_idx);                  // valid range [0-67]
  int Aget_fpn2raw(int FPN_idx);                             // valid range [0-3]

  int Asad_normal2raw(int aget_idx, int channel_idx);        // valid range [0-3][0-63]
  int Asad_normal2raw(int asad_channel_idx);                 // valid range [0-255]
  int Asad_normal2normal(int aget_idx, int channel_idx);     // valid range [0-3][0-63]
  int Asad_raw2normal(int aget_idx, int raw_channel_idx);    // valid range [0-3][0-67]
  int Asad_raw2normal(int asad_raw_channel_idx);             // valid range [0-271]
  int Asad_raw2raw(int aget_idx, int raw_channel_idx);       // valid range [0-3][0-67]

  int Global_normal2raw(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx);      // valid range [0-1][0-3][0-3][0-63]
  int Global_normal2raw(int glb_channel_idx);                                            // valid range [0-1024]
  int Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx);   // valid range [0-1][0-3][0-3][0-63]

  int Global_raw2normal(int COBO_idx, int ASAD_idx, int aget_idx, int raw_channel_idx);  // valid range [0-1][0-3][0-3][0-67]
  int Global_raw2normal(int glb_raw_channel_idx);                                        // valid range [0-(1023+4*ASAD_N*COBO_N)]
  int Global_raw2raw(int COBO_idx, int ASAD_idx, int aget_idx, int raw_channel_idx);     // valid range [0-1][0-3][0-3][0-67]
  int Global_fpn2raw(int COBO_idx, int ASAD_idx, int aget_idx, int FPN_idx);             // valid range [0-1][0-3][0-3][0-3]

  int Global_strip2normal(StripTPC *s);
  int Global_strip2normal(int dir, int section, int num);                                // valid range [0-2][0-2][1-1024]
  //  int Global_strip2normal(int dir, int num);                 //legacy for section=0, valid range [0-2][1-1024]
  int Global_strip2raw(StripTPC *s);
  int Global_strip2raw(int dir, int section, int num);                                   // valid range [0-2][0-2][1-1024]
  //  inline int Global_strip2raw(int dir, int num);                    //legacy for section =0, valid range [0-2][1-1024]

  bool GetCrossPoint(StripTPC *strip1, StripTPC *strip2, TVector2 &point);
  bool GetUVWCrossPointInMM(int dir1, double UVW_pos1, int dir2, double UVW_pos2, TVector2 &point);
  bool MatchCrossPoint(StripTPC *strip1, StripTPC *strip2, StripTPC *strip3, double radius, TVector2 &point);

  inline double GetPadSize() { return pad_size; } // [mm]
  inline double GetPadPitch() { return pad_pitch; } // [mm]
  inline double GetStripPitch() { return strip_pitch; } // [mm]

  inline double GetDriftVelocity() { return runConditions.getDriftVelocity(); } // [cm/us]
  inline double GetSamplingRate() { return runConditions.getSamplingRate(); } // [MHz]
  inline double GetTriggerDelay() { return runConditions.getTriggerDelay(); } // [us]
  inline double GetTimeBinWidth() { return  1.0/GetSamplingRate()*GetDriftVelocity()*10.0; } // [mm]

  inline TVector2 GetReferencePoint() { return reference_point; } // XY ([mm],[mm])
  TVector2 GetStripUnitVector(int dir); // XY ([mm],[mm])
  TVector2 GetStripPitchVector(int dir); // XY ([mm],[mm])
  TVector3 GetStripPitchVector3D(int dir); // XYZ ([mm],[mm],0)  
  inline double GetDriftCageZmin() { return drift_zmin; } // [mm]
  inline double GetDriftCageZmax() { return drift_zmax; } // [mm]

  double Strip2posUVW(int dir, int section, int number, bool &err_flag); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a given direction
  double Strip2posUVW(int dir, int number, bool &err_flag); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a given direction
  double Strip2posUVW(StripTPC *strip, bool &err_flag); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a strip given direction
  
  double Cartesian2posUVW(double x, double y, int dir, bool &err_flag); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of a given (X,Y) point on the strip pitch axis for a given direction
  double Cartesian2posUVW(TVector2 pos, int dir, bool &err_flag); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of a given (X,Y) point on the strip pitch axis for a given direction
  
  double Timecell2pos(double position_in_cells, bool &err_flag); // [mm] output: position along Z-axis
  double Pos2timecell(double z, bool &err_flag); // output: time-cell number, valid range [0-511]

  TGraph GetActiveAreaConvexHull(double vetoBand=0); // get convex hull [mm] of the entire UVW active area
                                                     // with (optionally) excluded outer VETO band [mm]
  bool IsInsideActiveVolume(TVector3 point); // checks if 3D point [mm] has X,Y inside
                                             // UVW active area and Z within [zmin, zmax] range
  bool IsInsideActiveArea(TVector2 point); // checks if 2D point [mm] is inside UVW active area
  bool IsInsideElectronicsRange(double z); // checks if Z coordinate [mm] is inside Z-slice covered by the GET electronics
  bool IsInsideElectronicsRange(TVector3 point); // checks 3D point [mm] is inside Z-slice covered by the GET electronics
  std::tuple<double, double> rangeX(); //min/max X [mm] cartesian coordinates covered by UVW active area
  std::tuple<double, double> rangeY(); //min/max Y [mm] cartesian coordinates covered by UVW active area
  std::tuple<double, double> rangeZ(); //min/max Z [mm] cartesian coordinates covered by drift cage
  std::tuple<double, double, double, double> rangeXY(); //min/max X and Y [mm] cartesian coordinates covered by UVW active area
  std::tuple<double, double, double, double, double, double> rangeXYZ(); //min/max X and Y [mm] cartesian coordinates covered by UVW active area
                                                         // and min/max Z [mm] cartesian coordinates covered by drift cage
  std::tuple<double, double> rangeStripSectionInMM(int dir, int section); // [mm] min/max (signed) distance between projection of outermost strip's central axis and projection of the origin (X=0,Y=0) point on the U/V/W pitch axis for a given direction (per section)
  std::tuple<double, double> rangeStripDirInMM(int dir); // [mm] min/max (signed) distance between projection of outermost strip's central axis and projection of the origin (X=0,Y=0) point on the U/V/W pitch axis for a given direction (all sections)

  //  ClassDef(GeometryTPC,1)
};

// Single UVW strip defined as a class

//class StripTPC : public TObject {
class StripTPC {

  friend class GeometryTPC;

 private:

  GeometryTPC *geo_ptr; // parent pointer
  int dir; // direction/group: 0=U / 1=V / 2=W / 3=FPN / -1=ERROR  
  int section; // 0 =- / 1 =A / 2=B                                                    
  int num; // strip number: 1-1024 for U,V,W / 1-(4*ASAD_N*COBO_N) for FPN / -1=ERROR
  int coboId; // range [0-1]
  int asadId; // range [0-3]
  int agetId; // range [0-3]
  int agetCh; // range [0-63]
  int agetCh_raw; // range [0-67]
  TVector2 unit_vec;   // 2D directional unit vector (towards increasing pad numbers)
  TVector2 offset_vec; // 2D offset vector [mm] of the 1st pad wrt REF.POINT
 // double length;  // strip length [mm]
  int npads; //strip length in pads
 public:

  StripTPC(){};
  StripTPC(int direction, int section, int number, int cobo_index, int asad_index, int aget_index, int aget_channel, int aget_channel_raw, 
	   TVector2 unit_vector, TVector2 offset_vector_in_mm, int number_of_pads, GeometryTPC *geo_ptr);

  inline int Dir() { return dir; }
  inline int Section() { return section; }
  inline int Num() { return num; }
  inline int CoboId() { return coboId; }
  inline int AsadId() { return asadId; }
  inline int AgetId() { return agetId; }
  inline int AgetCh() { return agetCh; }
  inline int AgetCh_raw() { return agetCh_raw; }
  int GlobalCh();
  int GlobalCh_raw();
  inline TVector2 Unit() { return unit_vec; } // ([mm],[mm])
  inline TVector2 Offset() { return offset_vec; } // ([mm],[mm])
  inline double Length() { return geo_ptr->GetPadPitch()*npads; } // [mm]
  inline int Npads() {return npads;}

  //  ClassDef(StripTPC,1)
};

#define __GEOMETRYTPC_H__
#endif
