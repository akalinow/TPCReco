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

  Clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::Clear(){

  chargeMapWithSections.clear();
  chargeMapMergedSections.clear();

  keyLists.clear();
  asadMap.clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
  myGeometryPtr = aPtr;
  if(myGeometryPtr && !myGeometryPtr->IsOK()){
    throw std::logic_error("Geometry not initialised.");
  }
  if(myGeometryPtr) create3DHistoTemplate();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetChargeMap(const PEventTPC::chargeMapType & aChargeMap){

  Clear();

  chargeMapWithSections = aChargeMap;
  //fillMergedSectionsMap();


  filterHits(filter_type::none);
  updateHistosCache(filter_type::none);
  
  filterHits(filter_type::threshold);
  std::cout<<"Here 1"<<std::endl;
  updateHistosCache(filter_type::threshold);
  std::cout<<"Here 2"<<std::endl;

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

    if(iCell<0 || iCell>=myGeometryPtr->GetAgetNtimecells()) continue;
    
    for(int iStrip=std::get<2>(key)-delta_strips;
	iStrip<=std::get<2>(key)+delta_strips;++iStrip){

      if(iStrip<myGeometryPtr->GetDirMinStrip(iDir, iSection) ||
	 iStrip>myGeometryPtr->GetDirMaxStrip(iDir, iSection)) continue;
      auto neighbourKey = std::make_tuple(iDir, iSection, iStrip, iCell);
      if(chargeMapWithSections.find(neighbourKey)!=chargeMapWithSections.end()){
	  keyList.insert(neighbourKey);
	}
    }
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
  a3DHisto->Sumw2(true);
  a3DHistoRawPtr.reset(a3DHisto);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::updateHistosCache(filter_type filterType){

  std::shared_ptr<TH3D> aHisto((TH3D*)a3DHistoRawPtr->Clone());

  double x = 0.0, y = 0.0, z = 0.0, value=0.0;

  for(const auto & key: keyLists.at(filterType)){
    value = chargeMapWithSections.at(key);
    x = std::get<3>(key) + 1;
    y = std::get<2>(key) + 0;
    z = std::get<0>(key) + 1;
    value +=aHisto->GetBinContent(x, y, z);
    aHisto->SetBinContent(x, y, z, value);
  }
  a3DHistoRawMap[filterType] = aHisto;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
bool EventTPC::CheckAsadNboards() const {
  return myGeometryPtr->GetAsadNboards()==(int)asadMap.size();
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

  auto item = chargeMapMergedSections.find({strip_dir,  strip_number, time_cell});
  if(item!=chargeMapMergedSections.end()) return item->second;
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int aStrip_dir, int aStrip_section, int aStrip_number,
			      filter_type filterType) const {

  double result = 0.0;
  
  projection_type projType =projection_type::NONE;
  if(aStrip_dir==projection_type::DIR_U) projType = projection_type::DIR_TIME_U;
  else if(aStrip_dir==projection_type::DIR_V) projType = projection_type::DIR_TIME_V;
  else if(aStrip_dir==projection_type::DIR_W) projType = projection_type::DIR_TIME_W;

  if(aStrip_dir<0){
    result = a3DHistoRawMap.at(filterType)->GetMaximum();
  }
  else if(aStrip_section<0 && aStrip_number<0){
    result = get2DProjection(projType, filterType, scale_type::raw)->GetMaximum();
  }
  else if(aStrip_section<0){
    std::shared_ptr<TH2D> histo2d = get2DProjection(projType, filterType, scale_type::raw);
    histo2d->GetYaxis()->SetRangeUser(aStrip_number, aStrip_number);
    std::shared_ptr<TH1D> histo1d(histo2d->ProjectionX("_px"));
    result = histo1d->GetMaximum();
    histo2d->GetYaxis()->SetRange(0,0);
  }
  else{
    double value=0;
    int strip_dir=0, strip_section=0, strip_number=0;
    for(const auto & key: keyLists.at(filterType)){
      strip_dir = std::get<0>(key);
      strip_section  = std::get<1>(key);
      strip_number  = std::get<2>(key);
      if((strip_dir==aStrip_dir) &&
	 (strip_section==aStrip_section) &&
	 (strip_number==aStrip_number) ){
	value = chargeMapWithSections.at(key);      
	if(value>result) result = value;
      }
    }
  }
  return result;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::tuple<int,int> EventTPC::GetMaxChargePos(int aStrip_dir, filter_type filterType) const {
  
  std::shared_ptr<TH3D> histo3d = a3DHistoRawMap.at(filterType);
  if(aStrip_dir>0){
    histo3d->GetZaxis()->SetRangeUser(aStrip_dir, aStrip_dir);
  }
  int globalBin = histo3d->GetMaximumBin();

  int binX=0, binY=0, binZ=0;
  histo3d->GetBinXYZ(globalBin, binX, binY, binZ);
  int time_cell = histo3d->GetXaxis()->GetBinCenter(binX);
  int strip_number = histo3d->GetYaxis()->GetBinCenter(binY);

  histo3d->GetZaxis()->SetRangeUser(-1,-1);
  return std::make_tuple(time_cell, strip_number);
}
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
long EventTPC::GetMultiplicity(bool countHits,
			       int aStrip_dir, int aStrip_section, int aStrip_number, 
			       filter_type filterType) const{

  int counter = 0;

  if(!countHits && aStrip_dir>-1 && aStrip_section<0){
    projection_type projType =projection_type::NONE;
    if(aStrip_dir==projection_type::DIR_U) projType = projection_type::DIR_TIME_U;
    else if(aStrip_dir==projection_type::DIR_V) projType = projection_type::DIR_TIME_V;
    else if(aStrip_dir==projection_type::DIR_W) projType = projection_type::DIR_TIME_W;
    
    std::shared_ptr<TH2D> histo2d = get2DProjection(projType, filterType, scale_type::raw);
    std::shared_ptr<TH1D> histo1d(histo2d->ProjectionY("_py"));
    for(auto binX=1;binX<histo1d->GetNbinsX();++binX){
      counter += histo1d->GetBinContent(binX)!=0;
    }
  }
  else{
    std::set<int> strips;
    long int multiplexedPos = 0;
    int strip_dir=0, strip_section=0, strip_number=0, time_cell=0; 
    for(const auto & key: keyLists.at(filterType)){
      strip_dir = std::get<0>(key);
      strip_section  = std::get<1>(key);
      strip_number  = std::get<2>(key);
      time_cell  = std::get<3>(key);
      if((aStrip_dir<0 || strip_dir==aStrip_dir) &&
	 (aStrip_section<0 || strip_section==aStrip_section) &&
	 (aStrip_number<0 || strip_number==aStrip_number)
	 ) {
	if(countHits) multiplexedPos =
			1E9*strip_section*(aStrip_section>-1) +
			1E6*strip_dir +
			1E3*strip_number +
			time_cell;
	else multiplexedPos = 1E9*strip_section +  1E6*strip_dir +  1E3*strip_number;
	strips.insert(multiplexedPos);
      }
    }
    counter = strips.size();
  }
  return counter;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::tuple<int,int,int,int> EventTPC::GetSignalRange(int aStrip_dir, filter_type filterType) const{

  projection_type projType = projection_type::NONE;
  if(aStrip_dir==projection_type::DIR_U) projType = projection_type::DIR_TIME_U;
  else if(aStrip_dir==projection_type::DIR_V) projType = projection_type::DIR_TIME_V;
  else if(aStrip_dir==projection_type::DIR_W) projType = projection_type::DIR_TIME_W;

  int minBinX = -1, minBinY = -1;
  int maxBinX = -1, maxBinY = -1;
  int strip_number=0, time_cell=0;
  if(projType==projection_type::NONE){
    for(const auto & key: keyLists.at(filterType)){
      strip_number  = std::get<2>(key);
      time_cell = std::get<3>(key);
      if(minBinX==-1) minBinX = time_cell;
      if(minBinY==-1) minBinY = strip_number;
      minBinX = std::min(minBinX, time_cell);
      minBinY = std::min(minBinY, strip_number);
      maxBinX = std::max(maxBinX, time_cell);
      maxBinY = std::max(maxBinY, strip_number);
    }
  }
  else{  
    std::shared_ptr<TH2D> histo2d = get2DProjection(projType, filterType, scale_type::raw);
    double threshold = 0;
    minBinX = histo2d->FindFirstBinAbove(threshold, 1);
    minBinY = histo2d->FindFirstBinAbove(threshold, 2);
      
    maxBinX = histo2d->FindLastBinAbove(threshold, 1);
    maxBinY = histo2d->FindLastBinAbove(threshold, 2);
  }   
  return std::make_tuple(minBinX, maxBinX, minBinY, maxBinY);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH1D> EventTPC::get1DProjection(projection_type projType,
						filter_type filterType,
						scale_type scaleType) const{
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
						scale_type scaleType) const{
  TH2D *h2D = 0;
  std::shared_ptr<TH3D> a3DHistoRawPtr = a3DHistoRawMap.at(filterType);
  
  if(projType==projection_type::DIR_TIME_U){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_U)+1,
					 static_cast<int>(projection_type::DIR_U)+1);
  }
  else if(projType==projection_type::DIR_TIME_V){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_V)+1,
					 static_cast<int>(projection_type::DIR_V)+1);
  }
  else if(projType==projection_type::DIR_TIME_W){
    a3DHistoRawPtr->GetZaxis()->SetRange(static_cast<int>(projection_type::DIR_W)+1,
					 static_cast<int>(projection_type::DIR_W)+1);
  }
  h2D = (TH2D*)a3DHistoRawPtr->Project3D("yx");
  
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
  if(myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
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
  if(myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
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
