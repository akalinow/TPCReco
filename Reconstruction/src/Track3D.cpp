#include "Track3D.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D::Track3D(const TVector3 & aT, const TVector3 & aB, double aLenght, int aProj){
  
    myTangent = aT;
    myTangentUnit = aT.Unit();
    
    myBias = aB;

    double lambda = -myBias.X()/myTangent.X();
    myBiasAtX0 = myBias + lambda*myTangent;

    lambda = -myBias.Y()/myTangent.Y();
    myBiasAtY0 = myBias + lambda*myTangent;

    lambda = -myBias.Z()/myTangent.Z();
    myBiasAtZ0 = myBias + lambda*myTangent;

    myProjection = aProj;

    myLenght = aLenght;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////


