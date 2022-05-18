#include "CoordinateConverter.h"

#include "colorText.h"

////////////////////////////////////////////
////////////////////////////////////////////
CoordinateConverter::CoordinateConverter(double aPhi, double aTheta, double aPsi){

  detToNominalBeamRotation.SetXEulerAngles(-M_PI/2.0, M_PI/2.0, 0.0);
  nominalToActualBeamRotation.SetXEulerAngles(aPhi, aTheta, aPsi);
  totalRotation = nominalToActualBeamRotation*detToNominalBeamRotation;
    
}
////////////////////////////////////////////
////////////////////////////////////////////
CoordinateConverter::~CoordinateConverter() {}
////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::detToBeam(const TVector3 & aVec) const{

  return totalRotation*aVec;
  
}
////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::detToBeam(double x, double y, double z) const{

  return totalRotation*TVector3(x,y,z);
  
}
////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::beamToDet(const TVector3 & aVec) const{

  return totalRotation.Inverse()*aVec;

}
////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::beamToDet(double x, double y, double z) const{

  return totalRotation.Inverse()*TVector3(x,y,z);
  
}
////////////////////////////////////////////
////////////////////////////////////////////
void CoordinateConverter::printRotation(std::ostream &out) const{

  TVector3 x(1,0,0);
  TVector3 y(0,1,0);
  TVector3 z(0,0,1);

  out<<KBLU<<"DETECTOR  --> BEAM"<<RST<<std::endl;
  out<<"("<<x.X()<<", "<<x.Y()<<", "<<x.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<detToBeam(x).X()<<", "<<detToBeam(x).Y()<<", "<<detToBeam(x).Z()<<")"
     <<std::endl;

  out<<"("<<y.X()<<", "<<y.Y()<<", "<<y.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<detToBeam(y).X()<<", "<<detToBeam(y).Y()<<", "<<detToBeam(y).Z()<<")"
     <<std::endl;

  out<<"("<<z.X()<<", "<<z.Y()<<", "<<z.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<detToBeam(z).X()<<", "<<detToBeam(z).Y()<<", "<<detToBeam(z).Z()<<")"
     <<std::endl;

  out<<KBLU<<"BEAM      --> DETECTOR"<<RST<<std::endl;
  out<<"("<<x.X()<<", "<<x.Y()<<", "<<x.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<beamToDet(x).X()<<", "<<beamToDet(x).Y()<<", "<<beamToDet(x).Z()<<")"
     <<std::endl;
  
  out<<"("<<y.X()<<", "<<y.Y()<<", "<<y.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<beamToDet(y).X()<<", "<<beamToDet(y).Y()<<", "<<beamToDet(y).Z()<<")"
     <<std::endl;
  
  out<<"("<<z.X()<<", "<<z.Y()<<", "<<z.Z()<<")"
     <<KBLU<<" --> "<<RST
     <<"("<<beamToDet(z).X()<<", "<<beamToDet(z).Y()<<", "<<beamToDet(z).Z()<<")"
     <<std::endl;
}
////////////////////////////////////////////
////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const CoordinateConverter &aConverter){

  aConverter.printRotation(out);
  return out;
  
}
////////////////////////////////////////////
////////////////////////////////////////////

