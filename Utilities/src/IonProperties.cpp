// Sources:
// [1] W.J. Huang et al., "The AME 2020 atomic mass evaluation (I). Evaluation of input data, and adjustment procedures",
//     Chinese Phys. C 45 (2021) 030002; DOI 10.1088/1674-1137/abddb0.
// [2] M.Wang et al., "The AME 2020 atomic mass evaluation (II). Tables, graphs and references",
//     Chinese Phys. C 45 (2021) 030003; DOI 10.1088/1674-1137/abddaf.
//

#include "IonProperties.h"
#include <stdexcept>
#include <string>

using namespace std::string_literals;

double IonProperties::atomicMassUnitMeV = 931.49410242; // [1]

IonProperties::IonProperties() {
    //                                {A, Z, mass [in u]}
    propMap[pid_type::PROTON]      = {1,  1, 1.00782503190};  //[2]
    propMap[pid_type::ALPHA]       = {4,  2, 4.00260325413};  //[2]
    propMap[pid_type::CARBON_12]   = {12, 6, 12};             //[2]
    propMap[pid_type::CARBON_13]   = {13, 6, 13.00335483534}; //[2]
    propMap[pid_type::CARBON_14]   = {14, 6, 14.003241989};   //[2]
    propMap[pid_type::NITROGEN_15] = {15, 7, 15.0001088983};  //[2]
    propMap[pid_type::OXYGEN_16]   = {16, 8, 15.9949146193};  //[2]
    propMap[pid_type::OXYGEN_17]   = {17, 8, 16.9991317560};  //[2]
    propMap[pid_type::OXYGEN_18]   = {18, 8, 17.9991596121};  //[2]

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
                "Ion \""s+ GetPidName(ionType)+
                "\" does not exist in IonProperties!"s);
    return propMap[ionType];
}

double IonProperties::GetMassU(pid_type ionType){
    return GetIonProperty(ionType).mass;
}

double IonProperties::GetMassMeV(pid_type ionType){
    return GetMassU(ionType)*atomicMassUnitMeV;
}

int IonProperties::GetZ(pid_type ionType){
    return GetIonProperty(ionType).Z;
}

int IonProperties::GetA(pid_type ionType){
    return GetIonProperty(ionType).A;
}