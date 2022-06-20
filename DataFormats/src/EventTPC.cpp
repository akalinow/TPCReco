#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm> 

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"

#include "EventTPC.h"
#include "TrackSegmentTPC.h"

#include "colorText.h"
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
EventTPC::EventTPC(){

  int n_directions = 3;
  max_charge.resize(n_directions);
  max_charge_timing.resize(n_directions);
  max_charge_strip.resize(n_directions);
  tot_charge.resize(n_directions);

  Clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::Clear(){

  SetGeoPtr(0);
  initOK = false;

  glb_max_charge = 0.0;
  glb_max_charge_timing = -1;
  glb_max_charge_channel = -1;
  glb_tot_charge = 0.0;

  for(int idir=projection_type::DIR_U; idir<projection_type::DIR_W; ++idir) {
    max_charge.at(idir)=0.0;
    max_charge_timing.at(idir)=-1;
    max_charge_strip.at(idir)=-1;
    tot_charge.at(idir)=0.0;   
  }  
  totalChargeMap.clear();  // 2-key map: strip_dir, strip_number
  totalChargeMap2.clear();  // 2-key map: strip_dir, time_cell
  totalChargeMap3.clear();  // 1-key map: time_cell
  totalChargeMap4.clear();  // 3-key map: strip_dir, strip_section, strip_number
  totalChargeMap5.clear();  // 3-key map: strip_dir, strip_section, time_cell
  maxChargeMap.clear();    // 2-key map: strip_dir, strip_number
  maxChargeMap2.clear();   // 3-key map: strip_dir, strip_section, strip_number 
  chargeMap.clear();
  chargeMapWithSections.clear();
  asadMap.clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
  myGeometryPtr = aPtr;
  initOK = myGeometryPtr && myGeometryPtr->IsOK();
  if(initOK) create3DHistoTemplate();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetChargeMap(const PEventTPC::chargeMapType & aChargeMap){

  chargeMapWithSections = aChargeMap;
  filterHits(filter_type::none);
  filterHits(filter_type::threshold);

  updateHistosCache(filter_type::none);
  updateHistosCache(filter_type::threshold);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::fillAuxMaps(){

  for(auto const & item: chargeMapWithSections){
    auto key = item.first;
    auto value = item.second;

    const int strip_dir = std::get<0>(key);
    const int strip_sec = std::get<1>(key);
    const int strip_num = std::get<2>(key);
    const int time_cell = std::get<3>(key);

    MultiKey2 mkey_total(strip_dir, strip_num);
    MultiKey2 mkey_total2(strip_dir, time_cell);
    MultiKey3 mkey_total4(strip_dir, strip_sec, strip_num);
    MultiKey3 mkey_total5(strip_dir, strip_sec, time_cell);
    MultiKey2 mkey_maxval(strip_dir, strip_num);
    MultiKey3 mkey_maxval2(strip_dir, strip_sec, strip_num);

    glb_tot_charge += value;
    tot_charge[strip_dir] += value;
    totalChargeMap[mkey_total] += value;
    totalChargeMap2[mkey_total2] += value;
    totalChargeMap3[time_cell] += value;
    totalChargeMap4[mkey_total4] += value;
    totalChargeMap5[mkey_total5] += value;
    maxChargeMap[mkey_maxval] = value>maxChargeMap[mkey_maxval]? value : maxChargeMap[mkey_maxval];
    maxChargeMap2[mkey_maxval2] = value>maxChargeMap2[mkey_maxval2]? value : maxChargeMap2[mkey_maxval2];
    updateMaxChargeMaps(key, value);
   
    auto strip=myGeometryPtr->GetStripByDir(strip_dir, strip_sec, strip_num);
    if(strip) asadMap[MultiKey2(strip->CoboId(), strip->AsadId())] += 1;
  }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::create3DHistoTemplate(){

  if(a3DHistoRawPtr) return;

  int nBinsX = myGeometryPtr->GetAgetNtimecells();
  double minX = -0.5;
  double maxX = nBinsX + minX;// ends at 511.5 (cells numbered from 0 to 511)

  int nBinsY = 0;
  nBinsY = std::max(nBinsY, myGeometryPtr->GetDirNstrips(projection_type::DIR_U));
  nBinsY = std::max(nBinsY, myGeometryPtr->GetDirNstrips(projection_type::DIR_V));
  nBinsY = std::max(nBinsY, myGeometryPtr->GetDirNstrips(projection_type::DIR_W));
  double minY = 0.5;
  double maxY = nBinsY + minY;

  int nBinsZ = 3;
  double minZ = -0.5;
  double maxZ = nBinsZ + minZ;

  TH3D* a3DHisto = new TH3D("a3DHisto","",
			    nBinsX, minX, maxX,
			    nBinsY, minY, maxY,
			    nBinsZ, minZ, maxZ);

  a3DHistoRawPtr.reset(a3DHisto);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::updateHistosCache(filter_type filterType){

  std::shared_ptr<TH3D> aHisto((TH3D*)a3DHistoRawPtr->Clone());

  double x = 0.0, y = 0.0, z = 0.0, value=0.0;
  for(const auto & key: keyLists.at(filterType)){
    value = chargeMapWithSections.at(key);
    x = std::get<3>(key);
    y = std::get<2>(key);
    z = std::get<0>(key);    
    aHisto->Fill(x, y, z, value);
  }
  a3DHistoRawMap[filterType] = aHisto;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::updateMaxChargeMaps(const PEventTPC::chargeMapType::key_type & key,
				   double value){

  const int strip_dir = std::get<0>(key);
  const int strip_sec = std::get<1>(key);
  const int strip_num = std::get<2>(key);
  const int time_cell = std::get<3>(key);
  
  if(value > max_charge[strip_dir] ) { 
    max_charge[strip_dir] = value;
    max_charge_timing[strip_dir] = time_cell;
    max_charge_strip[strip_dir] = strip_num;
    if(value > glb_max_charge ) {
      glb_max_charge = value;
      glb_max_charge_timing = time_cell;
      glb_max_charge_channel = myGeometryPtr->Global_strip2normal(strip_dir, strip_sec, strip_num);
    }
  }  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
bool EventTPC::CheckAsadNboards() const {
  return IsOK() && myGeometryPtr->GetAsadNboards()==(int)asadMap.size();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell) const {

  auto item = chargeMapWithSections.find({strip_dir,  strip_section, strip_number, time_cell});
  if(item!=chargeMapWithSections.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const {
  
  if(strip) return GetValByStrip(strip->Dir(), strip->Section(), strip->Num(), time_cell);
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStripMerged(int strip_dir, int strip_number, int time_cell) const{

  auto item = chargeMap.find({strip_dir,  strip_number, time_cell});
  if(item!=chargeMap.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir, int strip_section, int strip_number) const {

  auto item = maxChargeMap2.find({strip_dir,  strip_section, strip_number});
  if(item!=maxChargeMap2.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir, int strip_number) const {

  auto item = maxChargeMap.find({strip_dir,  strip_number});
  if(item!=maxChargeMap.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir) const { 
  return max_charge.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge() const { 
  if(!IsOK()) return 0.0;
  return glb_max_charge;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeTime(int strip_dir) const {
  return max_charge_timing.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeStrip(int strip_dir) const {
  return max_charge_strip.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeTime() const {
  if(!IsOK()) return ERROR;
  return glb_max_charge_timing;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeChannel() const { 
  if(!IsOK()) return ERROR;
  return glb_max_charge_channel;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir, int strip_section, int strip_number) const {

   auto item = totalChargeMap4.find({strip_dir, strip_section, strip_number});
   if(item!=totalChargeMap4.end()) return item->second;
   return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir, int strip_number) const {

  auto item = totalChargeMap.find({strip_dir, strip_number});
  if(item!=totalChargeMap.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir) const{
  return tot_charge.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge() const {
  if(!IsOK()) return 0.0;
  return glb_tot_charge;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int strip_dir, int strip_section, int time_cell) const {

  auto item = totalChargeMap5.find({strip_dir, strip_section, time_cell});
  if(item!=totalChargeMap5.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int strip_dir, int time_cell) const {

  auto item = totalChargeMap2.find({strip_dir, time_cell});
  if(item!=totalChargeMap2.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int time_cell) const {

  auto item = totalChargeMap3.find({time_cell});
  if(item!=totalChargeMap3.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::filterHits(filter_type filterType){

  double chargeThreshold = 35.0;
  int delta_strips = 2;
  int delta_timecells = 5;

  std::set<PEventTPC::chargeMapType::key_type> keyList;
  
  for(const auto & item: chargeMapWithSections){
    auto key = item.first;
    auto value = item.second;
    if(value>chargeThreshold || filterType==filter_type::none){
      keyList.insert(key);      
      if(filterType==filter_type::threshold) addEnvelope(key, keyList, delta_timecells, delta_strips);
    }
  }
 keyLists[filterType] = keyList;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::addEnvelope(PEventTPC::chargeMapType::key_type key,
			   std::set<PEventTPC::chargeMapType::key_type> & keyList,
			   double delta_timecells,
			   double delta_strips){

  int iDir = std::get<0>(key);
  int iSection = std::get<1>(key);

  for(int iCell=std::get<3>(key)-delta_timecells;
      iCell<=std::get<3>(key)+delta_timecells;++iCell){
    for(int iStrip=std::get<2>(key)-delta_strips;
	iStrip<=std::get<2>(key)+delta_strips;++iStrip){
      auto neighbourKey = std::make_tuple(iDir, iSection, iStrip, iCell);
      keyList.insert(neighbourKey);
    }
  }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH1D> EventTPC::get1DProjection(projection_type projType,
						filter_type filterType,
						scale_type scaleType){
  TH1D *h1D = 0;
  std::shared_ptr<TH3D> a3DHistoRawPtr = a3DHistoRawMap.at(filterType);
  
  if(projType==projection_type::DIR_U ||
     projType==projection_type::DIR_V ||
     projType==projection_type::DIR_W){

    int minY = 0.5;
    int maxY = myGeometryPtr->GetDirNstrips(projType)+minY;

    h1D = a3DHistoRawPtr->ProjectionY("_py",0,-1,
				      static_cast<int>(projType)+1, static_cast<int>(projType)+1);
    h1D->GetXaxis()->SetRangeUser(minY, maxY);
  }
  else if(projType==projection_type::DIR_TIME_U){
    h1D = a3DHistoRawPtr->ProjectionX("_px",0,-1,
				      static_cast<int>(projection_type::DIR_U)+1, static_cast<int>(projection_type::DIR_U)+1);
    
  }
  else if(projType==projection_type::DIR_TIME_V){
    h1D = a3DHistoRawPtr->ProjectionX("_px",0,-1,
				      static_cast<int>(projection_type::DIR_V)+1, static_cast<int>(projection_type::DIR_V)+1);
    
  }
  else if(projType==projection_type::DIR_TIME_W){
    h1D = a3DHistoRawPtr->ProjectionX("_px",0,-1,
				      static_cast<int>(projection_type::DIR_W)+1, static_cast<int>(projection_type::DIR_W)+1);    
  }
  else if(projType==projection_type::DIR_TIME){
    h1D = a3DHistoRawPtr->ProjectionX();
  }
  else{
    std::cout<<KRED<<"EventTPC::get1DProjection(): unknown projType: "<<RST<<projType<<std::endl;
  }

  if(scaleType==scale_type::mm) scale1DHistoToMM(h1D, projType);
  setHistoLabels(h1D, projType, filterType, scaleType);

  return std::shared_ptr<TH1D>(h1D);    
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH2D> EventTPC::get2DProjection(projection_type projType,
						filter_type filterType,
						scale_type scaleType){
  TH2D *h2D = 0;
  std::shared_ptr<TH3D> a3DHistoRawPtr = a3DHistoRawMap.at(filterType);
  
  if(projType==projection_type::DIR_TIME_U){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_U)+1,
					 static_cast<int>(projection_type::DIR_U)+1);
    h2D = (TH2D*)a3DHistoRawPtr->Project3D("xy");
  }
  else if(projType==projection_type::DIR_TIME_V){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_V)+1,
					 static_cast<int>(projection_type::DIR_V)+1);
    h2D = (TH2D*)a3DHistoRawPtr->Project3D("xy");
  }
  else if(projType==projection_type::DIR_TIME_W){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_W)+1,
					 static_cast<int>(projection_type::DIR_W)+1);
    h2D = (TH2D*)a3DHistoRawPtr->Project3D("xy");
  }

  if(scaleType==scale_type::mm) scale2DHistoToMM(h2D, projType);
  setHistoLabels(h2D, projType, filterType, scaleType);
  
  return std::shared_ptr<TH2D>(h2D);    
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::scale1DHistoToMM(TH1D *h1D, projection_type projType) const{

  double minX = 0.0, maxX=0.0;
  if(projType==projection_type::DIR_U ||
     projType==projection_type::DIR_V ||
     projType==projection_type::DIR_W){
      
    std::tie(minX, maxX) = myGeometryPtr->rangeStripDirInMM(projType);
    minX-=myGeometryPtr->GetStripPitch()/2.0;
    maxX+=myGeometryPtr->GetStripPitch()/2.0;
  }
  else if(projType==projection_type::DIR_TIME_U ||
	  projType==projection_type::DIR_TIME_V ||
	  projType==projection_type::DIR_TIME_W ||
	  projType==projection_type::DIR_TIME){
    bool err_flag;
    int nBinsX = myGeometryPtr->GetAgetNtimecells();
    minX = -0.5;
    maxX = nBinsX + minX;// ends at 511.5 (cells numbered from 0 to 511)
    
    minX = myGeometryPtr->Timecell2pos(minX, err_flag); 
    maxX = myGeometryPtr->Timecell2pos(maxX, err_flag);
  }
  h1D->GetXaxis()->SetRange(minX, maxX);  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::scale2DHistoToMM(TH2D *h2D, projection_type projType) const{

  bool err_flag;
  int nBinsX = myGeometryPtr->GetAgetNtimecells();
  double minX = -0.5;
  double maxX = nBinsX + minX;// ends at 511.5 (cells numbered from 0 to 511)
  
  minX = myGeometryPtr->Timecell2pos(minX, err_flag); 
  maxX = myGeometryPtr->Timecell2pos(maxX, err_flag);
  
  double minY = 0.0, maxY=0.0;
  std::tie(minY, maxY) = myGeometryPtr->rangeStripDirInMM(projection_type::DIR_V);//FIXME
  minY-=myGeometryPtr->GetStripPitch()/2.0;
  maxY+=myGeometryPtr->GetStripPitch()/2.0;
  
  h2D->GetXaxis()->SetRange(minX, maxX);
  h2D->GetYaxis()->SetRange(minY, maxY);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::setHistoLabels(TH1 *h1, projection_type projType,
			      filter_type filterType,
			      scale_type scaleType) const {

  auto event_id = myEventInfo.GetEventId();
  projection_type strip_dir = projection_type::NONE;

  if(projType==projection_type::DIR_U ||
     projType==projection_type::DIR_V ||
     projType==projection_type::DIR_W ||
     projType==projection_type::DIR_TIME){
    strip_dir = projType;
  }
  else if(projType==projection_type::DIR_TIME_U){
    strip_dir = projection_type::DIR_U;
  }
  else if(projType==projection_type::DIR_TIME_V){
    strip_dir = projection_type::DIR_V;
  }
  else if(projType==projection_type::DIR_TIME_W){
    strip_dir = projection_type::DIR_W;
  }

  std::string filterName = "raw";
  if(filterType==filter_type::threshold) filterName="Thr";
  else if(filterType==filter_type::island) filterName="Island";

  std::string projName = myGeometryPtr->GetDirName(strip_dir);
  std::string scaleName = "";
  if(scaleType==scale_type::mm) scaleName="mm_";

  std::string name = "h"+filterName+"_"+projName+"_"+scaleName+"evt"+std::to_string(event_id);
  std::string title = "Event-"+std::to_string(event_id);
  title+= "Selected by "+filterName+ " from "+projName;
  title+= "integrated over time";

  std::string labelX = projName+ " strip no."; 
  std::string labelY = "Charge/strip [arb.u.]";
  std::string labelZ = "Charge [arb.u.]";

if(h1->GetDimension()==2){
  labelX = "Time bin"; 
  labelY = projName;
 
  if(scaleType==scale_type::mm){
    labelX += "[mm]";
    labelY += "[mm]";
  }
 }   

  
  h1->SetName(name.c_str());
  h1->SetTitle(title.c_str());
  h1->SetXTitle(labelX.c_str());
  h1->SetYTitle(labelY.c_str());
  h1->SetZTitle(labelZ.c_str());  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH2D> EventTPC::GetChannels(int cobo_idx, int asad_idx){ // valid range [0-1][0-3]
  //////// DEBUG
  //  std::cout << "GetChannels: cobo=" << cobo_idx << ", asad=" << asad_idx << std::endl << std::flush;
  //  std::cout << "GetChannels: NasadBoards=" << myGeometryPtr->GetAsadNboards(cobo_idx) << std::endl << std::flush;
  //////// DEBUG  
  if(!IsOK() || myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
     asad_idx<0 || asad_idx>=myGeometryPtr->GetAsadNboards(cobo_idx)) {
    //////// DEBUG
    //    std::cout << "GetChannels: ERROR" << std::endl << std::flush;
     //////// DEBUG  
    return std::shared_ptr<TH2D>();
  }
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_cobo%i_asad%i_signal_evt%d", cobo_idx, asad_idx,event_id),
					 Form("Event-%d: Raw signals from CoBo %i AsAd %i;Time bin [arb.u.];Global AsAd channel no.;Charge/bin [arb.u.]",
					      event_id, cobo_idx, asad_idx),
					 myGeometryPtr->GetAgetNtimecells(),
					 0.0-0.5, 
					 1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels()+1,
					 -0.5,
					 myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels()+0.5 ));
  // fill new histogram
  for(int aget_num=0; aget_num<myGeometryPtr->GetAgetNchips(); ++aget_num) {
    for(int aget_ch=0; aget_ch<myGeometryPtr->GetAgetNchannels();++aget_ch){
      for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	double val = GetValByStrip(myGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_num, aget_ch), icell);	
	result->Fill(1.*icell, aget_num*myGeometryPtr->GetAgetNchannels()+aget_ch, val); 
      }
    }
  }
  return result;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH2D> EventTPC::GetChannels_raw(int cobo_idx, int asad_idx){ // valid range [0-1][0-3]
  if(!IsOK() || myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
     asad_idx<0 || asad_idx>=myGeometryPtr->GetAsadNboards(cobo_idx)) {return std::shared_ptr<TH2D>();}
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_cobo%i_asad%i_signal_fpn_evt%d", cobo_idx, asad_idx,event_id),
					 Form("Event-%d: Raw signals from Cobo %i Asad %i;Time bin [arb.u.];Global raw channel no.;Charge/bin [arb.u.]",
					      event_id, cobo_idx, asad_idx),
					 myGeometryPtr->GetAgetNtimecells(),
					 0.0-0.5, 
					 1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels_raw()+1,
					 -0.5,
					 myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels_raw()+0.5 ));
  // fill new histogram
  for(int aget_num=0; aget_num<myGeometryPtr->GetAgetNchips(); ++aget_num) {
    for(int aget_ch=0; aget_ch<myGeometryPtr->GetAgetNchannels_raw();++aget_ch){
      for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	double val = GetValByStrip(myGeometryPtr->GetStripByAget_raw(cobo_idx, asad_idx, aget_num, aget_ch), icell);
	result->Fill(1.*icell, aget_num*myGeometryPtr->GetAgetNchannels_raw()+aget_ch, val); 
      }
    }
  }
  return result;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
// get ALL three projections on: XY, XZ, YZ planes
std::vector<TH2D*> EventTPC::Get2D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) { 

  //  const bool rebin_flag=false;
  TH2D *h1 = NULL;
  TH2D *h2 = NULL;
  TH2D *h3 = NULL;
  std::vector<TH2D*> hvec;
  hvec.resize(0);
  bool err_flag = false;

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(projection_type::DIR_U)<1 || cluster.GetNhits(projection_type::DIR_V)<1 || cluster.GetNhits(projection_type::DIR_W)<1 ) return hvec;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[projection_type::DIR_U], MAXIMUM( cluster.min_time[projection_type::DIR_V], cluster.min_time[projection_type::DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[projection_type::DIR_U], MINIMUM( cluster.max_time[projection_type::DIR_V], cluster.max_time[projection_type::DIR_W] ));

  ////////// DEBUG 
  //  std::cout << Form(">>>> EventId = %d", event_id) << std::endl;
  //  std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;
  ////////// DEBUG 

  const std::map<MultiKey2, std::vector<int> > & hitListByTimeDirMerged = cluster.GetHitListByTimeDirMerged();
  
  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
    if((hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_U))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_V))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_W))==hitListByTimeDirMerged.end())) continue;
    
    std::vector<int> hits[3] = {
				hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_U))->second,
				hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_V))->second,
				hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_W))->second};

    ////////// DEBUG 
    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[projection_type::DIR_U].size(), (int)hits[projection_type::DIR_V].size(), (int)hits[projection_type::DIR_W].size()) << std::endl;
    ////////// DEBUG 

    // check if there is at least one hit in each direction
    if(hits[projection_type::DIR_U].size()==0 || hits[projection_type::DIR_V].size()==0 || hits[projection_type::DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each merged strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])
   
    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      for(auto iter0=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_U).begin();
	  iter0!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_U).end(); iter0++) {
	std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(projection_type::DIR_U, *iter0, hits[0].at(i0));
	for(int i1=0; i1<(int)hits[1].size(); i1++) {
	  for(auto iter1=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_V).begin();
	      iter1!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_V).end(); iter1++) {
	    std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(projection_type::DIR_V, *iter1, hits[1].at(i1));
	    for(int i2=0; i2<(int)hits[2].size(); i2++) {
	      for(auto iter2=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_W).begin();
		  iter2!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_W).end(); iter2++) {
		std::shared_ptr<StripTPC> strip2 = myGeometryPtr->GetStripByDir(projection_type::DIR_W, *iter2, hits[2].at(i2));
		
		////////// DEBUG 
		//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
		//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;
		////////// DEBUG 

		TVector2 pos;
		if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
		  (n_match[projection_type::DIR_U])[hits[0].at(i0)]++;
		  (n_match[projection_type::DIR_V])[hits[1].at(i1)]++;
		  (n_match[projection_type::DIR_W])[hits[2].at(i2)]++;
		  MultiKey3 mkey(hits[0].at(i0), hits[1].at(i1), hits[2].at(i2));
		  // accept only first matched 2D postion 
		  if(hitPos.find(mkey)!=hitPos.end()) continue;
		  hitPos[mkey]=pos;
		  ////////// DEBUG 
		  //      std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
		  ////////// DEBUG 
		} else {
		  ////////// DEBUG 
		  //	  std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
		  ////////// DEBUG 
		}
	      }
	    }
	  }
	}
      }
    }
    ////////// DEBUG 
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    ////////// DEBUG 
    if(hitPos.size()<1) continue;

    // book histograms before first fill
    if(h1==NULL && h2==NULL && h3==NULL) {

      double xmin, xmax, ymin, ymax;
      std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
      
      double zmin=0.0-0.5;  // time_cell_min;
      double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
      
      int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
      int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
      int nz = (int)( zmax-zmin );
      auto event_id = myEventInfo.GetEventId();
      zmin = myGeometryPtr->Timecell2pos(zmin, err_flag);
      zmax = myGeometryPtr->Timecell2pos(zmax, err_flag);

      // rebin in space
      if(rebin_space>1) {
	nx /= rebin_space;
	ny /= rebin_space;
      }

      // rebin in time
      if(rebin_time>1) {
	nz /= rebin_time;
      }

      ////////// DEBUG 
      //      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
      //      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;
      ////////// DEBUG 

      h1 = new TH2D( Form("hrecoXY_evt%d", event_id),
		     Form("Event-%d: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, ny, ymin, ymax );
		     
      ////////// DEBUG 
      //      std::cout << Form(">>>> XZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			xmin, xmax, zmin, zmax, nx, nz) << std::endl;
      ////////// DEBUG 
		     
      h2 = new TH2D( Form("hrecoXZ_evt%d", event_id),
		     Form("Event-%d: Projection in XZ;X [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, nz, zmin, zmax );

      ////////// DEBUG 
      //      std::cout << Form(">>>> YZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			ymin, ymax, zmin, zmax, ny, nz) << std::endl;
      ////////// DEBUG 
		     
      h3 = new TH2D( Form("hrecoYZ_evt%d", event_id),
		     Form("Event-%d: Projection in YZ;Y [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     ny, ymin, ymax, nz, zmin, zmax );
    }

    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=std::get<0>(it1->first);
      int v1=std::get<1>(it1->first);
      int w1=std::get<2>(it1->first);
      double qtot[3] = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      double    q[3] = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[projection_type::DIR_U] = GetValByStripMerged(projection_type::DIR_U, u1, icell);
      q[projection_type::DIR_V] = GetValByStripMerged(projection_type::DIR_V, v1, icell);
      q[projection_type::DIR_W] = GetValByStripMerged(projection_type::DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=std::get<0>(it2->first);
	int v2=std::get<1>(it2->first);
	int w2=std::get<2>(it2->first);
	
	if(u1==u2) {
	  qtot[projection_type::DIR_V] += GetValByStripMerged(projection_type::DIR_V, v2, icell);
	  qtot[projection_type::DIR_W] += GetValByStripMerged(projection_type::DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[projection_type::DIR_W] += GetValByStripMerged(projection_type::DIR_W, w2, icell);
	  qtot[projection_type::DIR_U] += GetValByStripMerged(projection_type::DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[projection_type::DIR_U] += GetValByStripMerged(projection_type::DIR_U, u2, icell);
	  qtot[projection_type::DIR_V] += GetValByStripMerged(projection_type::DIR_V, v2, icell);
	}
      }
      fraction[projection_type::DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_U] / qtot[projection_type::DIR_U] ));
      fraction[projection_type::DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_V] / qtot[projection_type::DIR_V] ));
      fraction[projection_type::DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_W] / qtot[projection_type::DIR_W] ));
    }

    // loop over matched hits and fill histograms
    if(h1 && h2 && h3) {

      std::map<MultiKey3, TVector2>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally 
	  val = 
	    GetValByStripMerged(projection_type::DIR_U, std::get<0>(it->first), icell) / n_match[0].at(std::get<0>(it->first)) +
	    GetValByStripMerged(projection_type::DIR_V, std::get<1>(it->first), icell) / n_match[1].at(std::get<1>(it->first)) +
	    GetValByStripMerged(projection_type::DIR_W, std::get<2>(it->first), icell) / n_match[2].at(std::get<2>(it->first));
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions

	  val = 
	    GetValByStripMerged(projection_type::DIR_U, 
				std::get<0>(it->first), icell)*0.5*( fraction[projection_type::DIR_V].at(it->first) + fraction[projection_type::DIR_W].at(it->first) ) +
	    GetValByStripMerged(projection_type::DIR_V, 
				std::get<1>(it->first), icell)*0.5*( fraction[projection_type::DIR_W].at(it->first) + fraction[projection_type::DIR_U].at(it->first) ) +
	    GetValByStripMerged(projection_type::DIR_W, 
				std::get<2>(it->first), icell)*0.5*( fraction[projection_type::DIR_U].at(it->first) + fraction[projection_type::DIR_V].at(it->first) );
	  break;

	default: 
	  val=0.0;

	}; // end of switch (method)...
	
	Double_t z=myGeometryPtr->Timecell2pos(icell, err_flag);
	h1->Fill( (it->second).X(), (it->second).Y(), val );
	h2->Fill( (it->second).X(), z, val );
	h3->Fill( (it->second).Y(), z, val );

      }
    }
  }
  if(h1 && h2 && h3) {
    hvec.push_back(h1);
    hvec.push_back(h2);
    hvec.push_back(h3);
  }
  return hvec;
}

