#include <iostream>
#include "IonProperties.h"

//Example showing usage of the IonProperties class

int main()
{
    //Get pointer to unique instance of the class:
    auto ionProp = IonProperties::GetInstance();

    //Select a generic ion:
    auto anIon=pid_type::ALPHA;

    //Collect all the info:
    auto mMeV = ionProp->GetAtomMass(anIon);;
    auto A=ionProp->GetA(anIon);
    auto Z=ionProp->GetZ(anIon);

    //print it out to the console
    std::cout<<"Properties of \'"<<GetPidName(anIon)<<"\' ion:\n";
    std::cout<<"\tm="<<mMeV<<" MeV\n\t";
    std::cout<<"A="<<A<<"\n\tZ="<<Z<<std::endl;
}