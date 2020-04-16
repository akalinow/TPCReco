#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__
#include <cmath>
#include <iostream>
#include <tuple>
#include <map>
#include <vector>
#include <functional>


//class RV_Storage automatically manages function calls and stores already calculated results in memory
template <typename Ret_Type, typename... Func_Args>
class RV_Storage {
private:
	std::map<std::tuple<Func_Args...>, Ret_Type> ret_val_map;
	std::function<Ret_Type(Func_Args...)> func_ptr;
public:
	RV_Storage(std::function<Ret_Type(Func_Args...)>& f_ptr) : func_ptr(f_ptr) {};
	~RV_Storage() = default;
	decltype(func_ptr)& operator=(std::function < Ret_Type(Func_Args...)> f_ptr) {
		return (func_ptr = f_ptr);
	}
	Ret_Type operator()(Func_Args... args) {
		const auto it = ret_val_map.find(std::make_tuple( args... ));
		return (it != ret_val_map.end() ? it->second : (ret_val_map[std::make_tuple( args... )] = func_ptr(args...)));
	};
};

const double pi = 4 * std::atan(1);
const double deg_to_rad = pi / 180.0;
const bool is_debug = true;

enum class direction : unsigned {
	U = 0,    // U-direction channel index
	V = 1,    // V-direction channel index
	W = 2,    // W-direction channel index
	FPN_CH = 3 // FPN channel index
};

using position = std::tuple<direction, int, int, int>;
using position_reduced = std::tuple<direction, int, int>;
using position_by_time = std::tuple<int, direction, int, int>;
using position_by_time_reduced = std::tuple<int, direction, int>;

inline position_reduced to_reduced(position pos) {
	return position_reduced{ std::get<0>(pos), std::get<2>(pos), std::get<3>(pos) };
}

inline position to_normal(position_reduced pos) {
	return position{ std::get<0>(pos), 0, std::get<1>(pos), std::get<2>(pos) };
}

inline position_by_time_reduced to_reduced(position_by_time pos) {
	return position_by_time_reduced{ std::get<0>(pos), std::get<1>(pos), std::get<3>(pos) };
}

inline position_by_time to_normal(position_by_time_reduced pos) {
	return position_by_time{ std::get<0>(pos),std::get<1>(pos), 0,  std::get<2>(pos) };
}

inline position_by_time to_by_time(position pos) {
	return position_by_time{ std::get<3>(pos),std::get<0>(pos),std::get<1>(pos), std::get<2>(pos) };
}

inline position_by_time_reduced to_by_time(position_reduced pos) {
	return position_by_time_reduced{ std::get<2>(pos),std::get<0>(pos),std::get<1>(pos) };
}

enum class projection {
	DIR_XY,     // 2D direction on XY plane
	DIR_XZ,    // 2D direction on XZ plane
	DIR_YZ,    // 2D direction on YZ plane
	DIR_3D    // 3D reconstruction
};

inline std::ostream& operator<<(std::ostream& str, direction proj) {
	return (str << int(proj));
}

const auto dirs = std::vector<direction>{ direction::U,direction::V,direction::W };

const auto section_min = 0;

const auto section_max = 3;

const std::vector<int> section_indices = { 0,1,2 };

//#### Angles of U/V/W unit vectors wrt X-axis [deg]
//#ANGLES: 90.0 -30.0 30.0
const std::map<direction, const double> phiPitchDirection = { {direction::U, pi}, {direction::V, (-pi / 6.0 + pi / 2.0)}, {direction::W, (pi / 6.0 - pi / 2.0)} };

inline std::string filename_string(std::string path_str) {
	return path_str.substr(path_str.rfind("/") + 1, path_str.size() - path_str.rfind("/") - 1);
}

#define _endl_ " (" << filename_string(__FILE__) << "; " << __LINE__ << "; " << __FUNCTION__ << ")" << std::endl<char, std::char_traits<char>>
#define checkpoint std::cout << "checkpoint" << _endl_

const std::map<direction, std::string> dir_name = { {direction::U,"U"}, {direction::V,"V"}, {direction::W,"W"} };

class TH2D;
class TH3D;
using Reconstr_hist = std::pair<std::shared_ptr<TH2D>, std::shared_ptr<TH3D>>;

constexpr auto EventCharges_DEFAULT_RECO_METHOD = 1;  // 0 = equal charge division along the strip;
										// 1 = weighted charge division from complementary strip directions
constexpr auto EventCharges_DEFAULT_STRIP_REBIN = 2;  // number of strips to rebin [1-1024] ;
constexpr auto EventCharges_DEFAULT_TIME_REBIN = 5;  // number of time cells to rebin [1-512];

template < std::size_t I = 0, typename...Tp >
inline void sumTuples(const std::tuple < Tp...>& t1, const std::tuple < Tp...>& t2, std::tuple < Tp...>& _result) noexcept {
	if  (I < sizeof...(Tp)) {
		std::get < I >(_result) = std::get < I >(t1) + std::get < I >(t2);
		sumTuples < I + 1, Tp...>(t1, t2, _result);
	}
}

template < typename...Tp >
inline std::tuple < Tp...> operator+(const std::tuple < Tp...>& t1, const std::tuple < Tp...>& t2) {
	std::tuple < Tp...> _result;
	sumTuples < 0, Tp...>(t1, t2, _result);
	return _result;
}

template < typename...Tp >
inline std::tuple < Tp...> operator+=(std::tuple < Tp...>& t1, const std::tuple < Tp...>& t2) {
	sumTuples < 0, Tp...>(t1, t2, t1);
	return t1;
}

#endif