// get 3D histogram of clustered hits
TH3D *EventTPC::Get3D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) {

  TH3D *h = NULL;
  bool err_flag = false;

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(projection_type::DIR_U)<1 || cluster.GetNhits(projection_type::DIR_V)<1 || cluster.GetNhits(projection_type::DIR_W)<1 ) return h;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[projection_type::DIR_U], MAXIMUM( cluster.min_time[projection_type::DIR_V], cluster.min_time[projection_type::DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[projection_type::DIR_U], MINIMUM( cluster.max_time[projection_type::DIR_V], cluster.max_time[projection_type::DIR_W] ));

  ////////// DEBUG 
  //std::cout << Form(">>>> EventId = %d", event_id) << std::endl;
  //std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;
  ////////// DEBUG 

  const std::map<MultiKey2, std::vector<int> > & hitListByTimeDirMerged = cluster.GetHitListByTimeDirMerged();
  
  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
    if((hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_U))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_V))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_W))==hitListByTimeDirMerged.end())) continue;
    
    std::vector<int> hits[3] = {
      hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_U))->second,
      hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_V))->second,
      hitListByTimeDirMerged.find(MultiKey2(icell, projection_type::DIR_W))->second};
    /*
      const std::map<MultiKey2, std::vector<int> > & hitListByTimeDir = cluster.GetHitListByTimeDir();
      
      for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
      
      if((hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_U))==hitListByTimeDir.end()) ||
      (hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_V))==hitListByTimeDir.end()) ||
      (hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_W))==hitListByTimeDir.end())) continue;
      
      std::vector<int> hits[3] = {
      hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_U))->second,
      hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_V))->second,
      hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_W))->second};
    */
    ////////// DEBUG 
    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[projection_type::DIR_U].size(), (int)hits[projection_type::DIR_V].size(), (int)hits[projection_type::DIR_W].size()) << std::endl;
    ////////// DEBUG 
    
    // check if there is at least one hit in each direction
    if(hits[projection_type::DIR_U].size()==0 || hits[projection_type::DIR_V].size()==0 || hits[projection_type::DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])

    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      for(auto iter0=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_U).begin();
	  iter0!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_U).end(); iter0++) {
	std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(projection_type::DIR_U, *iter0, hits[0].at(i0));
	for(int i1=0; i1<(int)hits[1].size(); i1++) {
	  for(auto iter1=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_V).begin();
	      iter1!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_V).end(); iter1++) {
	    std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(projection_type::DIR_V, *iter1, hits[1].at(i1));
	    for(int i2=0; i2<(int)hits[2].size(); i2++) {
	      for(auto iter2=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_W).begin();
		  iter2!=myGeometryPtr->GetDirSectionIndexList(projection_type::DIR_W).end(); iter2++) {
		std::shared_ptr<StripTPC> strip2 = myGeometryPtr->GetStripByDir(projection_type::DIR_W, *iter2, hits[2].at(i2));
		
		////////// DEBUG 
		//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
		//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;
		////////// DEBUG 

		TVector2 pos;
		if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
		  (n_match[projection_type::DIR_U])[hits[0].at(i0)]++;
		  (n_match[projection_type::DIR_V])[hits[1].at(i1)]++;
		  (n_match[projection_type::DIR_W])[hits[2].at(i2)]++;
		  MultiKey3 mkey(hits[0].at(i0), hits[1].at(i1), hits[2].at(i2));
		  // accept only first matched 2D postion 
		  if(hitPos.find(mkey)!=hitPos.end()) continue;
		  hitPos[mkey]=pos;
		  ////////// DEBUG 
		  //	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
		  ////////// DEBUG 
		} else {
		  ////////// DEBUG 
		  //	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
		  ////////// DEBUG 
		}
	      }
	    }
	  }
	}
      }
    }
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    if(hitPos.size()<1) continue;

    // book 3D histogram before first fill
    if(h==NULL) h = myGeometryPtr->Get3DFrame(rebin_space, rebin_time); 
    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=std::get<0>(it1->first);
      int v1=std::get<1>(it1->first);
      int w1=std::get<2>(it1->first);
      std::vector<double> qtot = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      std::vector<double> q = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[projection_type::DIR_U] = GetValByStripMerged(projection_type::DIR_U, u1, icell);
      q[projection_type::DIR_V] = GetValByStripMerged(projection_type::DIR_V, v1, icell);
      q[projection_type::DIR_W] = GetValByStripMerged(projection_type::DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=std::get<0>(it2->first);
	int v2=std::get<1>(it2->first);
	int w2=std::get<2>(it2->first);
	
	if(u1==u2) {
	  qtot[projection_type::DIR_V] += GetValByStripMerged(projection_type::DIR_V, v2, icell);
	  qtot[projection_type::DIR_W] += GetValByStripMerged(projection_type::DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[projection_type::DIR_W] += GetValByStripMerged(projection_type::DIR_W, w2, icell);
	  qtot[projection_type::DIR_U] += GetValByStripMerged(projection_type::DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[projection_type::DIR_U] += GetValByStripMerged(projection_type::DIR_U, u2, icell);
	  qtot[projection_type::DIR_V] += GetValByStripMerged(projection_type::DIR_V, v2, icell);
	}
      }
      fraction[projection_type::DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_U] / qtot[projection_type::DIR_U] ));
      fraction[projection_type::DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_V] / qtot[projection_type::DIR_V] ));
      fraction[projection_type::DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[projection_type::DIR_W] / qtot[projection_type::DIR_W] ));
    }
    
    // loop over matched hits and fill histograms
    if(h) {

      std::map<MultiKey3, TVector2>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally
	  val = 
	    GetValByStripMerged(projection_type::DIR_U, std::get<0>(it->first), icell) / n_match[0].at(std::get<0>(it->first)) +
	    GetValByStripMerged(projection_type::DIR_V, std::get<1>(it->first), icell) / n_match[1].at(std::get<1>(it->first)) +
	    GetValByStripMerged(projection_type::DIR_W, std::get<2>(it->first), icell) / n_match[2].at(std::get<2>(it->first));
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions
	  val = 
	    GetValByStripMerged(projection_type::DIR_U, 
			  std::get<0>(it->first), icell)*0.5*( fraction[projection_type::DIR_V].at(it->first) + fraction[projection_type::DIR_W].at(it->first) ) +
	    GetValByStripMerged(projection_type::DIR_V, 
			  std::get<1>(it->first), icell)*0.5*( fraction[projection_type::DIR_W].at(it->first) + fraction[projection_type::DIR_U].at(it->first) ) +
	    GetValByStripMerged(projection_type::DIR_W, 
			  std::get<2>(it->first), icell)*0.5*( fraction[projection_type::DIR_U].at(it->first) + fraction[projection_type::DIR_V].at(it->first) );
	  break;
	  
	default: 
	  val=0.0;
	}; // end of switch (method)...
	Double_t z=myGeometryPtr->Timecell2pos(icell, err_flag);
	h->Fill( (it->second).X(), (it->second).Y(), z, val );
      }
    }
  }
  return h;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// overloading << operator
std::ostream& operator<<(std::ostream& os, const EventTPC& e) {
  os << "EventTPC: " << e.GetEventInfo();
  return os;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
