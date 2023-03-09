#ifndef _IonRangeCalculator_H_
#define _IonRangeCalculator_H_

#include <string>
#include <vector>
#include <map>
#include <tuple>

#include <TH1D.h>
#include <TGraph.h>
#include "TPCReco/CommonDefinitions.h"

class IonRangeCalculator{

 public:

  IonRangeCalculator(gas_mixture_type gas=CO2, double p_mbar=250.0, double T_Kelvin=293.15, bool debug_flag=false); // default gas: CO2, 250 mbar, 20C

  void setGasMixture(gas_mixture_type gas); // set current GAS index

  void setGasPressure(double p_mbar); // set current gas pressure [mbar]

  void setGasTemperature(double T_Kelvin); // set current gas temperature [K]

  void setGasConditions(gas_mixture_type gas, double p_mbar, double T_Kelvin); // input: GAS index, pressure [mbar], temperature [K]

  gas_mixture_type getGasMixture(); // get current GAS index

  double getGasPressure(); // get current gas pressure [mbar]

  double getGasTemperature(); // get current gas temperature [K]

  std::tuple<gas_mixture_type, double, double> getGasConditions(); // get current set of: GAS index, pressure [mbar], temperature [K]

  double getIonRangeMM(pid_type ion, double E_MeV); // interpolated result in [mm] for the current {gas, p, T}

  double getIonEnergyMeV(pid_type ion, double range_mm); // interpolated result in [MeV] for the current {gas, p, T}

  double getIonMassMeV(pid_type ion); // particle or isotope mass in [MeV/c^2]

  TGraph getIonBraggCurveMeVPerMM(pid_type ion, double E_MeV, int Npoints=1000); // dE/dx curve in [MeV/mm] for the current {gas, p, T}

  double getIonBraggCurveIntegralMeV(pid_type ion, double E_MeV, int Npoints=1000); // integral of dE/dx curve for the current {gas, p, T}
  
  bool IsOK(); // check if there is at least one valid range/energy curve

  inline void setDebug(bool flag) { _debug=flag; }
    
 private:

  std::map<std::tuple<gas_mixture_type,pid_type>, TGraph*> refGasRangeCurveMap;       // reference Range(E_kin) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, TGraph*> refEnergyCurveMap;         // reference inverted Range(E_kin) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, TGraph*> refBraggCurveMap;          // reference dE/dx(x) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, double>  refGasRangePressureMap;    // reference pressure for Range(E_kin) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, double>  refGasRangeTemperatureMap; // reference temperature for Range(E_kin) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, double>  refBraggPressureMap;       // reference pressure for dE/dx(x) curve at given {gas, ion}
  std::map<std::tuple<gas_mixture_type,pid_type>, double>  refBraggTemperatureMap;    // reference temperature for dE/dx(x) curve at given {gas, ion}

  gas_mixture_type myGasMixture{gas_mixture_type::GAS_MIN};   // GAS index 
  std::map<pid_type, double> massTableMap; // particle or isotope mass table in [MeV/c^2]
  
  double myGasTemperature{0}; // Kelvins
  double myGasPressure{0};    // mbar
  
  void addIonRangeCurve(pid_type ion, gas_mixture_type gas, double p_mbar, double T_Kelvin, const char *datafile); // range(E_kin) corresponding to {gas, p, T}
  void addIonBraggCurve(pid_type ion, gas_mixture_type gas, double p_mbar, double T_Kelvin, const char *datafile); // dE/dx(x) corresponding to {gas, p, T}
  TGraph invertTGraph(const TGraph &aGraph) const;
  TGraph horizontalFlipTGraph(const TGraph &aGraph) const;
  void scaleTGraph(TGraph *aGraph, double factor);
  void zeroSuppressTGraph(TGraph *aGraph);

  bool _debug{false};

};

#endif
