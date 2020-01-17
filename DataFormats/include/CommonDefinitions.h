#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__
#include <cmath>
#include <iostream>
#include <tuple>
#include <map>
const double pi = 4 * atan(1);
const double deg_to_rad = pi / 180.0;

enum class projection : unsigned {
		DIR_U=0,    // U-direction channel index
        DIR_V=1,    // V-direction channel index
        DIR_W=2,    // W-direction channel index
		DIR_XY=3,     // 2D projection on XY plane
		DIR_XZ=4,    // 2D projection on XZ plane
		DIR_YZ=5,    // 2D projection on YZ plane
		DIR_3D=6    // 3D reconstruction
};

inline std::ostream& operator<<(std::ostream& str, projection proj) {
	return (str << int(proj));
}

inline bool IsDIR_UVW(projection DIR_) {
	return DIR_ == projection::DIR_U || DIR_ == projection::DIR_V || DIR_ == projection::DIR_W;
}

const auto proj_vec_UVW = std::vector<projection>{ projection::DIR_U,projection::DIR_V,projection::DIR_W };
  
//#### Angles of U/V/W unit vectors wrt X-axis [deg]
//#ANGLES: 90.0 -30.0 30.0
const std::map<projection, const double> phiPitchDirection = { {projection::DIR_U, pi}, {projection::DIR_V, (-pi / 6.0 + pi / 2.0)}, {projection::DIR_U, (pi / 6.0 - pi / 2.0)} };

inline std::string filename_string(std::string path_str) {
	return path_str.substr(path_str.rfind("\\") + 1, path_str.size() - path_str.rfind("\\") - 1);
}

#define _endl_ " (" << filename_string(__FILE__) << "; " << __LINE__ << ")" << std::endl<char, std::char_traits<char>>
#define checkpoint std::cout << "checkpoint" << _endl_

class TH2D;
class TH3D;
using Reconstr_hist = std::pair<std::map<projection, std::shared_ptr<TH2D>>, std::shared_ptr<TH3D>>;


constexpr auto EVENTTPC_DEFAULT_RECO_METHOD = 1;  // 0 = equal charge division along the strip;
										// 1 = weighted charge division from complementary strip directions
constexpr auto EVENTTPC_DEFAULT_STRIP_REBIN = 2;  // number of strips to rebin [1-1024] ;
constexpr auto EVENTTPC_DEFAULT_TIME_REBIN = 5;  // number of time cells to rebin [1-512];

#endif
