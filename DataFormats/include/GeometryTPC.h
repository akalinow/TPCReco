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
#include <functional>
#include <mutex>
#include <sstream>
#include <tuple>

#include "TROOT.h"
#include "TMath.h"
#include "TVector2.h"
#include "TGraph.h"
#include "TH2Poly.h"
#include "CommonDefinitions.h"

constexpr auto ERROR = -1;   // error result indicator
constexpr auto NUM_TOLERANCE = 1e-6;

class Geometry_Strip;

class GeometryTPC;

GeometryTPC& Geometry(std::string fname = "");

auto find_line = [](std::vector<std::string>& vec, std::string str) {
	return std::find_if(/*std::execution::par, */vec.begin(), vec.end(), [&](std::string& str_) { return str_.find(str) != std::string::npos; }); //C++17
};

// UVW strip geometry defined as a class

class GeometryTPC {

private:

	int COBO_N;                           // total # of COBO boards in the system
	int AGET_Nchips;                      // # of AGET chips per ASAD board
	int AGET_Nchan;                       // # of channels in AGET chip (without FPN channels)
	int AGET_Nchan_fpn;                   // # of FPN channels in AGET chip
	int AGET_Nchan_raw;                   // # of total channels in AGET chip (including FPN channels)
	int AGET_Ntimecells;                  // # of time cells (buckets) in AGET chip
	int ASAD_Nboards;                     // total # of ASAD boards
	std::map<direction, int>         stripN;    // pair=(directory_idx, number_of_strips=xxx,xxx,xxx,4*ASAD_N*COBO_N)                          
	std::map<direction, std::string> dir2name;  // pair=(directory_idx, group_name="U","V","W","FPN")
	std::map<std::string, direction> name2dir;  // pair=(group_name="U","V","W","FPN", directory_idx)
	std::map<std::tuple<int, int, int, int>, std::shared_ptr<Geometry_Strip>> arrayByAget;/*channel_idx[0-63] AGET_idx[0-3] ASAD_idx[0-3] COBO_idx[0-1]*/
	std::map<std::tuple<direction,int, int>, std::shared_ptr<Geometry_Strip>> stripArray; // key = {U,V,W}, section [0-2], STRIP_NUMBER [1-1024]
	std::vector<int> ASAD_N;       // pair=(COBO_idx, number of ASAD boards)
	const std::vector<int> FPN_chanId = { 11, 22, 45, 56 };     // FPN channels in AGET chips
	double pad_size;                 // in [mm]
	double pad_pitch;                // in [mm]
	double strip_pitch;              // in [mm]
	TVector2 reference_point;        // XY offset in [mm] of the REFERENCE POINT used to define the geometry
	std::map<direction, TVector2> strip_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending pad numbers of the strip
	std::map<direction, TVector2> pitch_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending strip numbers 
	double vdrift;                   // electron drift velocity in [cm / micosecond]
	double sampling_rate;            // electronics sampling rate in [MHz]
	double trigger_delay;            // delay in [microseconds] of the external "t0" trigger signal (for accelerator beam) 
	double drift_zmin;               // lower drift cage acceptance limit along Z-axis [mm] (closest to readout PCB)
	double drift_zmax;               // upper drift cage acceptance limit along Z-axis [mm] (farthest from readout PCB)
	std::shared_ptr<TH2Poly> tp;                     // for internal storage of arbitrary strip shapes
	int grid_nx = 25;                     // partition size of TH2Poly in X-dir
	int grid_ny = 25;                     // partition size of TH2Poly in Y-dir
	std::map<int, std::shared_ptr<Geometry_Strip>> fStripMap; // maps TH2Poly bin to a given StripTPC object  [TH2Poly bin index [1..1024]] 

	// Setter methods 

	bool Load(std::string fname);                 // loads geometry from TXT config file
	bool InitTH2Poly();                           // define bins for the underlying TH2Poly histogram

	void SetTH2PolyStrip(int ibin, std::shared_ptr<Geometry_Strip> s);  // maps TH2Poly bin to a given StripTPC object

	GeometryTPC(std::string fname);

public:
	void Debug();

	void SetTH2PolyPartition(int nx, int ny); // change cartesian binning of the underlying TH2Poly
	inline int GetTH2PolyPartitionX() { return grid_nx; }
	inline int GetTH2PolyPartitionY() { return grid_ny; }

