#include "TFile.h"
#include "TTree.h"

#include "HistoManager.h"
#include "TrackBuilder.h"
#include "EventSourceROOT.h"

#include <boost/property_tree/json_parser.hpp>

/*
range_alpha_in_CO2_250mbar.dat
Minimizer is Linear / Migrad
Chi2                      =      3.34975
NDf                       =           43
p0                        =      2.48331   +/-   0.111405    
p1                        =    0.0136052   +/-   0.000374387 
p2                        = -1.27003e-06   +/-   3.01603e-07 
p3                        =  7.58944e-10   +/-   8.33559e-11 
p4                        = -5.81673e-14   +/-   7.29298e-15 


range_C12_in_CO2_250mbar.dat
Minimizer is Linear / Migrad
Chi2                      =    0.0867014
NDf                       =           35
p0                        =     0.349971   +/-   0.0311672   
p1                        =    0.0117859   +/-   0.000172907 
p2                        = -6.49773e-06   +/-   2.51863e-07 
p3                        =  2.25571e-09   +/-   1.32453e-10 
p4                        = -2.91529e-13   +/-   2.25827e-14 

 */
//range [mm] Energy [MeV]
std::vector<double> params_alpha = {2.48331, 0.0136052, -1.27003e-06, 7.58944e-10, -5.81673e-14};
std::vector<double> params_C12 = {0.349971, 0.0117859, -6.49773e-06, 2.25571e-09, -2.91529e-13};
std::vector< std::vector<double> > params = {params_alpha,  params_C12};
//////////////////////////
double getKineticEnergyForRange(double range, int type){

  if(type>1) return 0.0;
  
  double result = 0.0;
  for(unsigned int iPower=0;iPower<params.size();++iPower){
    result += params[type].at(iPower)*std::pow(range, iPower);
  }
  return result; 
}
/////////////////////////
// Define some simple structures
typedef struct {Float_t length, energy, charge, cosTheta, phi, x0, y0, z0, x1, y1, z1;} TrackData;
/////////////////////////
int main(int argc, char *argv[]) {

  std::string geometryFileName = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat";
  std::string dataFileNamePrefix = "";
  //dataFileNamePrefix = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/EventTPC_2021-06-22T12:01:56.568";
  dataFileNamePrefix = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/EventTPC_2021-06-22T14:11:08.614";

  //dataFileNamePrefix = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210617_extTrg_CO2_250mbar_DT1470ET/EventTPC_2021-06-17T11:54:38.000";

  //dataFileNamePrefix = "/scratch/akalinow/ELITPC/data/calibration/2021-11-25T13/EventTPC_2021-11-25T13:53:16.129";
  //dataFileNamePrefix = "/scratch/akalinow/ELITPC/data/calibration/2021-11-25T13/EventRaw_2021-11-25T13:53:16.129";

  int index = dataFileNamePrefix.find("Event")+9;
  std::string timestamp = dataFileNamePrefix.substr(index, 23);
  std::string rootFileName = "TrackAnalysis_"+timestamp+ ".root";
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");

  // Define some simple structures
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  tree->Branch("track",&track_data,"length:energy:charge:cosTheta:phi:x0:y0:z0:x1:y1:z1");
  
  std::shared_ptr<EventSourceBase> myEventSource;
  myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
  
  HistoManager myHistoManager;
  TrackBuilder myTkBuilder;
  myHistoManager.setGeometry(myEventSource->getGeometry());
  myTkBuilder.setGeometry(myEventSource->getGeometry());

  for(int chunkId=0;chunkId<5;++chunkId){
    std::string suffix = "_"+std::to_string(chunkId);
    std::string dataFileName = dataFileNamePrefix+suffix+".root";
    if(dataFileName.find(".root")!=std::string::npos){
      myEventSource->loadDataFile(dataFileName);
      std::cout<<"File with "<<myEventSource->numberOfEntries()<<" frames loaded."<<std::endl;
    }
    else{
      std::cout<<"Wrong input file: "<<dataFileName<<std::endl;
      return -1;
    }

    //Event loop
    unsigned int nEntries = myEventSource->numberOfEntries();
    //nEntries = 1;
    for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
      myEventSource->loadFileEntry(iEntry);
      std::cout<<"EventID: "<<myEventSource->currentEventNumber()<<std::endl;
      if(myEventSource->getCurrentEvent()->GetOneCluster(35, 0, 0).GetNhits()>20000){
	std::cout<<"Noisy event - skipping."<<std::endl;
	continue;
      }

      std::cout<<*myEventSource->getCurrentEvent()<<std::endl;
      myTkBuilder.setEvent(myEventSource->getCurrentEvent());
      myTkBuilder.reconstruct();      
      myHistoManager.setEvent(myEventSource->getCurrentEvent());

      const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);      
      double length = aTrack3D.getLength();
      double charge = aTrack3D.getIntegratedCharge(length);
      double cosTheta = cos(aTrack3D.getSegments().front().getTangent().Theta());
      double phi = aTrack3D.getSegments().front().getTangent().Phi();
      const TVector3 & start = aTrack3D.getSegments().front().getStart();
      const TVector3 & end = aTrack3D.getSegments().front().getEnd();

      track_data.length = length;
      track_data.charge = charge;
      track_data.cosTheta = cosTheta;
      track_data.phi = phi;
      track_data.x0 = start.X();
      track_data.y0 = start.Y();
      track_data.z0 = start.Z();
      track_data.x1 = end.X();
      track_data.y1 = end.Y();
      track_data.z1 = end.Z();
      track_data.energy = getKineticEnergyForRange(length, 0);
      tree->Fill();
     
      //std::cout<<aTrack3D<<std::endl;      
      //std::cout<<"length: "<<length<<" charge: "<<charge<<std::endl;
    }
  }

  outputROOTFile.Write();
  return 0;
}
