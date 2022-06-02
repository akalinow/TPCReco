#include <iostream>

#include "TStopwatch.h"
#include "colorText.h"
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  aStopwatch.Stop();
  std::cout<<KBLU<<"Real time:       "<<RST<<aStopwatch.RealTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"CPU time:        "<<RST<<aStopwatch.CpuTime()<<" s"<<std::endl;

  return 0;
}
/////////////////////////////
////////////////////////////

