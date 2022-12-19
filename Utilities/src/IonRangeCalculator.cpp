#include <iostream>
#include "IonRangeCalculator.h"

////////////////////////////////////////////////
////////////////////////////////////////////////
IonRangeCalculator::IonRangeCalculator(gas_mixture_type gas, double p_mbar, double T_Kelvin){ // input: GAS index, pressure [mbar], temperature [K]

  // Initialize ion range curves obtained from SRIM simulations at reference temperature and pressure (T0=20 C, p0=250 mbar)
  addIonRangeCurve(pid_type::ALPHA,     gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_alpha_10MeV_CO2_250mbar.dat");
  addIonRangeCurve(pid_type::CARBON_12, gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_12C_5MeV_CO2_250mbar.dat");
  addIonRangeCurve(pid_type::CARBON_14, gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_12C_5MeV_CO2_250mbar.dat");//TEST

  // Rescale ion range curves for specific gas, pressure and temperature.
  setGasConditions(gas, p_mbar, T_Kelvin);
  
  // Initialize masses of netural atoms.
  // Sources:
  // [1] W.J. Huang et al., "The AME 2020 atomic mass evaluation (I). Evaluation of input data, and adjustment procedures",
  //     Chinese Phys. C 45 (2021) 030002; DOI 10.1088/1674-1137/abddb0.
  // [2] M.Wang et al., "The AME 2020 atomic mass evaluation (II). Tables, graphs and references",
  //     Chinese Phys. C 45 (2021) 030003; DOI 10.1088/1674-1137/abddaf.
  //
  const double atomicMassUnit = 931.49410242;                            // 1u in MeV/c^2                         [1]
  massTableMap[pid_type::PROTON]      = 1.00782503190  * atomicMassUnit; // isotope Hydrogen-1  (Z=1, N=0, A=1)   [2]
  massTableMap[pid_type::ALPHA]       = 4.00260325413  * atomicMassUnit; // isotope Helium-4    (Z=2, N=2, A=4)   [2]
  massTableMap[pid_type::CARBON_12]   = 12             * atomicMassUnit; // isotope Carbon-12   (Z=6, N=6, A=12)  [2]
  massTableMap[pid_type::CARBON_13]   = 13.00335483534 * atomicMassUnit; // isotope Carbon-13   (Z=6, N=7, A=13)  [2]
  massTableMap[pid_type::CARBON_14]   = 14.003241989   * atomicMassUnit; // isotope Carbon-14   (Z=6, N=8, A=14)  [2]
  massTableMap[pid_type::NITROGEN_15] = 15.0001088983  * atomicMassUnit; // isotope Nitrogen-15 (Z=7, N=8, A=15)  [2]
  massTableMap[pid_type::OXYGEN_16]   = 15.9949146193  * atomicMassUnit; // isotope Oxygen-16   (Z=8, N=8, A=16)  [2]
  massTableMap[pid_type::OXYGEN_17]   = 16.9991317560  * atomicMassUnit; // isotope Oxygen-17   (Z=8, N=9, A=17)  [2]
  massTableMap[pid_type::OXYGEN_18]   = 17.9991596121  * atomicMassUnit; // isotope Oxygen-18   (Z=8, N=10, A=18) [2]
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::setGasMixture(gas_mixture_type gas){ // GAS index

  // check gas index
  if(gas<GAS_MIN || gas>GAS_MAX) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong gas mixture index="<<gas<<"!"<<std::endl;
    exit(-1);
  }
  
  // check that at least one range/energy curve exists for this gas index
  bool valid=false;
  for(auto ion=pid_type::PID_MIN; ion!=pid_type::PID_MAX; ion=pid_type(ion+1)) {
    if(refGasRangeCurveMap.find(MultiKey2((int)gas, (int)ion))!=refGasRangeCurveMap.end()) {
      valid=true;
      break;
    }
  }
  if(!valid){
    std::cerr<<__FUNCTION__<<": ERROR: Reference range/energy curves do not exist for gas mixture index="<<gas<<std::endl;
    exit(-1);
  }
  myGasMixture=gas;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::setGasPressure(double p_mbar){ // pressure [mbar]
  if(p_mbar>0.0) myGasPressure=p_mbar;
  else {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong gas pressure p="<<p_mbar<<" mbar!"<<std::endl;
    exit(-1);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::setGasTemperature(double T_Kelvin){ // temperature [K]
  if(T_Kelvin>0.0) myGasTemperature=T_Kelvin;
  else {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong gas temperature T="<<T_Kelvin<<" K!"<<std::endl;
    exit(-1);
  }
}

////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::setGasConditions(gas_mixture_type gas, double p_mbar, double T_Kelvin){ // input: GAS index, pressure [mbar], temperature [K]
  setGasMixture(gas);
  setGasPressure(p_mbar);
  setGasTemperature(T_Kelvin);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
gas_mixture_type IonRangeCalculator::getGasMixture() { return myGasMixture; } // GAS index

////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getGasPressure() { return myGasPressure; } // pressure [mbar]

////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getGasTemperature() { return myGasTemperature; } // temperature [K]

////////////////////////////////////////////////
////////////////////////////////////////////////
std::tuple<gas_mixture_type, double, double> IonRangeCalculator::getGasConditions(){ // output: GAS index, pressure [mbar], temperature [K]
  return std::tuple<gas_mixture_type, double, double>(myGasMixture, myGasPressure, myGasTemperature);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getIonRangeMM(pid_type ion, double E_MeV){ // interpolated result in [mm] for current {gas, p, T}

  // sanity checks
  auto it=refGasRangeCurveMap.find(MultiKey2((int)myGasMixture, (int)ion));
  if(it==refGasRangeCurveMap.end()) {
    std::cerr<<__FUNCTION__<<": ERROR: Reference range/energy curve is missing for: gas index="<<myGasMixture<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  if(E_MeV<0.0) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong energy="<<E_MeV<<" MeV!"<<std::endl;
    exit(-1);
  }
  
  // rescale output range to current {p, T} values assuming ideal gas pV=nRT formula
  double ref_range=(it->second)->Eval(E_MeV); // mm
  //  std::cout<<__FUNCTION__<<": non-scaled range="<<ref_range<<" mm, "
  //	   <<"T_ref="<<refGasTemperatureMap[it->first]<<" K, "
  //	   <<"p_ref="<<refGasPressureMap[it->first]<<" mbar"<<std::endl;
  return ref_range*(myGasTemperature/refGasTemperatureMap[it->first])*(refGasPressureMap[it->first]/myGasPressure); // result in [mm]
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getIonEnergyMeV(pid_type ion, double range_mm){ // interpolated result in [MeV] for current {gas, p, T}

  // sanity checks
  auto it=refEnergyCurveMap.find(MultiKey2((int)myGasMixture, (int)ion));
  if(it==refEnergyCurveMap.end()) {
    std::cerr<<__FUNCTION__<<": ERROR: Reference range/energy curve is missing for: gas index="<<myGasMixture<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  if(range_mm<0.0) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong range="<<range_mm<<" mm!"<<std::endl;
    exit(-1);
  }

  // rescale input range to reference {p_ref, T_ref} values assuming ideal gas pV=nRT formula
  double ref_range=range_mm*(refGasTemperatureMap[it->first]/myGasTemperature)*(myGasPressure/refGasPressureMap[it->first]); // mm
  //  std::cout<<__FUNCTION__<<": non-scaled range="<<ref_range<<" mm, "
  //	   <<"T_ref="<<refGasTemperatureMap[it->first]<<" K, "
  //	   <<"p_ref="<<refGasPressureMap[it->first]<<" mbar"<<std::endl;
  return (it->second)->Eval(ref_range); // result in [MeV]
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getIonMassMeV(pid_type ion){ // particle or isotope mass in [MeV/c^2]
  auto it=massTableMap.end();
  if((it=massTableMap.find(ion))==massTableMap.end()) return 0;
 else return it->second;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::addIonRangeCurve(pid_type ion, gas_mixture_type gas, double p_mbar, double T_Kelvin, const char *datafile){ // range(E_kin) corresponding to {gas, p, T}

  // sanity checks
  if(ion<PID_MIN || ion>PID_MAX) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong ion index="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  if(gas<GAS_MIN || gas>GAS_MAX) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong gas mixture index="<<gas<<"!"<<std::endl;
    exit(-1);
  }
  
  auto mkey2=MultiKey2((int)gas, (int)ion);
  auto it=refGasRangeCurveMap.find(mkey2);

  // add new map elements
  if(it==refGasRangeCurveMap.end()) {
    refGasRangeCurveMap[mkey2]=new TGraph(datafile, "%lg %lg");
    refGasRangeCurveMap[mkey2]->SetPoint(refGasRangeCurveMap[mkey2]->GetN(), 0.0, 0.0);
    refGasRangeCurveMap[mkey2]->Sort();
    if(refGasRangeCurveMap[mkey2]->GetN()<2) {
	std::cerr<<__FUNCTION__<<": ERROR: Wrong initialization of range vs energy TGraph using datafile="<<datafile<<"!"<<std::endl;
	exit(-1);
      }
    refEnergyCurveMap[mkey2]=(TGraph*)invertTGraph(*refGasRangeCurveMap[mkey2]).Clone();
    if(refEnergyCurveMap[mkey2]->GetN()<2) {
	std::cerr<<__FUNCTION__<<": ERROR: Wrong initialization of energy vs range TGraph using datafile="<<datafile<<"!"<<std::endl;
	exit(-1);
      }
    refGasPressureMap[mkey2]=p_mbar; // reference pressure [mbar]
    refGasTemperatureMap[mkey2]=T_Kelvin; // reference tempereature T[K]
    //    std::cout<<__FUNCTION__<<": Gas index="<<mkey2.key1<<", Ion index="<<mkey2.key2<<", Npoints="<<refEnergyCurveMap[mkey2]->GetN()<<std::endl;
    //    double E=1.0;
    //    double L=1.0;
    //    std::cout<<__FUNCTION__<<": Cross check E="<<E<<" MeV: interpolated range="<<refGasRangeCurveMap[mkey2]->Eval(E)<<" mm"<<std::endl;
    //    std::cout<<__FUNCTION__<<": Cross check L="<<L<<" mm: interpolated Ee="<<refEnergyCurveMap[mkey2]->Eval(L)<<" MeV"<<std::endl;
  } else {
    std::cerr<<__FUNCTION__<<": ERROR: Reference range/energy curve already exists for: gas index="<<gas<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TGraph IonRangeCalculator::invertTGraph(const TGraph &aGraph) const{

  TGraph invertedGraph(aGraph);

  double x, y;
  for(int iPoint=0;iPoint<aGraph.GetN();++iPoint){
    aGraph.GetPoint(iPoint, x, y);
    invertedGraph.SetPoint(iPoint, y, x);
    //    std::cout<<__FUNCTION__<<": point="<<iPoint<<"  x="<<x<<"  y="<<y<<std::endl;
  }
  return invertedGraph;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
bool IonRangeCalculator::IsOK(){
  bool result=false;
  
  // check if there is at least one valid range/energy curve and p, T are valid
  if(refGasRangeCurveMap.size() && myGasPressure>0 && myGasTemperature>0) result=true;
  return result;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
