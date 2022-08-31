#ifndef _RawSignal_analysis_H_
#define _RawSignal_analysis_H_

//#include <string>
//#include <vector>
#include <chrono>

#include "TFile.h"
#include "TTree.h"

//#include "colorText.h"
#include "RawSignal_tree_dataFormat.h"
//#include "EventInfo.h"
#include "EventTPC.h"
#include "GeometryTPC.h"

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
			  ClusterConfig &aClusterConfig); // definition of LAB detector coordinates
  ~RawSignal_tree_analysis();
  
  void fillTree(std::shared_ptr<EventTPC> aEventTPC, bool & isFirst);//(eventraw::EventInfo *aEventInfo);

 private:

  ClusterConfig myClusterConfig;
  Event_rawsignal *event_rawsignal_ = new Event_rawsignal;
  std::shared_ptr<TFile> OutputFilePtr;
  std::shared_ptr<TTree> OutputTreePtr;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  
  void initialize();
  void finalize();
  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
  //  double getUnixTimestamp(time_t run_id, uint64_t elapsed_time_10ns); // creates absolute Unix timestamp with millisecond precision [s]
};

#endif
