#ifndef _IonProperties_H_
#define _IonProperties_H_

#include<memory>
#include<map>
#include "CommonDefinitions.h"


class IonProperties{
public:
    //delete copy constructor and assignment operator:
    IonProperties(const IonProperties&)=delete;
    IonProperties& operator=(const IonProperties&)=delete;
    static std::shared_ptr<IonProperties> GetInstance();
    double GetAtomMassU(pid_type ionType);
    double GetAtomMassMeV(pid_type ionType);
    int GetZ(pid_type ionType);
    int GetA(pid_type ionType);
    static double atomicMassUnitMeV;
private:
    struct SingleIonProperty{
        int A;
        int Z;
        double atomMassU;
    };
    IonProperties();
    std::map<pid_type, SingleIonProperty> propMap;
    SingleIonProperty GetIonProperty(pid_type ionType);

};

#endif //_IonProperties_H_
