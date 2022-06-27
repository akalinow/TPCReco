#include <cstdlib>
#include <iostream>
#include <algorithm>

#include "TVector3.h"
#include "TVector2.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TProfile2D.h"
//#include "TObjArray.h"
//#include "TF1.h"
//#include "TTree.h"
#include "TFile.h"
//#include "TFitResult.h"
//#include "Math/Functor.h"

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"

#include "DotFinder.h"
#include "colorText.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
DotFinder::DotFinder() {
  myEvent = 0;
  myHistogramsInitialized = false;
  myHistograms.clear();
  myHistograms2D.clear();
  myHistogramsProf.clear();
  myHistogramsProf2D.clear();
  isFirstEvent_All = true;
  isFirstEvent_Dot = true;
  isDotEvent = false;
  myDot3D = TVector3(0., 0., 0.);
  myDotDeltaZ = 0.0;
  myEventCounter_All=0;
  myEventCounter_Dot=0;
  previousEventTime_All = -1;
  previousEventTime_Dot = -1;
  setCuts(DOTFINDER_DEFAULT_HIT_THR, /*DOTFINDER_DEFAULT_NSTRIPS, DOTFINDER_DEFAULT_NTIMECELLS,*/ DOTFINDER_DEFAULT_CHARGE_THR, DOTFINDER_DEFAULT_RADIUS);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
DotFinder::~DotFinder() {
  closeOutputStream();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::setEvent(std::shared_ptr<EventTPC> aEvent){
  setEvent(aEvent.get());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::setEvent(EventTPC* aEvent){
  myEvent = aEvent;
  // needs a valid EventTPC object with geometry pointer to initialize histogram ranges
  if(myEvent && !myHistogramsInitialized) initializeHistograms();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::setCuts(unsigned int aHitThr,
			//			unsigned int aMaxStripsPerDir,
			//			unsigned int aMaxTimeCellsPerDir,
			unsigned int aTotalChargeThr,
			double aMatchRadiusInMM){
  if(  myHitThr == aHitThr && 
       //       myMaxStripsPerDir == aMaxStripsPerDir &&
       //       myMaxTimeCellsPerDir == aMaxTimeCellsPerDir &&
       myTotalChargeThr == aTotalChargeThr &&
       myMatchRadiusInMM == aMatchRadiusInMM ) return; // no change

  myHitThr = aHitThr;
  //  myMaxStripsPerDir = aMaxStripsPerDir;
  //  myMaxTimeCellsPerDir = aMaxTimeCellsPerDir;
  myTotalChargeThr = aTotalChargeThr;
  myMatchRadiusInMM = aMatchRadiusInMM;

  resetHistograms();
  /*  
  if(!myHistogramsInitialized) {
    initializeHistograms();
  } else {
    // change of cuts invalidate existing histograms
    resetHistograms();
  }
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::initializeHistograms(){

  if(myHistogramsInitialized || !myEvent) return; // nothing to do

  std::string hname;
  double maxTimeDiff; // upper time difference range [s]

  hname = "h_timediff1_all"; maxTimeDiff = 100.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (no cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff2_all"; maxTimeDiff = 10.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (no cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff3_all"; maxTimeDiff = 1.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (no cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff4_all"; maxTimeDiff = 0.1; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (no cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff5_all"; maxTimeDiff = 0.01; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (no cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff1_dot"; maxTimeDiff = 100.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (dot cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff2_dot"; maxTimeDiff = 10.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (dot cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff3_dot"; maxTimeDiff = 1.0; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (dot cuts)", 100, 0.0, maxTimeDiff); 

  hname = "h_timediff4_dot"; maxTimeDiff = 0.1; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (dot cuts)", 100, 0.0, maxTimeDiff);

  hname = "h_timediff5_dot"; maxTimeDiff = 0.01; // sec
  myHistograms[hname]=new TH1D(hname.c_str(), "Event time difference (dot cuts)", 100, 0.0, maxTimeDiff);

  double xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  double bin_widthXY =  myEvent->GetGeoPtr()->GetStripPitch();// [mm]
  std::tie(xmin, xmax, ymin, ymax) = myEvent->GetGeoPtr()->rangeXY();
  const int nbinX = (int)((xmax-xmin)/bin_widthXY+0.5);
  const int nbinY = (int)((ymax-ymin)/bin_widthXY+0.5);
  hname = "h_xy_dot";
  myHistograms2D[hname]=new TH2D(hname.c_str(), "Centers of dot-like events",
				    nbinX, xmin, xmax, nbinY, ymin, ymax);				 
  hname = "h_x_dot";
  myHistograms[hname]=new TH1D(hname.c_str(), "Centers of dot-like events",
				  nbinX, xmin, xmax);
  hname = "h_y_dot";
  myHistograms[hname]=new TH1D(hname.c_str(), "Centers of dot-like events",
				  nbinY, ymin, ymax);
  hname = "prof_deltaz_xy_dot";
  myHistogramsProf2D[hname]=new TProfile2D(hname.c_str(), "Z-width of dot-like events",
					   nbinX, xmin, xmax, nbinY, ymin, ymax, 0., myMatchRadiusInMM);
  hname = "prof_deltaz_x_dot";
  myHistogramsProf[hname]=new TProfile(hname.c_str(), "Z-width of dot-like events",
				       nbinX, xmin, xmax, 0., myMatchRadiusInMM);
  hname = "prof_deltaz_y_dot";
  myHistogramsProf[hname]=new TProfile(hname.c_str(), "Z-width of dot-like events",
				   nbinY, ymin, ymax, 0., myMatchRadiusInMM);

  zmin = myEvent->GetGeoPtr()->GetDriftCageZmin();
  zmax = myEvent->GetGeoPtr()->GetDriftCageZmax();
  const int nbinZ = myEvent->GetGeoPtr()->GetAgetNtimecells();
  hname = "h_z_dot";
  myHistograms[hname]=new TH1D(hname.c_str(), "Centers of dot-like events",
				  nbinZ, zmin, zmax);				 

  myHistogramsInitialized = true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::resetHistograms(){

  if(!myHistogramsInitialized) return; // nothing to do

  for(auto it=myHistograms.begin() ; it!=myHistograms.end() ; ++it) {
    if(it->second) (it->second)->Reset();
  }
  for(auto it=myHistograms2D.begin() ; it!=myHistograms2D.end() ; ++it) {
    if(it->second) (it->second)->Reset();
  }
  for(auto it=myHistogramsProf.begin() ; it!=myHistogramsProf.end() ; ++it) {
    if(it->second) (it->second)->Reset();
  }
  for(auto it=myHistogramsProf2D.begin() ; it!=myHistogramsProf2D.end() ; ++it) {
    if(it->second) (it->second)->Reset();
  }
  myEventCounter_All = 0;
  myEventCounter_Dot = 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool DotFinder::checkCuts(){

  isDotEvent = false;
  myDot3D = TVector3(0., 0., 0.);
  myDotDeltaZ = 0.0;

  // sanity check
  if(!myEvent) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: myEvent is NULL => ERROR!" << std::endl;
    //////// DEBUG
    return false;
  }

  // are data frames missing?
  if( !myEvent->CheckAsadNboards() ) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: Missing data frames ==> event rejected." << std::endl;
    //////// DEBUG
    return false;
  }

  if(myEvent->GetTotalCharge(-1, -1, -1, -1, filter_type::threshold) < myTotalChargeThr ) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: Too small total charge  => event rejected." << std::endl
	      << "                      TotalCharge="
	      << myEvent->GetTotalCharge(-1, -1, -1, -1, filter_type::threshold)
	      << " (limit=" << myTotalChargeThr << ")" << std::endl;
    //////// DEBUG
    return false;
  }

  int multiplicityU = myEvent->GetMultiplicity(false, projection_type::DIR_U, -1, -1,  filter_type::threshold);
  int multiplicityV = myEvent->GetMultiplicity(false, projection_type::DIR_V, -1, -1,  filter_type::threshold);
  int multiplicityW = myEvent->GetMultiplicity(false, projection_type::DIR_W, -1, -1,  filter_type::threshold);
  
  if(multiplicityU + multiplicityV + multiplicityW<2){
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: Cluster has not enough hits => event rejected." << std::endl
	      << "                      Dir=" << DIR_U
	      << ": mult= "<<multiplicityU<< std::endl
      	      << "                      Dir=" << DIR_V
	      << ": mult= "<<multiplicityV<< std::endl
      	      << "                      Dir=" << DIR_W
	      << ": mult= "<<multiplicityV<< std::endl
	      << std::endl;
    //////// DEBUG
    return false;
  }

  const int maxStrips    = std::max( 1, (int)(myMatchRadiusInMM/myEvent->GetGeoPtr()->GetStripPitch()+0.5) );      // maximum # of strips
  const int maxTimeCells = std::max( 1, (int)(myMatchRadiusInMM/(10.0*myEvent->GetGeoPtr()->GetDriftVelocity()     // [1mm / (10mm/cm * 1cm/us / 1MHz ]=[dimensionless]
								 /myEvent->GetGeoPtr()->GetSamplingRate())+0.5) ); // maximum # of time cells
			       
  
  
  int minTime=0, maxTime=0, minStrip=0, maxStrip=0; 
  std::tie(minTime, maxTime, minStrip, maxStrip) = myEvent->GetSignalRange(-1, filter_type::threshold);

  /*
  int minTimeU=0, maxTimeU=0, minStripU=0, maxStripU=0;
  int minTimeV=0, maxTimeV=0, minStripV=0, maxStripV=0;
  int minTimeW=0, maxTimeW=0, minStripW=0, maxStripW=0;
  std::tie(minTimeU, maxTimeU, minStripU, maxStripU) = myEvent->GetSignalRange(projection_type::DIR_U, filter_type::threshold);
  std::tie(minTimeV, maxTimeV, minStripV, maxStripV) = myEvent->GetSignalRange(projection_type::DIR_V, filter_type::threshold);
  std::tie(minTimeW, maxTimeW, minStripW, maxStripW) = myEvent->GetSignalRange(projection_type::DIR_W, filter_type::threshold);

   if(multiplicityU > (int)myMaxStripsPerDir ||
     multiplicityV > (int)myMaxStripsPerDir ||
     multiplicityW > (int)myMaxStripsPerDir ||
     maxTimeU - minTimeU > (int)myMaxTimeCellsPerDir ||
     maxTimeV - minTimeV > (int)myMaxTimeCellsPerDir ||
     maxTimeW - minTimeW > (int)myMaxTimeCellsPerDir ||
     maxStripU - minStripU > (int)myMaxStripsPerDir ||
     maxStripV - minStripV > (int)myMaxStripsPerDir ||
     maxStripW - minStripW > (int)myMaxStripsPerDir 
  */

  myDotDeltaZ = fabs(maxTime - minTime + 1 )*(10.0*myEvent->GetGeoPtr()->GetDriftVelocity()     // [ 10mm/cm * 1cm/us / 1MHz ]=[mm]
					      /myEvent->GetGeoPtr()->GetSamplingRate());      
  if(myEvent->GetMultiplicity(false, projection_type::DIR_U, -1, -1,  filter_type::threshold) > maxStrips ||
     myEvent->GetMultiplicity(false, projection_type::DIR_V, -1, -1,  filter_type::threshold) > maxStrips ||
     myEvent->GetMultiplicity(false, projection_type::DIR_W, -1, -1,  filter_type::threshold) > maxStrips ||
     maxTime - minTime > maxTimeCells ) {
    //////// DEBUG
    int minTimeU=0, maxTimeU=0, minStripU=0, maxStripU=0;
    int minTimeV=0, maxTimeV=0, minStripV=0, maxStripV=0;
    int minTimeW=0, maxTimeW=0, minStripW=0, maxStripW=0;
    std::tie(minTimeU, maxTimeU, minStripU, maxStripU) = myEvent->GetSignalRange(projection_type::DIR_U, filter_type::threshold);
    std::tie(minTimeV, maxTimeV, minStripV, maxStripV) = myEvent->GetSignalRange(projection_type::DIR_V, filter_type::threshold);
    std::tie(minTimeW, maxTimeW, minStripW, maxStripW) = myEvent->GetSignalRange(projection_type::DIR_W, filter_type::threshold);

    std::cout << "DotFinder::checkCuts: Cluster is too wide => event rejected." << std::endl
	      << "                      Dir=" << DIR_U
	      << ": mult="
	      << myEvent->GetMultiplicity(false, projection_type::DIR_U, -1, -1,  filter_type::threshold)
	      << " (limit=" << maxStrips << "), strip_min="
	      << minStripU
	      << ", strip_max="
	      << maxStripU
	      << std::endl
      	      << "                      Dir=" << DIR_V
	      << ": mult="
	      << myEvent->GetMultiplicity(false, projection_type::DIR_V, -1, -1,  filter_type::threshold)
	      << " (limit=" << maxStrips << "), strip_min="
	      << minStripV
	      << ", strip_max="
	      << maxStripV
	      << std::endl
      	      << "                      Dir=" << DIR_W
	      << ": mult="
	      << myEvent->GetMultiplicity(false, projection_type::DIR_W, -1, -1,  filter_type::threshold)
	      << " (limit=" << maxStrips << "), strip_min="
	      << minStripW
	      << ", strip_max="
	      << maxStripW
	      << std::endl
	      << "                      Dir=Z"
	      << ": cells=" << maxTime - minTime
	      << " (limit=" << maxTimeCells << "), cell_min=" << minTime << ",cell_max=" << maxTime << std::endl;
    //////// DEBUG
  return false;
  }

  
  // STEP2 - Z matching
  bool err_flag0[3];
  err_flag0[0] = false;
  err_flag0[1] = false;
  err_flag0[2] = false;

  int maxTimeU=0, maxStripU=0;
  int maxTimeV=0, maxStripV=0;
  int maxTimeW=0, maxStripW=0;
  std::tie(maxTimeU, maxStripU) = myEvent->GetMaxChargePos(projection_type::DIR_U, filter_type::threshold);
  std::tie(maxTimeV, maxStripV) = myEvent->GetMaxChargePos(projection_type::DIR_V, filter_type::threshold);
  std::tie(maxTimeW, maxStripW) = myEvent->GetMaxChargePos(projection_type::DIR_W, filter_type::threshold);
      
  double ZinMM[3] = { myEvent->GetGeoPtr()->Timecell2pos(maxTimeU, err_flag0[0]),
		      myEvent->GetGeoPtr()->Timecell2pos(maxTimeV, err_flag0[1]),
		      myEvent->GetGeoPtr()->Timecell2pos(maxTimeW, err_flag0[2]) };
  const auto nbad_zcoord = ((int)err_flag0[0] + (int)err_flag0[1] + (int)err_flag0[2]);
  if( nbad_zcoord > 2 ||
      (!err_flag0[0] && fabs( ZinMM[0] - ZinMM[1] ) > myMatchRadiusInMM ) ||
      (!err_flag0[1] && fabs( ZinMM[1] - ZinMM[2] ) > myMatchRadiusInMM ) ||
      (!err_flag0[2] && fabs( ZinMM[2] - ZinMM[0] ) > myMatchRadiusInMM )   ) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: STEP2: Z-coordinate matching error => event rejected." << std::endl;
    //////// DEBUG
    return false;
  }

  // STEP3 - XY matching
  int UVWstrip[3] = {maxStripU, maxStripV, maxStripW};

  bool err_flag1[3];
  err_flag1[0] = false;
  err_flag1[1] = false;
  err_flag1[2] = false;
  double UVWinMM[3] = {  myEvent->GetGeoPtr()->Strip2posUVW(DIR_U, UVWstrip[0], err_flag1[0]),
			 myEvent->GetGeoPtr()->Strip2posUVW(DIR_V, UVWstrip[1], err_flag1[1]),
			 myEvent->GetGeoPtr()->Strip2posUVW(DIR_W, UVWstrip[2], err_flag1[2]) };
  if( ((int)err_flag1[0] + (int)err_flag1[1] + (int)err_flag1[2]) > 1 ) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: STEP3: XY-coordinate matching error => event rejected." << std::endl
	      << "                      Dir=" << DIR_U << ": coord=" << UVWinMM[0] << "mm, err=" << err_flag1[0] << std::endl
      	      << "                      Dir=" << DIR_V << ": coord=" << UVWinMM[1] << "mm, err=" << err_flag1[1] << std::endl
      	      << "                      Dir=" << DIR_W << ": coord=" << UVWinMM[2] << "mm, err=" << err_flag1[2] << std::endl;
    //////// DEBUG
    return false;
  }

  // check each pair
  TVector2 XYinMM[3]; // [mm, mm]
  bool err_flag2[3];
  err_flag2[0] = err_flag1[0] || err_flag1[1] || (! myEvent->GetGeoPtr()->GetUVWCrossPointInMM(DIR_U, UVWinMM[0], DIR_V, UVWinMM[1], XYinMM[0]));
  err_flag2[1] = err_flag1[1] || err_flag1[2] || (! myEvent->GetGeoPtr()->GetUVWCrossPointInMM(DIR_V, UVWinMM[1], DIR_W, UVWinMM[2], XYinMM[1]));
  err_flag2[2] = err_flag1[2] || err_flag1[0] || (! myEvent->GetGeoPtr()->GetUVWCrossPointInMM(DIR_W, UVWinMM[2], DIR_U, UVWinMM[0], XYinMM[2]));

  const auto nbad_pairs = (int)err_flag2[0] + (int)err_flag2[1] + (int)err_flag2[2];
  if( nbad_pairs > 2 ||
      (!err_flag2[0] && ( XYinMM[0] - XYinMM[1] ).Mod() > myMatchRadiusInMM ) ||
      (!err_flag2[1] && ( XYinMM[1] - XYinMM[2] ).Mod() > myMatchRadiusInMM ) ||
      (!err_flag2[2] && ( XYinMM[2] - XYinMM[0] ).Mod() > myMatchRadiusInMM )   ) {
    //////// DEBUG
    std::cout << "DotFinder::checkCuts: STEP3: XY-coordinate matching error => event rejected." << std::endl
	      << "from UV pair [mm]: [X,Y]=" << "[" << XYinMM[0].X() << ", " << XYinMM[0].Y() << "], err=" << err_flag2[0] << std::endl
      	      << "from VW pair [mm]: [X,Y]=" << "[" << XYinMM[1].X() << ", " << XYinMM[1].Y() << "], err=" << err_flag2[1] << std::endl
      	      << "from WU pair [mm]: [X,Y]=" << "[" << XYinMM[2].X() << ", " << XYinMM[2].Y() << "], err=" << err_flag2[2] << std::endl
	      << "Discrepancy  [mm]: " << ( XYinMM[0] - XYinMM[1] ).Mod() << ", " << ( XYinMM[1] - XYinMM[2] ).Mod() << ", " << ( XYinMM[2] - XYinMM[0] ).Mod() << std::endl;
    //////// DEBUG
    return false;
  }

  // event PASSED
  isDotEvent = true;
  const auto ngood_pairs = 3 - nbad_pairs;
  TVector3 componentXY(0., 0., 0.);
  if(!err_flag2[0]) componentXY += TVector3( XYinMM[0].X(), XYinMM[0].Y(), 0.);
  if(!err_flag2[1]) componentXY += TVector3( XYinMM[1].X(), XYinMM[1].Y(), 0.);
  if(!err_flag2[2]) componentXY += TVector3( XYinMM[2].X(), XYinMM[2].Y(), 0.);
  componentXY *= 1.0/(double)ngood_pairs;
  const auto ngood_zcoord = 3 - nbad_zcoord;
  TVector3 componentZ(0., 0., 0.);
  if(!err_flag0[0]) componentZ += TVector3( 0., 0., ZinMM[0] );
  if(!err_flag0[1]) componentZ += TVector3( 0., 0., ZinMM[1] );
  if(!err_flag0[2]) componentZ += TVector3( 0., 0., ZinMM[2] );
  componentZ *= 1.0/(double)ngood_zcoord;
  myDot3D = componentXY + componentZ;
  //////// DEBUG
  std::cout << "DotFinder::checkCuts: Passed all cuts => event ACCEPTED." << std::endl
	    << "                      position: [X,Y,Z]=[" << myDot3D.X() << " mm , " << myDot3D.Y() << " mm , " << myDot3D.Z() << " mm]" << std::endl
	    << "                      quality:  X,Y from " << ngood_pairs << " point(s), Z from " << ngood_zcoord << " point(s)" << std::endl;
  //////// DEBUG

  /*
  myDot3D = TVector3( (XYinMM[0].X()+XYinMM[1].X()+XYinMM[2].X())/3.0,
		      (XYinMM[0].Y()+XYinMM[1].Y()+XYinMM[2].Y())/3.0,
		      (ZinMM[0]+ZinMM[1]+ZinMM[2])/3.0 );
  */
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::reconstruct(){

  // sanity check
  if(!myHistogramsInitialized || !myEvent) return;
  
  //////// DEBUG
  std::cout << "DotFinder::reconstruct: myCluster thr=" << myHitThr
	    << ", hits=" << myEvent->GetMultiplicity(true, -1, -1, -1, filter_type::threshold) << std::endl << std::flush;
  //////// DEBUG
  
  // fill histograms before selections cuts (all events)
  auto currentEventTime = myEvent->GetEventInfo().GetEventTimestamp(); // in 10ns units
  
  myEventCounter_All++;
  if(isFirstEvent_All) {
    previousEventTime_All = currentEventTime; 
    isFirstEvent_All = false;
  } else {
    double timeDiffInSEC = 10.0e-9*(currentEventTime-previousEventTime_All); // convert 10ns units into seconds
    previousEventTime_All = currentEventTime;
    myHistograms.at("h_timediff1_all")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff2_all")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff3_all")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff4_all")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff5_all")->Fill( timeDiffInSEC );
  }
   
  // apply cuts
  if(!checkCuts()) return; // reject event
    
  // fill histograms after selection cuts (point-like events)
  myHistograms2D.at("h_xy_dot")->Fill( myDot3D.X(), myDot3D.Y() );
  myHistograms.at("h_x_dot")->Fill( myDot3D.X() );
  myHistograms.at("h_y_dot")->Fill( myDot3D.Y() );
  myHistograms.at("h_z_dot")->Fill( myDot3D.Z() );
  myHistogramsProf2D.at("prof_deltaz_xy_dot")->Fill( myDot3D.X(), myDot3D.Y(), myDotDeltaZ  );
  myHistogramsProf.at("prof_deltaz_x_dot")->Fill( myDot3D.X(), myDotDeltaZ );
  myHistogramsProf.at("prof_deltaz_y_dot")->Fill( myDot3D.Y(), myDotDeltaZ );
  
  if(isFirstEvent_Dot) {
    previousEventTime_Dot = currentEventTime;
    isFirstEvent_Dot = false;
  } else {
    double timeDiffInSEC = 10.0e-9*(currentEventTime-previousEventTime_Dot); // convert 10ns units into seconds
    previousEventTime_Dot = currentEventTime;
    myHistograms.at("h_timediff1_dot")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff2_dot")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff3_dot")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff4_dot")->Fill( timeDiffInSEC );
    myHistograms.at("h_timediff5_dot")->Fill( timeDiffInSEC );
  }
  myEventCounter_Dot++;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::openOutputStream(const std::string & fileName){

  //  if(!myFittedTrackPtr){
  //    std::cout<<KRED<<__FUNCTION__<<RST
  //	     <<" pointer to fitted track not set!"
  //	     <<std::endl;
  //    return;
  //  }
  //  std::string treeName = "TPCRecoData";
  myOutputFilePtr = std::make_shared<TFile>(fileName.c_str(),"RECREATE");
  //  myOutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  //  myOutputTreePtr->Branch("RecoEvent", "Track3D", &myFittedTrackPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::closeOutputStream(){

  if(!myOutputFilePtr){
    std::cout<<KRED<<__FUNCTION__<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }
  myOutputFilePtr->Close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DotFinder::fillOutputStream(){

  if(!myOutputFilePtr){
    std::cout<<KRED<<__FUNCTION__<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }
  myOutputFilePtr->cd();
  
  if(!myHistogramsInitialized) {
    std::cout<<KRED<<__FUNCTION__<<RST
	     <<" histograms were not initialized!"
	     <<std::endl;
    return;
  }
  for(auto it=myHistograms.begin() ; it!=myHistograms.end() ; ++it) {
    if(it->second) (it->second)->Write();
  }
  for(auto it=myHistograms2D.begin() ; it!=myHistograms2D.end() ; ++it) {
    if(it->second) (it->second)->Write();
  }
  for(auto it=myHistogramsProf.begin() ; it!=myHistogramsProf.end() ; ++it) {
    if(it->second) (it->second)->Write();
  }
  for(auto it=myHistogramsProf2D.begin() ; it!=myHistogramsProf2D.end() ; ++it) {
    if(it->second) (it->second)->Write();
  }

  std::cout << "DotFinder: Total analyzed events   = " << myEventCounter_All << std::endl
	    << "           Total point-like events = " << myEventCounter_Dot << std::endl;
}
