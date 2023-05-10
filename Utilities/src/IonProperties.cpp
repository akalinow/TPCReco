// Sources:
// [1] W.J. Huang et al., "The AME 2020 atomic mass evaluation (I). Evaluation of input data, and adjustment procedures",
//     Chinese Phys. C 45 (2021) 030002; DOI 10.1088/1674-1137/abddb0.
// [2] M.Wang et al., "The AME 2020 atomic mass evaluation (II). Tables, graphs and references",
//     Chinese Phys. C 45 (2021) 030003; DOI 10.1088/1674-1137/abddaf.
//

#include "TPCReco/IonProperties.h"
#include <stdexcept>
#include <string>

using namespace std::string_literals;

double IonProperties::atomicMassUnitMeV = 931.49410242; // 1 u in MeV/c^2 [1]

IonProperties::IonProperties() {
    //                                {A, Z, atomMass [in u]}
    propMap[pid_type::PROTON]      = {1,  1, 1.00782503190};  // isotope Hydrogen-1  (Z=1, N=0, A=1)   [2]
    propMap[pid_type::ALPHA]       = {4,  2, 4.00260325413};  // isotope Helium-4    (Z=2, N=2, A=4)   [2]
    propMap[pid_type::CARBON_12]   = {12, 6, 12};             // isotope Carbon-12   (Z=6, N=6, A=12)  [2]
    propMap[pid_type::CARBON_13]   = {13, 6, 13.00335483534}; // isotope Carbon-13   (Z=6, N=7, A=13)  [2]
    propMap[pid_type::CARBON_14]   = {14, 6, 14.003241989};   // isotope Carbon-14   (Z=6, N=8, A=14)  [2]
    propMap[pid_type::NITROGEN_15] = {15, 7, 15.0001088983};  // isotope Nitrogen-15 (Z=7, N=8, A=15)  [2]
    propMap[pid_type::OXYGEN_16]   = {16, 8, 15.9949146193};  // isotope Oxygen-16   (Z=8, N=8, A=16)  [2]
    propMap[pid_type::OXYGEN_17]   = {17, 8, 16.9991317560};  // isotope Oxygen-17   (Z=8, N=9, A=17)  [2]
    propMap[pid_type::OXYGEN_18]   = {18, 8, 17.9991596121};  // isotope Oxygen-18   (Z=8, N=10, A=18) [2]

}

std::shared_ptr<IonProperties> IonProperties::GetInstance()
{
    static std::shared_ptr<IonProperties> p(new IonProperties);
    return p;
}

IonProperties::SingleIonProperty IonProperties::GetIonProperty(pid_type ionType)
{
    if(propMap.find(ionType)==propMap.end())
        throw std::out_of_range(
                "Ion \""s+ enumDict::GetPidName(ionType)+
                "\" does not exist in IonProperties!"s);
    return propMap[ionType];
}

double IonProperties::GetAtomMass(pid_type ionType){
    return GetIonProperty(ionType).atomMassU * atomicMassUnitMeV;
}

int IonProperties::GetZ(pid_type ionType){
    return GetIonProperty(ionType).Z;
}

int IonProperties::GetA(pid_type ionType){
    return GetIonProperty(ionType).A;
}