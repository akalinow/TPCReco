#ifndef _IonRangeCalculator_H_
#define _IonRangeCalculator_H_

#include <string>
#include <vector>
#include <map>
#include <tuple>

#include "TH1D.h"
#include "TGraph.h"
#include "MultiKey.h"

class MultiKey2;

class IonRangeCalculator{

 public:

  enum pid_type{
    ALPHA=1,          // Helium 4
    CARBON_12,        // Carbon 12
    CARBON_14,        // Carbon 14
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

  IonRangeCalculator(gas_mixture_type gas=CO2, double p_mbar=250.0, double T_Kelvin=293.15); // default gas: CO2, 250 mbar, 20C

  void setGasMixture(gas_mixture_type gas); // set current GAS index

  void setGasPressure(double p_mbar); // set current gas pressure [mbar]

  void setGasTemperature(double T_Kelvin); // set current gas temperature [K]

  void setGasConditions(gas_mixture_type gas, double p_mbar, double T_Kelvin); // input: GAS index, pressure [mbar], temperature [K]

  IonRangeCalculator::gas_mixture_type getGasMixture(); // get current GAS index

  double getGasPressure(); // get current gas pressure [mbar]

  double getGasTemperature(); // get current gas temperature [K]

  std::tuple<gas_mixture_type, double, double> getGasConditions(); // get current set of: GAS index, pressure [mbar], temperature [K]

  double getIonRangeMM(pid_type ion, double E_MeV); // interpolated result in [mm] for the current {gas, p, T}

  double getIonEnergyMeV(pid_type ion, double range_mm); // interpolated result in [MeV] for the current {gas, p, T}

  bool IsOK(); // check if there is at least one valid range/energy curve
    
 private:

  std::map<MultiKey2, TGraph*> refGasRangeCurveMap;  // reference curve for given {gas, ion} pair
  std::map<MultiKey2, TGraph*> refEnergyCurveMap; // reference inverted curve for given {gas, ion} pair
  std::map<MultiKey2, double>  refGasPressureMap;    // reference pressure for given {gas, ion} pair
  std::map<MultiKey2, double>  refGasTemperatureMap; // reference temperature for given {gas, ion} pair

  IonRangeCalculator::gas_mixture_type myGasMixture{IonRangeCalculator::GAS_MIN};   // GAS index 
  double myGasTemperature{0}; // Kelvins
  double myGasPressure{0};    // mbar
  
  void addIonRangeCurve(pid_type ion, gas_mixture_type gas, double p_mbar, double T_Kelvin, const char *datafile); // range(E_kin) corresponding to {gas, p, T}

  TGraph invertTGraph(const TGraph &aGraph) const;

};

#endif