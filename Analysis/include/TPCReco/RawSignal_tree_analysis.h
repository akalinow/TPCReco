#ifndef _RawSignal_analysis_H_
#define _RawSignal_analysis_H_

#include <chrono>

#include <TFile.h>
#include <TTree.h>

#include "TPCReco/RawSignal_tree_dataFormat.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/GeometryTPC.h"

class GeometryTPC;

// helper class with cluster parameters
struct ClusterConfig {
  bool clusterEnable;
  float clusterThreshold;
  int clusterDeltaStrips;
  int clusterDeltaTimeCells;
};

class RawSignal_tree_analysis{

public:

  RawSignal_tree_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,
			  ClusterConfig &aClusterConfig,
			  std::string aOutputFileName="RawSignalTree.root"); // definition of LAB detector coordinates
  ~RawSignal_tree_analysis();
  
  void fillTree(std::shared_ptr<EventTPC> aEventTPC, bool & isFirst);//(eventraw::EventInfo *aEventInfo);

 private:

  ClusterConfig myClusterConfig;
  Event_rawsignal *event_rawsignal_ = new Event_rawsignal;
  std::string myOutputFileName;
  std::shared_ptr<TFile> myOutputFilePtr;
  std::shared_ptr<TTree> myOutputTreePtr;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  
  void initialize();
  void finalize();
  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
};

#endif
