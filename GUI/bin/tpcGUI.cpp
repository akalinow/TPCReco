#include <cstdlib>
#include <iostream>
#include <vector>

#include <TROOT.h>
#include <TApplication.h>
#include <MainFrame.h>
#include <TH1F.h>

int main(int argc, char **argv){

  std::string configFileName;
  if(argc<1){
     std::cout<<" Usage: tpcGUI config.json"<<std::endl;
    return 0;
   }
  else {
    configFileName.append(argv[1]);
    std::cout<<"Using configFileName = "<<configFileName<<std::endl;
  }

   TApplication theApp("App", &argc, argv);
   MainFrame mainWindow(gClient->GetRoot(),0, 0);
   theApp.Run();

   return 0;
}
