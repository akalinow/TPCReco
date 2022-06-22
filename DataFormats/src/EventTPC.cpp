#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>

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
  chargeMapMergedSections.clear();
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
  fillMergedSectionsMap();

  filterHits(filter_type::none);
  filterHits(filter_type::threshold);

  updateHistosCache(filter_type::none);
  updateHistosCache(filter_type::threshold);

  fillAuxMaps(filter_type::none);
    
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::fillMergedSectionsMap(){

  for(const auto & item: chargeMapWithSections){
    auto key =  item.first;
    auto value =  item.second;

    const int strip_dir = std::get<0>(key);
    const int strip_num = std::get<2>(key);
    const int time_cell = std::get<3>(key);

    chargeMapMergedSections[{strip_dir, strip_num, time_cell}] += value;
  }  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::fillAuxMaps(filter_type filterType){

  for(const auto & key: keyLists.at(filterType)){
    auto value =  chargeMapWithSections.at(key);

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
  
  if(value > max_charge.at(strip_dir) ) { 
    max_charge.at(strip_dir) = value;
    max_charge_timing.at(strip_dir) = time_cell;
    max_charge_strip.at(strip_dir) = strip_num;
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
/*
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
*/
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMultiplicity(int aStrip_dir, int aStrip_section, filter_type filterType) const{

  int counter = 0;
  int strip_dir=0, strip_section=0;
  for(const auto & key: keyLists.at(filterType)){
    strip_dir = std::get<0>(key);
    strip_section  = std::get<1>(key);
    if( (aStrip_dir<0 || strip_dir==aStrip_dir) &&
	(aStrip_section<0 || strip_section==aStrip_section)) ++counter;
  }
  return counter;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
/*
double EventTPC::GetMaxCharge(int aStrip_dir, int aStrip_section, 
			      int aStrip_number, int aTime_cell,
			      filter_type filterType) const{

  double value=0;
  int strip_dir=0, strip_section=0, strip_number=0, time_cell=0;
  double maxCharge=0;
  for(const auto & key: keyLists.at(filterType)){
    strip_dir = std::get<0>(key);
    strip_section  = std::get<1>(key);
    strip_number  = std::get<2>(key);
    time_cell  = std::get<3>(key);    
    if( (aStrip_dir<0 || strip_dir==aStrip_dir) &&
	(aStrip_number<0 || strip_number==aStrip_number) &&
	(aStrip_section<0 || strip_section==aStrip_section) &&
	(aTime_cell<0 || time_cell==aTime_cell) ){
	  
      if(aStrip_section<0) value = chargeMapMergedSections.at({strip_dir, strip_number, time_cell});
      else value = chargeMapWithSections.at(key);
      
      if(value>maxCharge) maxCharge = value;
    }
  }
  return maxCharge;
}
*/
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int aStrip_dir, int aStrip_section,
				int aStrip_number, int aTime_cell,
				filter_type filterType) const{

  double sum = 0;
  int strip_dir=0, strip_section=0, strip_number=0, time_cell=0;
  for(const auto & key: keyLists.at(filterType)){
    strip_dir = std::get<0>(key);
    strip_section  = std::get<1>(key);
    strip_number  = std::get<2>(key);
    time_cell = std::get<3>(key);
    if( (aStrip_dir<0 || strip_dir==aStrip_dir) &&
	(aStrip_number<0 || strip_number==aStrip_number) &&
	(aStrip_section<0 || strip_section==aStrip_section) &&
	(aTime_cell<0 || time_cell==aTime_cell) ) sum+=chargeMapWithSections.at(key);
  }
  return sum;  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::sumOverSections(const PEventTPC::chargeMapType::key_type & key,
				 filter_type filterType) const{

  double value = 0;
  int strip_dir = std::get<0>(key);
  int strip_number  = std::get<2>(key);
  int time_cell  = std::get<3>(key);
  
  auto key_section0 = std::make_tuple(strip_dir, 0, strip_number, time_cell);
  auto key_section1 = std::make_tuple(strip_dir, 1, strip_number, time_cell);
  auto key_section2 = std::make_tuple(strip_dir, 2, strip_number, time_cell);
  auto keyList = keyLists.at(filterType);

  if(std::find(keyList.begin(), keyList.end(), key_section0) != keyList.end()) value += chargeMapWithSections.at(key_section0);
  if(std::find(keyList.begin(), keyList.end(), key_section1) != keyList.end()) value += chargeMapWithSections.at(key_section1);
  if(std::find(keyList.begin(), keyList.end(), key_section2) != keyList.end()) value += chargeMapWithSections.at(key_section2);

  return value;
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
  std::string timeName = "time";
  std::string labelX = "", labelY = "", labelZ = "";
  std::string integratedOverTime = "";
  bool isTimeProjection = false;

  std::string filterName = "raw";
  if(filterType==filter_type::threshold) filterName="Threshold";
  else if(filterType==filter_type::island) filterName="Island";

  if(projType==projection_type::DIR_U ||
     projType==projection_type::DIR_V ||
     projType==projection_type::DIR_W){
    strip_dir = projType;
    timeName = "pro";
    integratedOverTime = "integrated over time";
    labelX = myGeometryPtr->GetDirName(strip_dir);
    labelY = "Charge/strip [arb.u.]";

    if(scaleType==scale_type::mm) labelX += " [mm]";
    else labelX += " [strip]";
  }
  else if(projType==projection_type::DIR_TIME){
    strip_dir = projection_type::DIR_TIME;
    isTimeProjection = true;
   }
  else if(projType==projection_type::DIR_TIME_U){
    strip_dir = projection_type::DIR_U;
    isTimeProjection = true;
  }
  else if(projType==projection_type::DIR_TIME_V){
    strip_dir = projection_type::DIR_V;
    isTimeProjection = true;
  }
  else if(projType==projection_type::DIR_TIME_W){
    strip_dir = projection_type::DIR_W;
    isTimeProjection = true;
  }

  if(isTimeProjection){
    timeName = "time";    
    labelX = "Time"; 
    labelY = "Charge/time bin [arb.u.]";
    if(scaleType==scale_type::mm) labelX += " [mm]";
    else labelX += " [bin]";
  }

  std::string projName = myGeometryPtr->GetDirName(strip_dir);
  std::string scaleName = "_";
  if(scaleType==scale_type::mm) scaleName="_mm_";

  std::string name = "h"+filterName+"_"+projName+timeName+scaleName+"evt"+std::to_string(event_id);
  std::string title = "Event-"+std::to_string(event_id)+" ";
  title+= "selected by "+filterName+ " from "+projName+ " ";
  title+= integratedOverTime;

if(h1->GetDimension()==2){
  name = "h"+filterName+"_"+projName+"_vs_"+timeName+scaleName+"evt"+std::to_string(event_id);
  title = "Event-"+std::to_string(event_id)+" selected by "+filterName+" from "+projName;
  labelX = "Time"; 
  labelY = projName;
  labelZ = "Charge [arb.u.]";
  if(scaleType==scale_type::mm){
    labelX += " [mm]";
    labelY += " [mm]";
  }
  else{
    labelX += " [bin]";
    labelY += " [strip]";
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
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// overloading << operator
std::ostream& operator<<(std::ostream& os, const EventTPC& e) {
  os << "EventTPC: " << e.GetEventInfo();
  return os;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
