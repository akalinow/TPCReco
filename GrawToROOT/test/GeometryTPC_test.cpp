#include <iostream>
#include "GeometryTPC.h"

int main(int argc, char *argv[]) {

  std::cout << "main: argc=" << argc << std::endl;
  for(int i=0; i<argc; i++) {
    std::cout << "main: argv[" << i << "]=" << argv[i] << std::endl << std::flush;
  }
  if(argc!=2) return -1;
  
  GeometryTPC *g = new GeometryTPC(argv[1]);

  std::cout << "REF point: (X=" << g->GetReferencePoint().X() << " mm, Y=" << g->GetReferencePoint().Y() << " mm)" << std::endl;
  for(int idir=0; idir<3; idir++) {

    std::cout << "Strip DIRECTION: " << g->GetDirName(idir) << std::endl;
    std::vector<int> strip_index;
    strip_index.push_back(1);
    strip_index.push_back(g->GetDirNstrips(idir));

    for(unsigned int istrip=0; istrip<strip_index.size(); istrip++) {
      bool err_flag=true;
      double pos = g->Strip2posUVW(idir, strip_index[istrip], err_flag);
      std::cout << "Projection of strip " << g->GetDirName(idir) << strip_index[istrip]
		<< ": X_" << g->GetDirName(idir) << "=" << pos
		<< " mm (err=" << err_flag << ")" << std::endl;
    }
  }
  
  return 0;
}
