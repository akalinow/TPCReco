#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__

#include <map>
#include <iostream>
#include <functional>
#include <memory>

enum projection{
		DIR_U=0,    // U-direction channel index
                DIR_V=1,    // V-direction channel index
                DIR_W=2,    // W-direction channel index
		DIR_XY=3,     // 2D projection on XY plane
		DIR_XZ=4,    // 2D projection on XZ plane
		DIR_YZ=5,    // 2D projection on YZ plane
		DIR_3D=6    // 3D reconstruction
};

enum pid_type{
	      UNKNOWN=0,
	      ALPHA=1,          // Helium 4
	      CARBON_12,        // Carbon 12
	      CARBON_14,        // Carbon 14
	      C12_ALPHA,        
	      HELIUM_4=ALPHA,   // alias
	      C_12=CARBON_12,   // alias
	      C_14=CARBON_14,   // alias
	      PID_MIN=ALPHA,    // alias
	      PID_MAX=CARBON_14 // alias
  };

enum gas_mixture_type{
		      CO2=1,            // Carbon dioxide
		      GAS_MIN=CO2,
		      GAS_MAX=CO2
};


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
		const auto it = ret_val_map.find({ args... });
		return (it != ret_val_map.end() ? it->second : (ret_val_map[{ args... }] = func_ptr(args...)));
	};
};

inline std::ostream& operator<<(std::ostream& str, projection proj) {
	return (str << int(proj));
}

inline std::string filename_string(std::string path_str) {
	return path_str.substr(path_str.rfind("/") + 1, path_str.size() - path_str.rfind("/") - 1);
}

#define _endl_ " (" << filename_string(__FILE__) << "; " << __LINE__ << ")" << std::endl<char, std::char_traits<char>>
#define checkpoint std::cout << "checkpoint" << _endl_

class TH2D;
class TH3D;
using Reconstr_hist = std::pair<std::shared_ptr<TH2D>, std::shared_ptr<TH3D>>;

template < std::size_t I = 0, typename...Tp >
inline void sumTuples(const std::tuple < Tp...>& t1, const std::tuple < Tp...>& t2, std::tuple < Tp...>& _result) noexcept {
	if (I < sizeof...(Tp)) {
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