	// Getter methods

	inline std::shared_ptr<TH2Poly> GetTH2Poly() { return tp; }   // returns pointer to the underlying TH2Poly
	std::shared_ptr<Geometry_Strip> GetTH2PolyStrip(int ibin);          // returns pointer to StripTPC object corresponding to TH2Poly bin 

	// Getter methods  
	int GetDirNstrips(direction dir) const;

	inline int GetAgetNchips() { return AGET_Nchips; }
	inline int GetAgetNchannels() { return AGET_Nchan; }
	inline int GetAgetNchannels_raw() { return AGET_Nchan_raw; }
	inline int GetAgetNchannels_fpn() { return AGET_Nchan_fpn; }
	inline int GetAgetNtimecells() { return AGET_Ntimecells; }
	inline int GetAsadNboards() { return ASAD_Nboards; }
	inline int GetAsadNboards(int COBO_idx) { return (COBO_idx < ASAD_N.size() ? ASAD_N[COBO_idx] : 0); }
	inline int GetCoboNboards() { return COBO_N; }
	inline double GetVdrift() { return vdrift; }
	inline double GetDriftCageZmin() { return drift_zmin; };
	inline double GetDriftCageZmax() { return drift_zmax; };
	inline double GetSamplingRate() { return sampling_rate; };
	inline double GetTriggerDelay() { return trigger_delay; };

	double Cartesian2posUVW(double x, double y, direction dir); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of a given (X,Y) point on the strip pitch axis for a given direction
	double Cartesian2posUVW(TVector2 pos, direction dir); // [mm] (signed) distance of projection of (X=0, Y=0) point from projection of a given (X,Y) point on the strip pitch axis for a given direction

	std::string GetDirName(direction dir);

	std::shared_ptr<Geometry_Strip> GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx) const;         // valid range [0-1][0-3][0-3][0-63]
	std::shared_ptr<Geometry_Strip> GetStripByDir(direction dir, int num) const;                                                   // valid range [0-2][1-1024]
	std::shared_ptr<Geometry_Strip> GetStripByDir(direction dir, int section, int num) const;                                                   // valid range [0-2][0-2][1-1024]
	std::shared_ptr<Geometry_Strip> GetStripByGlobal(int global_channel_idx);                                          // valid range [0-1023]

	std::vector<std::shared_ptr<Geometry_Strip>> GetStrips() const;

	// various helper functions for calculating local/global normal/raw channel index
	int Aget_normal2raw(int channel_idx);                      // valid range [0-63]
	int Aget_fpn2raw(int FPN_idx);                             // valid range [0-3]

	int Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx);   // valid range [0-1][0-3][0-3][0-63]

	bool GetCrossPoint(std::shared_ptr<Geometry_Strip> strip1, std::shared_ptr<Geometry_Strip> strip2, TVector2& point);
	bool MatchCrossPoint(std::array<int, 3> strip_nums, double radius, TVector2& point);

	inline double GetPadPitch() { return pad_pitch; } // [mm]
	inline double GetStripPitch() { return strip_pitch; } // [mm]
	inline TVector2 GetReferencePoint() { return reference_point; } // XY ([mm],[mm])
	TVector2 GetStripPitchVector(direction dir); // XY ([mm],[mm])

	double Strip2posUVW(direction dir, int section, int number); //legacy for section=0, [mm] (signed) distance of projection of (X=0, Y=0) point from projection of the central line of the (existing) strip on the strip pitch axis for a given direction
	double Strip2posUVW(direction dir, int number); // [mm] (signed) distance of direction of (X=0, Y=0) point from direction of the central line of the (existing) strip on the strip pitch axis for a given direction
	double Strip2posUVW(std::shared_ptr<Geometry_Strip> strip); // [mm] (signed) distance of direction of (X=0, Y=0) point from direction of the central line of the (existing) strip on the strip pitch axis for a strip given direction

	double Timecell2pos(double position_in_cells); // [mm] output: position along Z-axis

	std::tuple<double, double, double, double> rangeXY(); //min/max X Y cartesian coordinates covered by strips in any direction

	friend GeometryTPC& Geometry(std::string fname);
};

#include "Geometry_Strip.h"

#endif
