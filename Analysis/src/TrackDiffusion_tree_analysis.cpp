#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TGraph.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/TrackDiffusion_tree_analysis.h"
#include "TPCReco/TrackDiffusion_tree_dataFormat.h"

#include "TPCReco/colorText.h"
#include "TPCReco/UtilsMath.h"

///////////////////////////////
///////////////////////////////
TrackDiffusion_tree_analysis::TrackDiffusion_tree_analysis(const std::shared_ptr<GeometryTPC> aGeometryPtr,
							   const TrackDiffusionConfig &aConfig,
							   const std::string aOutputFileName){
  setGeometry(aGeometryPtr);
  myConfig = aConfig;
  myOutputFileName = aOutputFileName;
  initialize();
}
///////////////////////////////
///////////////////////////////
TrackDiffusion_tree_analysis::~TrackDiffusion_tree_analysis(){
  finalize();
}
///////////////////////////////
///////////////////////////////
void TrackDiffusion_tree_analysis::initialize(){

  std::string treeName = "TrackDiffusion";
  myOutputFilePtr = std::make_shared<TFile>(myOutputFileName.c_str(),"RECREATE");
  if(!myOutputFilePtr) {
    std::cout<<KRED<<"TrackDiffusion_tree_analysis::open: Cannot create new ROOT file: "<<RST<<myOutputFileName
  	     <<std::endl;
    return;
  }
  myOutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  myOutputTreePtr->Branch("data", &event_rawdiffusion);
  myOutputTreePtr->SetDirectory(myOutputFilePtr.get());
  ////// DEBUG
  // std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << myOutputTreePtr->GetCurrentFile() << std::endl;
  ////// DEBUG
}
///////////////////////////////
///////////////////////////////
void TrackDiffusion_tree_analysis::finalize(){

  if(!myOutputFilePtr){
    std::cout<<KRED<<"TrackDiffusion_tree_analysis::finalize: "<<RST
  	     <<" pointer to output file not set!"
  	     <<std::endl;
    return;
  }
  myOutputFilePtr->Write("", TObject::kOverwrite);
  ////// DEBUG
  // std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << myOutputTreePtr->GetCurrentFile() << std::endl;
  ////// DEBUG
}
///////////////////////////////
///////////////////////////////
//void TrackDiffusion_tree_analysis::fillTree(const std::shared_ptr<EventTPC> aEventTPC, const std::shared_ptr<Track3D> aTrack){
void TrackDiffusion_tree_analysis::fillTree(const std::shared_ptr<EventTPC> aEventTPC, Track3D *aTrack){

  clear();

  // sanity checks
  if(!myOutputTreePtr){
    std::cout<<KRED<<"TrackDiffusion_tree_analysis::fillTree"<<RST
	     <<" pointer to output tree not set!"
	     <<std::endl;
    return;
  }
  if(!aTrack || aTrack->getSegments().size()==0) {
    std::cout<<KRED<<"TrackDiffusion_tree_analysis::fillTree"<<RST
	     <<" pointer to Track3D not set or empty TrackSegment3D collection in Track3D!"
	     <<std::endl;
    return;
  }

  filter_type filterType = filter_type::none;
  if(myConfig.clusterEnable) { // CLUSTER
    filterType = filter_type::threshold;
    boost::property_tree::ptree config;
    config.put("recoClusterThreshold", myConfig.clusterThreshold);
    config.put("recoClusterDeltaStrips",myConfig.clusterDeltaStrips);
    config.put("recoClusterDeltaTimeCells",myConfig.clusterDeltaTimeCells);  
    aEventTPC->setHitFilterConfig(filterType, config);
  }
  
  // MAKE A CLUSTER AND EXTRACT 2D PROJECTIONS IN MM
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir] = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
    ////// DEBUG
    // std::cout << __FUNCTION__ << ": 2D strip vs time:  dir=" << strip_dir << ", ptr=" << (histosInMM[strip_dir]).get()
    // 	      << ", entries=" << (histosInMM[strip_dir] ? histosInMM[strip_dir]->GetEntries() : 0)  << std::endl;
    ////// DEBUG
  }
  std::shared_ptr<TH1D> histoTimeProjInMM;
  histoTimeProjInMM = aEventTPC->get1DProjection(definitions::projection_type::DIR_TIME, filterType, scale_type::mm);
  ////// DEBUG
  // std::cout << __FUNCTION__ << ": 1D time:  ptr=" << histoTimeProjInMM.get()
  // 	    << ", entries=" << (histoTimeProjInMM ? histoTimeProjInMM->GetEntries() : 0) << std::endl;
  ////// DEBUG

  // COMPUTE DIFFUSION PROPERTIES PER STRIP DIRECTION PER TRACK
  auto isOK=false;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    auto aColl = aTrack->getSegments();
    TrackDiffusionCropList aList = getCropList(aColl, strip_dir);
    isOK |= processCropList(aList, aColl, histosInMM[strip_dir], strip_dir); // loop over histogram hits and fill event_rawdiffusion
  }

  // FILL EVENT INFO
  event_rawdiffusion.runId=(aEventTPC ? aEventTPC->GetEventInfo().GetRunId() : -1); // run number
  event_rawdiffusion.eventId=(aEventTPC ? aEventTPC->GetEventInfo().GetEventId() : -1); // event number

  // FILL CLUSTERING INPUT PARAMETERS
  event_rawdiffusion.clusterFlag = myConfig.clusterEnable; // is clustering enabled?
  event_rawdiffusion.clusterThr = myConfig.clusterThreshold; // clustering threshold in ADC units for seed hits
  event_rawdiffusion.clusterDeltaStrips = myConfig.clusterDeltaStrips; // clustering envelope size in +/- strip units around seed hits
  event_rawdiffusion.clusterDeltaTimeCells = myConfig.clusterDeltaTimeCells; // clustering size in +/- time cell units around seed hits

  // FILL RECO TRACK DATA
  auto aColl = aTrack->getSegments();
  auto aSeg = aColl.front();
  event_rawdiffusion.ntracks = aColl.size();
  event_rawdiffusion.pid = aSeg.getPID();
  event_rawdiffusion.vertexPos = aSeg.getStart();
  event_rawdiffusion.endPos = aSeg.getEnd();;
  event_rawdiffusion.length = aSeg.getLength();
  event_rawdiffusion.phiDET = aSeg.getTangent().Phi(); // [rad]
  event_rawdiffusion.cosThetaDET = aSeg.getTangent().CosTheta(); // [-1, 1]

  // UPDATE MEAN AND RMS
  updateMeanRms();

  // UPDATE TREE
  myOutputTreePtr->Fill();

  // discard U/V/W/TIME projectons that are not needed anymore
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir].reset();
  }
  histoTimeProjInMM.reset();

}
///////////////////////////////
///////////////////////////////
void TrackDiffusion_tree_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  myGeometryPtr = aGeometryPtr;
  if(!myGeometryPtr) {
    std::cout<<KRED<<"TrackDiffusion_tree_analysis::setGeometry: "<<RST
	     <<" pointer to TPC geometry not set!"
	     <<std::endl;
    exit(-1);
  }
}
///////////////////////////////
///////////////////////////////
TrackDiffusionCropList TrackDiffusion_tree_analysis::getCropList(TrackSegment3DCollection &aColl, int dir) {
  TrackDiffusionCropList aList;

  // sanity checks
  if(aColl.size()==0) return aList;
  if(dir<definitions::projection_type::DIR_U || dir>definitions::projection_type::DIR_W) return aList;
  if(aColl.front().getGeometry()!=myGeometryPtr) return aList;

  for(auto &aSeg : aColl) {
    TGraph gr;
    const auto vtx=aSeg.getStart();
    const auto endpoint=aSeg.getEnd();
    const auto geo=aSeg.getGeometry();
    auto err=false;
    double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), dir, err); // strip position [mm] for a given XY_DET position
    double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), dir, err); // strip position [mm] for a given XY_DET position
    if(!err) {
      double zstart_pos=vtx.Z(); // drift time [mm]
      double zend_pos=endpoint.Z(); // drift time [mm]
      const auto radius=myConfig.trackDistanceMM; // mm
      
      auto offset=TVector2(zstart_pos, sstart_pos); // position
      auto tangent=TVector2(zend_pos-zstart_pos, send_pos-sstart_pos); // direction
      auto p=offset + myConfig.trackFractionStart*tangent;

      // 1st corner point
      p += radius*(tangent.Rotate(TMath::Pi()*0.5).Unit());
      gr.SetPoint(gr.GetN(), p.X(), p.Y());

      // 2nd corner point
      p += 2*radius*(tangent.Rotate(-TMath::Pi()*0.5).Unit());
      gr.SetPoint(gr.GetN(), p.X(), p.Y());

      // 3rd corner point
      p += (1-myConfig.trackFractionEnd-myConfig.trackFractionStart)*tangent;
      gr.SetPoint(gr.GetN(), p.X(), p.Y());

      // 4th corner point
      p += 2*radius*(tangent.Rotate(TMath::Pi()*0.5).Unit());
      gr.SetPoint(gr.GetN(), p.X(), p.Y());

      // repeat 1st corner point to close the loop
      p -= (1-myConfig.trackFractionEnd-myConfig.trackFractionStart)*tangent;
      gr.SetPoint(gr.GetN(), p.X(), p.Y());
    }
    aList.push_back(gr);
  }
  return aList;
}
///////////////////////////////
///////////////////////////////
bool TrackDiffusion_tree_analysis::verifyCropList(TrackDiffusionCropList &aList){
  for(auto icut1=0U; icut1<aList.size(); icut1++) {
    for(auto icut2=icut1+1; icut2<aList.size(); icut2++) {
      auto cut1=aList[icut1];
      auto cut2=aList[icut2];
      if(edgesInside(cut1, cut2) || edgesInside(cut2, cut1) || edgesIntersect(cut1, cut2)) return false;
    }
  }
  return true;
}
///////////////////////////////
///////////////////////////////
bool TrackDiffusion_tree_analysis::processCropList(TrackDiffusionCropList &aList, TrackSegment3DCollection &aColl, std::shared_ptr<TH2D> aHist, int dir) {
  
  // sanity check
  if(!verifyCropList(aList)) {
    std::cout << __FUNCTION__ << ": Regions of interest overlap for some tracks! (ntracks=" << aColl.size() << ")" << std::endl;
    return false;
  }

  const auto aSeg=aColl.front();
  const auto vtx=aSeg.getStart();
  const auto endpoint=aSeg.getEnd();
  const auto geo=aSeg.getGeometry();
  auto err=false;
  double sstart_pos=geo->Cartesian2posUVW(vtx.X(), vtx.Y(), dir, err); // strip position [mm] for a given XY_DET position
  double send_pos=geo->Cartesian2posUVW(endpoint.X(), endpoint.Y(), dir, err); // strip position [mm] for a given XY_DET position
  double zstart_pos=vtx.Z(); // drift time [mm]
  double zend_pos=endpoint.Z(); // drift time [mm]
  auto offset=TVector2(zstart_pos, sstart_pos); // vertex position
  auto tangent=TVector2(zend_pos-zstart_pos, send_pos-sstart_pos); // track direction

  for(auto iBinX=1; iBinX <= aHist->GetNbinsX(); iBinX++) {
    for(auto iBinY=1; iBinY <= aHist->GetNbinsY(); iBinY++) {
      double charge=0.0, err=0.0;
      if((err=aHist->GetBinError(iBinX, iBinY)) || (charge=aHist->GetBinContent(iBinX, iBinY))) {
	auto gr=aList.front(); // ROI corresponding to the leading track
	double x, y;
	if(gr.IsInside((x=aHist->GetXaxis()->GetBinCenter(iBinX)), (y=aHist->GetYaxis()->GetBinCenter(iBinY)))) {
	  double dist = Utils::signedDistancePointLine(offset, tangent, {x,y});
	  myHitMap[dir].push_back({dist, charge});
	  ////// DEBUG
	  //	  std::cout << __FUNCTION__ << ": dir=" << dir << ": hit distance=" << dist << ", hit charge=" << charge << std::endl;
	  ////// DEBUG
	}
      }
    }
  }
  return true;
}
///////////////////////////////
///////////////////////////////
void TrackDiffusion_tree_analysis::clear() {
  event_rawdiffusion=Event_rawdiffusion(); // revert to default values
  myHitMap.clear();
}
///////////////////////////////
///////////////////////////////
//
// returns TRUE when some corners of a closed-loop TGraph g1 (test) are inside a closed-loop TGraph g2 (reference)
bool TrackDiffusion_tree_analysis::edgesInside(TGraph &g1, TGraph &g2) {
  if(g2.GetN()<3 || g1.GetN()==0) return false;
  for(auto ipoint=0; ipoint<g1.GetN(); ipoint++) {
    double x, y;
    g1.GetPoint(ipoint, x, y);
    if(g2.IsInside(x, y)) {
      std::cout << __FUNCTION__ << ": G1 polygon corner (i="<<ipoint<<", x="<<x<<", y="<<y<<") is inside G2 polygon" << std::endl;
      return true;
    }
  }
  return false;
}
///////////////////////////////
///////////////////////////////
//
// returns TRUE when a closed-loop TGraph g1 intersects with a closed-loop TGraph g2
bool TrackDiffusion_tree_analysis::edgesIntersect(TGraph &g1, TGraph &g2) {
  if(g1.GetN()<2) return false;
  for(auto ipoint=0; ipoint<g1.GetN(); ipoint++) {    
    double x1, y1, x2, y2;
    g1.GetPoint(ipoint, x1, y1);
    g1.GetPoint((ipoint+1)%g1.GetN(), x2, y2);
    auto offset=TVector2(x1, y1);
    auto tangent=TVector2(x2-x1, y2-y1);
    std::vector<TVector2> point_vec;
    std::vector<int> index_vec;
    if(Utils::intersectionEdgePolygon(offset, tangent, g2, point_vec, index_vec)) {
      std::cout << __FUNCTION__ << ": G1 polygon edge="<<ipoint<<" intersects with G2 polygon edge="<<index_vec.front()<<std::endl;
      return true;
    }
  }
  return false;
}
///////////////////////////////
///////////////////////////////
void TrackDiffusion_tree_analysis::updateMeanRms() {

  // calculate MEAN and RMS per strip direction
  double totalSum_x=0.0, totalSum_x2=0.0, totalSum_w=0.0;
  for(auto dir=0; dir<3; dir++) {
    double sum_x=0.0, sum_x2=0.0, sum_w=0.0;
    for(auto &hit : myHitMap[dir]) {
      auto w = fabs(hit.q);
      if(w==0) continue;
      auto x = hit.x;
      sum_x2 += x * x * w;
      sum_x += x * w;
      sum_w += w;
    }
    double mean=0.0, mean2=0.0, rms=0.0;
    if(sum_w) {
      totalSum_x2 += sum_x2;
      totalSum_x += sum_x;
      totalSum_w += sum_w;
      mean = sum_x / sum_w;
      mean2 = sum_x2 / sum_w;
      rms = sqrt( mean2 - mean*mean );
      event_rawdiffusion.flagPerDir[dir]=true;
    }
    ////// DEBUG
    std::cout << __FUNCTION__ << ": dir=" << dir << ": mean=" << mean << ", rms=" << rms << std::endl;
    ////// DEBUG
    event_rawdiffusion.meanPerDir[dir]=mean;
    event_rawdiffusion.sigmaPerDir[dir]=rms;
  }
  // calculate total MEAN and RMS from all strip directions
  // NOTE: total RMS can be larger than RMS[dir] when MEAN[i] non-zero (e.g due to GET electronics peaking time effect)
  double totalMean=0.0, totalMean2=0.0, totalRms=0.0;
  if(totalSum_w) {
    totalMean = totalSum_x / totalSum_w;
    totalMean2 = totalSum_x2 / totalSum_w;
    totalRms = sqrt( totalMean2 - totalMean*totalMean );
    event_rawdiffusion.flagAll=true;
  }
  ////// DEBUG
  std::cout << __FUNCTION__ << ": mean=" << totalMean << ", rms=" << totalRms << std::endl;
  ////// DEBUG
  event_rawdiffusion.meanAll=totalMean;
  event_rawdiffusion.sigmaAll=totalRms;
}
///////////////////////////////
///////////////////////////////
