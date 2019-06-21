#include <iostream>
#include "GeometryTPC.h"

int main(int argc, char *argv[]) {

  std::cout << "main: argc=" << argc << std::endl;
  for(int i=0; i<argc; i++) {
    std::cout << "main: argv[" << i << "]=" << argv[i] << std::endl << std::flush;
  }
  if(argc!=2) return -1;
  
  GeometryTPC *g = new GeometryTPC(argv[1]);

  bool err_flag=true;
  
  std::cout << "REF point: (X=" << g->GetReferencePoint().X() << " mm, Y=" << g->GetReferencePoint().Y() << " mm)" << std::endl;
  for(int idir=0; idir<3; idir++) {

    std::cout << "Strip DIRECTION: " << g->GetDirName(idir) << std::endl;
    std::vector<int> strip_index;
    strip_index.push_back(1);
    strip_index.push_back(g->GetDirNstrips(idir));

    std::cout << "Projection of reference point to direction " << g->GetDirName(idir) 
		<< " strip number: " << g->Cartesian2posUVW(g->GetReferencePoint(), idir, err_flag)
		<<std::endl;

    for(unsigned int istrip=0; istrip<strip_index.size(); istrip++) {      
      double pos = g->Strip2posUVW(idir, strip_index[istrip], err_flag);      
      std::cout << "Projection of strip " << g->GetDirName(idir) << strip_index[istrip]
		<< ": X_" << g->GetDirName(idir) << "=" << pos
		<< " mm (err=" << err_flag << ")" << std::endl;
    }
  }

  int idir = 0;
  TVector2 aPoint(106, 110);

  double directionScale = g->GetDirNstrips(idir)/std::abs(2*g->Strip2posUVW(idir, 1, err_flag));
  //aPoint -= g->GetReferencePoint();
  std::cout << "Projection of point : ";
  aPoint.Print();
  std::cout<<" to direction " << g->GetDirName(idir) 
	   << " strip number: " << g->Cartesian2posUVW(aPoint, idir, err_flag)*directionScale
	   <<std::endl;
  
  return 0;
}
