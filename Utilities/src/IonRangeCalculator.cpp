#include <iostream>
#include <tuple>
#include "IonRangeCalculator.h"

////////////////////////////////////////////////
////////////////////////////////////////////////
IonRangeCalculator::IonRangeCalculator(gas_mixture_type gas, double p_mbar, double T_Kelvin, bool debug_flag){ // input: GAS index, pressure [mbar], temperature [K]
  setDebug(debug_flag);

  // Initialize ion range curves obtained from SRIM simulations at reference temperature and pressure (T0=20 C, p0=250 mbar)
  // Data file columns: E_kin[MeV] Range[mm]
  addIonRangeCurve(pid_type::PROTON,    gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_proton_2MeV_CO2_250mbar.dat");
  addIonRangeCurve(pid_type::ALPHA,     gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_alpha_10MeV_CO2_250mbar.dat");
  addIonRangeCurve(pid_type::CARBON_12, gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_12C_5MeV_CO2_250mbar.dat");
  addIonRangeCurve(pid_type::CARBON_14, gas_mixture_type::CO2, 250.0, 273.15+20, "range_corr_thr_1keV_12C_5MeV_CO2_250mbar.dat");//TEST

  // Initialize ion Bragg curves obtained from SRIM simulations at reference temperature and pressure (T0=20 C, p0=190 mbar)
  // Data file columns: Depth[mm] dE/dx[keV/mm]
  //  addIonBraggCurve(pid_type::ALPHA,     gas_mixture_type::CO2, 190.0, 273.15+20, "dEdx_uncorr_alpha_10MeV_CO2_190mbar.dat");
  //  addIonBraggCurve(pid_type::CARBON_12, gas_mixture_type::CO2, 190.0, 273.15+20, "dEdx_uncorr_12C_5MeV_CO2_190mbar.dat");
  //  addIonBraggCurve(pid_type::CARBON_14, gas_mixture_type::CO2, 190.0, 273.15+20, "dEdx_uncorr_14C_5MeV_CO2_190mbar.dat");//TEST
  // Initialize ion Bragg curves obtained from SRIM simulations at reference temperature and pressure (T0=20 C, p0=250 mbar)
  // Data file columns: Depth[mm] dE/dx[keV/mm]
  addIonBraggCurve(pid_type::PROTON,    gas_mixture_type::CO2, 250.0, 273.15+20, "dEdx_corr_proton_2MeV_CO2_250mbar.dat");
  addIonBraggCurve(pid_type::ALPHA,     gas_mixture_type::CO2, 250.0, 273.15+20, "dEdx_corr_alpha_10MeV_CO2_250mbar.dat");
  addIonBraggCurve(pid_type::CARBON_12, gas_mixture_type::CO2, 250.0, 273.15+20, "dEdx_corr_12C_5MeV_CO2_250mbar.dat");
  addIonBraggCurve(pid_type::CARBON_14, gas_mixture_type::CO2, 250.0, 273.15+20, "dEdx_corr_12C_5MeV_CO2_250mbar.dat");//TEST

  // DEBUG
  if(_debug) {
    setGasConditions(gas, 250.0, 273.15+20);
    std::cout<<__FUNCTION__<<": ALPHA: dE/dx integral=" << getIonBraggCurveIntegralMeV(pid_type::ALPHA, 10.0) << " MeV" << std::endl;
    std::cout<<__FUNCTION__<<": C-12:  dE/dx integral=" << getIonBraggCurveIntegralMeV(pid_type::CARBON_12, 5.0) << " MeV" << std::endl;
  }
  // DEBUG

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

  // DEBUG
  if(_debug) {
    std::cout<<__FUNCTION__<<": non-scaled range="<<ref_range<<" mm, "
	     <<"T_ref="<<refGasRangeTemperatureMap[it->first]<<" K, "
	     <<"p_ref="<<refGasRangePressureMap[it->first]<<" mbar"<<std::endl;
  }
  // DEBUG

  return ref_range*(myGasTemperature/refGasRangeTemperatureMap[it->first])*(refGasRangePressureMap[it->first]/myGasPressure); // result in [mm]
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
  double ref_range=range_mm*(refGasRangeTemperatureMap[it->first]/myGasTemperature)*(myGasPressure/refGasRangePressureMap[it->first]); // mm

  // DEBUG
  if(_debug) {
    std::cout<<__FUNCTION__<<": non-scaled range="<<ref_range<<" mm, "
	     <<"T_ref="<<refGasRangeTemperatureMap[it->first]<<" K, "
	     <<"p_ref="<<refGasRangePressureMap[it->first]<<" mbar"<<std::endl;
  }
  // DEBUG

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
    refGasRangePressureMap[mkey2]=p_mbar; // reference pressure [mbar]
    refGasRangeTemperatureMap[mkey2]=T_Kelvin; // reference tempereature T[K]

    // DEBUG
    if(_debug) {
      std::cout<<__FUNCTION__<<": Gas index="<<std::get<0>(mkey2)<<", Ion index="<<std::get<1>(mkey2)<<", Npoints="<<refEnergyCurveMap[mkey2]->GetN()<<std::endl;
    }
    // DEBUG

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
TGraph IonRangeCalculator::horizontalFlipTGraph(const TGraph &aGraph) const{

  TGraph flippedGraph(aGraph);
  double xmin, xmax, ymin, ymax;
  flippedGraph.ComputeRange(xmin, ymin, xmax, ymax);
  auto xmid=0.5*(xmin+xmax);
  for(int iPoint=0;iPoint<aGraph.GetN();++iPoint){
    double x, y;
    aGraph.GetPoint(iPoint, x, y);
    flippedGraph.SetPoint(iPoint, xmid-(x-xmid), y);
  }
  return flippedGraph;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::scaleTGraph(TGraph *aGraph, double factor){
  if(!aGraph) return;
  double x, y;
  for(int iPoint=0;iPoint<aGraph->GetN();++iPoint){
    aGraph->GetPoint(iPoint, x, y);
    aGraph->SetPoint(iPoint, x, factor*y);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::zeroSuppressTGraph(TGraph *aGraph){
  // iteratively remove all EMPTY points (if any) except the very last one
  if(!aGraph) return;
  auto exit_loop=false;
  while(!exit_loop) {
    double x1,x2,y1,y2;
    aGraph->GetPoint(aGraph->GetN()-1, x2, y2);
    aGraph->GetPoint(aGraph->GetN()-2, x1, y1);
    if(y2==0 && y1==0) {
      aGraph->RemovePoint(aGraph->GetN()-1);
    } else {
      exit_loop=true;
    }
  };
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TGraph IonRangeCalculator::getIonBraggCurveMeVPerMM(pid_type ion, double E_MeV, int Npoints){

  // sanity checks
  if(Npoints<2) {
    std::cerr<<__FUNCTION__<<": ERROR: Requested Bragg curve with insufficient number of points for: gas index="<<myGasMixture<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  auto itB=refBraggCurveMap.find(MultiKey2((int)myGasMixture, (int)ion));
  if(itB==refBraggCurveMap.end()) {
    std::cerr<<__FUNCTION__<<": ERROR: Reference Bragg curve is missing for: gas index="<<myGasMixture<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  auto itR=refGasRangeCurveMap.find(MultiKey2((int)myGasMixture, (int)ion));
  if(itR==refGasRangeCurveMap.end()) {
    std::cerr<<__FUNCTION__<<": ERROR: Reference range curve is missing for: gas index="<<myGasMixture<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
  if(E_MeV<=0.0) {
    std::cerr<<__FUNCTION__<<": ERROR: Wrong energy E="<<E_MeV<<" MeV!"<<std::endl;
    exit(-1);
  }

  TGraph aGraph(Npoints);

  // multiplicative factor for rescaling current range to the reference {p_ref, T_ref} conditions
  const auto factor=(refBraggTemperatureMap[itB->first]/myGasTemperature)*(myGasPressure/refBraggPressureMap[itB->first]); // Bragg curve reference {p_ref, T_ref}
  auto range_mm=getIonRangeMM(ion, E_MeV); // current {p, T}
  double ref_xmax_mm, ref_y;
  itB->second->GetPoint(itB->second->GetN()-1, ref_xmax_mm, ref_y); // Bragg curve reference {p_ref, T_ref}

  // DEBUG
  if(_debug) {
    double xmin, xmax, ymin, ymax;
    itB->second->ComputeRange(xmin, ymin, xmax, ymax);
    std::cout<<__FUNCTION__<<": Bragg x-check: Xmax(last-point)[mm]="<<ref_xmax_mm<<", Xmax(ComputeRange)="<<xmax<<std::endl;
  }
  // DEBUG

  for(int ipoint=0; ipoint<Npoints; ipoint++){
    auto x_mm = range_mm*ipoint/(Npoints-1); // current {p, T}
    auto ref_x_mm = x_mm * factor; // reference Bragg curve {p_ref, T_ref}
    aGraph.SetPoint(ipoint, x_mm, itB->second->Eval(ref_xmax_mm-ref_x_mm) * factor ); // current {p, T}
  }
  aGraph=horizontalFlipTGraph(aGraph);
  aGraph.Sort();

  // DEBUG
  if(_debug) {
    std::cout<<__FUNCTION__<<": Gas index="<<getGasMixture()<<", Ion index="<<ion<<", Npoints="<<aGraph.GetN()<<std::endl;
    for(auto iPoint=0; iPoint<aGraph.GetN(); iPoint++) {
      double x, y;
      aGraph.GetPoint(iPoint, x, y);
      std::cout<<__FUNCTION__<<": point="<<iPoint<<"  x[mm]="<<x<<"  dE/dx[MeV/mm]="<<y<<std::endl;
    }
  }
  // DEBUG

  return aGraph;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double IonRangeCalculator::getIonBraggCurveIntegralMeV(pid_type ion, double E_MeV, int Npoints){ // integral of dE/dx curve for the current {gas, p, T}

  // assume that all dE/dx values are non-negative
  TGraph aGraph(getIonBraggCurveMeVPerMM(ion, E_MeV, Npoints));
  double xmin, xmax, ymin, ymax;
  aGraph.ComputeRange(xmin, ymin, xmax, ymax);
  aGraph.SetPoint(aGraph.GetN(), xmin, 0.0);
  aGraph.SetPoint(aGraph.GetN(), xmax, 0.0);
  aGraph.Sort();
  double area=aGraph.Integral();
  return area;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void IonRangeCalculator::addIonBraggCurve(pid_type ion, gas_mixture_type gas, double p_mbar, double T_Kelvin, const char *datafile){ // dE/dx(x) corresponding to {gas, p, T}

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
  auto it=refBraggCurveMap.find(mkey2);

  // add new map elements
  if(it==refBraggCurveMap.end()) {
    refBraggCurveMap[mkey2]=new TGraph(datafile, "%lg %lg"); // X=(position along the track [mm]), Y=(ionization energy loss dE/dx [keV/mm])
    if(refBraggCurveMap[mkey2]->GetN()<2) {
      std::cerr<<__FUNCTION__<<": ERROR: Wrong initialization of dE/dx TGraph using datafile="<<datafile<<"!"<<std::endl;
      exit(-1);
    }
    scaleTGraph(refBraggCurveMap[mkey2], 1e-3); // convert keV/mm to MeV/mm
    refBraggCurveMap[mkey2]->Sort(); // sort by distance along the track in ascending order
    double first_pos,first_val;
    refBraggCurveMap[mkey2]->GetPoint(0, first_pos, first_val);
    if(first_pos<0.0) {
	std::cerr<<__FUNCTION__<<": ERROR: Wrong initialization of dE/dx TGraph using datafile="<<datafile<<"!"<<std::endl;
	exit(-1);
    } else if(first_pos>0.0) {
      refBraggCurveMap[mkey2]->SetPoint(refBraggCurveMap[mkey2]->GetN(), 0.0, first_val);
      refBraggCurveMap[mkey2]->Sort(); // re-sort
    }
    zeroSuppressTGraph(refBraggCurveMap[mkey2]);
    refBraggPressureMap[mkey2]=p_mbar; // reference pressure [mbar]
    refBraggTemperatureMap[mkey2]=T_Kelvin; // reference tempereature T[K]

    // DEBUG
    if(_debug) {
      std::cout<<__FUNCTION__<<": Gas index="<<std::get<0>(mkey2)<<", Ion index="<<std::get<1>(mkey2)<<", Npoints="<<refBraggCurveMap[mkey2]->GetN()<<std::endl;
      for(auto iPoint=0; iPoint<refBraggCurveMap[mkey2]->GetN(); iPoint++) {
	double x, y;
	refBraggCurveMap[mkey2]->GetPoint(iPoint, x, y);
	std::cout<<__FUNCTION__<<": point="<<iPoint<<"  x[mm]="<<x<<"  dE/dx[MeV/mm]="<<y<<std::endl;
      }
    }
    // DEBUG

  } else {
    std::cerr<<__FUNCTION__<<": ERROR: Reference dE/dx curve already exists for: gas index="<<gas<<", ion="<<ion<<"!"<<std::endl;
    exit(-1);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
bool IonRangeCalculator::IsOK(){
  bool result=false;
  
  // check if there is at least one valid range/energy curve and p, T are valid
  if(refGasRangeCurveMap.size() && refBraggCurveMap.size() && myGasPressure>0 && myGasTemperature>0) result=true;
  return result;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
