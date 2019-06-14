#include <iostream>
#include <thread>
#include <chrono>
#include <TApplication.h>
#include "UtilsTPC.h"

int main(int argc, char *argv[]) {

  TApplication app ("app",&argc,argv); // allows to open graphics windows

  std::cout << "main: argc=" << argc << std::endl;
  for(int i=0; i<argc; i++) {
    std::cout << "main: argv[" << i << "]=" << argv[i] << std::endl << std::flush;
  }

  plot_MCevent("resources/bkg_1e7gammas_8.3MeV__1mm_bins.root",
	      "Edep-hist", 
	      NULL, NULL, 
	      "results/plot_MCevent",
	      "resources/geometry_mini_eTPC_rot90deg.dat", 
	       false, false) ; // true);

  std::this_thread::sleep_for( std::chrono::seconds(3) ); // allows to exit properly TApplication

  return 0;
}
