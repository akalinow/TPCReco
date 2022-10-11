#include <iostream>

#include "StripResponseCalculator.h"
#include "GeometryTPC.h"
#include "PEventTPC.h"
#include "colorText.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TF2.h"
#include "TVector2.h"
#include "TVector3.h"
#include "TMath.h"
#include "TRandom3.h"
#include "TFile.h"

StripResponseCalculator::StripResponseCalculator(std::shared_ptr<GeometryTPC> aGeometryPtr,
                                                 int delta_strips, // consider +/- neighbour strips
						 int delta_timecells, // consider +/- neighbour time cells
						 int delta_pads, // consider +/- pads for the nearest strip section boundary
                                                 double sigma_xy, double sigma_z, // horizontal and vertical gaussian spread [mm]
						 const char *fname,
                                                 bool debug_flag) :
  myGeometryPtr(aGeometryPtr), Nstrips(abs(delta_strips)), Ntimecells(abs(delta_timecells)), Npads(abs(delta_pads)),
  sigma_xy(sigma_xy), sigma_z(sigma_z), has_UVWprojectionsRaw(false), has_UVWprojectionsInMM(false),
  debug_flag(debug_flag) {

  // sanity checks
  if(!myGeometryPtr) {
    std::cout<<__FUNCTION__<<KRED<<" No valid geometry pointer!"<<RST<<std::endl;
    exit(-1);
  }
  if(delta_strips<=0 || sigma_xy<=0) {
    std::cout<<__FUNCTION__<<KRED<<": Wrong horizontal smearing parameters!"<<RST<<std::endl;
    exit(-1);
  }
  if(delta_timecells<=0 || sigma_z<=0) {
    std::cout<<__FUNCTION__<<KRED<<": Wrong vertical smearing parameters!"<<RST<<std::endl;
    exit(-1);
  }
  if(fname==NULL || strlen(fname)==0 || (strlen(fname)>0 && !loadHistograms(fname))) {
    initializeStripResponse();
    initializeTimeResponse();
  }
  ////// DEBUG
  for(auto & it : responseMapPerMergedStrip) {
    if(debug_flag) {
      std::cout << __FUNCTION__ << ": " << it.second->GetName() << ", integral=" << it.second->Integral() << std::endl;
    }
  }
  ////// DEBUG
}

bool StripResponseCalculator::loadHistograms(const char *fname) {
  TFile f(fname, "OLD");
  if(!f.IsOpen()) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot open TFile: "<<fname<<"!"<<RST<<std::endl;
    return false;
  }

  // initilaize strip domain histograms (relative merged strip's index wrt reference node)
  for(auto & it : responseMapPerMergedStrip) {
    if(it.second) delete it.second;
  }
  responseMapPerMergedStrip.clear();
  for(int istrip=-Nstrips; istrip<=Nstrips; istrip++) {
    for(int strip_dir=DIR_U; strip_dir<=DIR_W; strip_dir++) {
      auto hist=(TH2D*)f.Get(getStripResponseHistogramName(strip_dir, istrip));
      if(!hist) {
	if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot find histogram: "<<getStripResponseHistogramName(strip_dir, istrip)<<"!"<<RST<<std::endl;
	return false;
      }
      hist->SetDirectory(0); // do not assiciate this histogram with any TFile directory
      responseMapPerMergedStrip[MultiKey2(strip_dir, istrip)]=hist;
    }
  }

  // initilaize strip domain histograms (relative strip section start in pad units wrt reference node)
  for(auto & it : responseMapPerStripSectionStart) {
    if(it.second) delete it.second;
  }
  responseMapPerStripSectionStart.clear();
  for(int istrip=-Nstrips; istrip<=Nstrips; istrip++) {
    for(int strip_dir=DIR_U; strip_dir<=DIR_W; strip_dir++) {
      if(myGeometryPtr->GetDirNSections(strip_dir)<2) continue; // just one section per strip direction
      for(int ipad=-Npads; ipad<=Npads+abs((istrip%2)); ipad++) { // add 1 extra pad for odd relative strip index
	auto hist=(TH2D*)f.Get(getStripSectionStartResponseHistogramName(strip_dir, istrip, ipad));
	if(!hist) {
	  if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot find histogram: "<<getStripSectionStartResponseHistogramName(strip_dir, istrip, ipad)<<"!"<<RST<<std::endl;
	  return false;
	}
	hist->SetDirectory(0); // do not assiciate this histogram with any TFile directory
	responseMapPerStripSectionStart[MultiKey3(strip_dir, istrip, ipad)]=hist;
	////// DEBUG
	if(debug_flag) std::cout << __FUNCTION__ << ": dir=" << strip_dir << ", delta_strip=" << istrip << ", delta_pad=" << ipad << ", hist=" << hist << std::endl;
	////// DEBUG
      }
    }
  }

  // initialize time domain
  for(auto & it : responseMapPerTimecell) {
    if(it.second) delete it.second;
  }
  responseMapPerTimecell.clear();
  for(auto icell=-Ntimecells; icell<=Ntimecells; icell++) {
    auto hist=(TH1D*)f.Get(getTimeResponseHistogramName(icell));
    if(!hist) {
      if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot find histogram: "<<getTimeResponseHistogramName(icell)<<"!"<<RST<<std::endl;
      return false;
    }
    hist->SetDirectory(0); // do not assiciate this histogram with any TFile directory
    responseMapPerTimecell[icell]=hist;
  }

  f.Close();

  ////// DEBUG
  for(auto & it : responseMapPerMergedStrip) {
    if(debug_flag) {
      std::cout << __FUNCTION__ << ": " << it.second->GetName() << ", integral=" << it.second->Integral() << std::endl;
    }
  }
  ////// DEBUG

  return true;
}
bool StripResponseCalculator::saveHistograms(const char *fname) {
  TFile f(fname, "CREATE");
  if(!f.IsOpen()) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot create new TFile: "<<fname<<"!"<<RST<<std::endl;
    return false;
  }
  f.cd();
  for(auto & it : responseMapPerMergedStrip) {
    it.second->SetTitleOffset(1.3, "X");
    it.second->SetTitleOffset(1.5, "Y");
    it.second->SetTitleOffset(1.5, "Z");
    it.second->SetOption("COLZ");
    it.second->Write();
  }
  for(auto & it : responseMapPerStripSectionStart) {
    it.second->SetTitleOffset(1.3, "X");
    it.second->SetTitleOffset(1.5, "Y");
    it.second->SetTitleOffset(1.5, "Z");
    it.second->SetOption("COLZ");
    it.second->Write();
  }
  for(auto & it : responseMapPerTimecell) {
    it.second->SetTitleOffset(1.3, "X");
    it.second->SetTitleOffset(1.5, "Y");
    it.second->SetOption("HISTO");
    it.second->Write();
  }
  f.Close();
  return true;
}

// declare new array of UZ/VZ/WZ histograms to be filled (channel vs time cell)
bool StripResponseCalculator::setUVWprojectionsRaw(std::vector<TH2D*> aUVWprojectionsRaw) {
  has_UVWprojectionsRaw=false;
  if(aUVWprojectionsRaw.size()!=3) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid size of TH2D histogram array!"<<RST<<std::endl;
    return false;
  }
  for(auto & it : aUVWprojectionsRaw) {
    if(!it) {
      if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid pointer to TH2D histogram!"<<RST<<std::endl;
      return false;
    }
  }
  fillUVWprojectionsRaw.clear();
  fillUVWprojectionsRaw=aUVWprojectionsRaw;
  has_UVWprojectionsRaw=true;
  return true;
}

bool  StripResponseCalculator::setUVWprojectionsRaw(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsRaw) { // pair={STRIP_DIR, TH2D pointer}
  std::vector<TH2D*> vec;
  for(auto & hist : aUVWprojectionsRaw) {
    vec.push_back(hist.get());
  }
  return setUVWprojectionsRaw(vec);
}

// declare new array of UZ/VZ/WZ histograms to be filled (in mm)
bool StripResponseCalculator::setUVWprojectionsInMM(std::vector<TH2D*> aUVWprojectionsInMM) {
  has_UVWprojectionsInMM=false;
  if(aUVWprojectionsInMM.size()!=3) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid size of TH2D histogram array!"<<RST<<std::endl;
    return false;
  }
  for(auto & it : aUVWprojectionsInMM) {
    if(!it) {
      if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid pointer to TH2D histogram!"<<RST<<std::endl;
      return false;
    }
  }
  fillUVWprojectionsInMM.clear();
  fillUVWprojectionsInMM=aUVWprojectionsInMM;
  has_UVWprojectionsInMM=true;
  return true;
}

bool  StripResponseCalculator::setUVWprojectionsInMM(std::vector<std::shared_ptr<TH2D> > aUVWprojectionsInMM) { // pair={STRIP_DIR, TH2D pointer}
  std::vector<TH2D*> vec;
  for(auto & hist : aUVWprojectionsInMM) {
    vec.push_back(hist.get());
  }
  return setUVWprojectionsInMM(vec);
}

// fill all declared objects
void StripResponseCalculator::addCharge(TVector3 position3d, double charge, std::shared_ptr<PEventTPC> aEventPtr) {
  addCharge(position3d.X(), position3d.Y(), position3d.Z(), charge, aEventPtr);
}

void StripResponseCalculator::addCharge(double x, double y, double z, double charge, std::shared_ptr<PEventTPC> aEventPtr) {

  if(charge==0.0 || (!has_UVWprojectionsRaw && !has_UVWprojectionsInMM && !aEventPtr)) return; // nothing to do

  // strip domain
  auto refNodePosInMM=TVector2(0,0);
  const auto refStrips = getReferenceStripNode(x, y, &refNodePosInMM); // vector with {u0, v0, w0} triplet, range [1-1024]
  auto err=(refStrips.size()!=3);
  if(err) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid charge XY-position!"<<RST<<std::endl;
    return;
  }
  // time domain
  const auto refCell = getReferenceTimecell(z); // time cell t0, range [0-511]
  double refCellPosInMM(0); // lower end of time cell t0 converted to mm
  refCellPosInMM=myGeometryPtr->Timecell2pos(refCell, err);
  if(err) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<" Invalid charge Z-position!"<<RST<<std::endl;
    return;
  }
  
  // calculate relative positions in mm wrt reference node / reference time cell
  const auto dx = x-refNodePosInMM.X(); // wrt nearest UVW strip node
  const auto dy = y-refNodePosInMM.Y(); // wrt nearest UVW strip node
  const auto dz = z-refCellPosInMM; // wrt beginning of the time cell

  ////// DEBUG
  if(debug_flag) std::cout << __FUNCTION__ << ": Relative postion=[dx=" << dx << ", dy=" << dy << ", dz=" << dz << "]"
			   << ", Charge=" << charge
			   << ", has_UVWprojectionsRaw=" << has_UVWprojectionsRaw
			   << ", has_UVWprojectionsInMM=" << has_UVWprojectionsInMM
			   << std::endl;
  ////// DEBUG

  // fill all declared UZ, VZ, WZ projection histograms
  err=false;
  for(auto & respXY : responseMapPerMergedStrip) {
    const auto smeared_strip_dir= std::get<0>(respXY.first); // DIR index
    const auto smeared_strip_num=refStrips[smeared_strip_dir]+std::get<1>(respXY.first); // STRIP index
    const auto smeared_pos=myGeometryPtr->Strip2posUVW(smeared_strip_dir, smeared_strip_num, err);
    if(err) continue;

    const auto smeared_bin=respXY.second->FindBin(dx ,dy);
    const auto smeared_fractionXY=respXY.second->GetBinContent(smeared_bin);
    if(smeared_fractionXY<Utils::NUMERICAL_TOLERANCE) continue; // speeds up filling

    // calculate map of total charge fractions per strip section for merged strip {strip_dir, strip_number}
    // * step-1 - use look-up table of section boundary points corresponding to the same merged strip
    // * step-2 - initialize all sections with charge fractions taken from the merged strip (TOTAL)
    // * step-3 - for each section boundary point (START or END) compute delta_pad index
    // * step-4 - if delta_pad is within region of interest then assign charge fraction to adjacent strip sections as:
    //             f and TOTAL-f according to respective 2D response per section histogram[delta_pad]
    // * step-5 - if delta_pad is outside of region of interest then zero one section and leave the other one as-is
    //
    std::map<int, double> fractionPerSectionMap;
    auto list=myGeometryPtr->GetStripSectionBoundaryList(smeared_strip_dir, smeared_strip_num);

    ////// DEBUG
    if(debug_flag) {
      std::cout << __FUNCTION__ << ": Merged strip "
		<< myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
		<< " [dir="<<smeared_strip_dir<<", num="<<smeared_strip_num<<"]"
		<< ": Number of boundary points=" << list.size() << ":" << std::endl;
      for(auto & it : list) {
	std::cout << "    point=[" << it.pos.X() << ", " << it.pos.Y() << "], prev_sec=" << it.previous << ", next_sec=" << it.next << std::endl;
      }
    }
    ////// DEBUG

    // initialize the map with total fraction per merged strip
    fractionPerSectionMap.clear();
    for(auto & it : list) {
      if(it.next!=GeometryTPC::outside_section && fractionPerSectionMap.find(it.next)==fractionPerSectionMap.end())
	fractionPerSectionMap[it.next]=smeared_fractionXY;
      if(it.previous!=GeometryTPC::outside_section && fractionPerSectionMap.find(it.previous)==fractionPerSectionMap.end())
	fractionPerSectionMap[it.previous]=smeared_fractionXY;
    }

    ////// DEBUG
    if(debug_flag) {
      for(auto & it : fractionPerSectionMap) {
	std::cout << __FUNCTION__ << ": Unmerged strip "
		  << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
		  << " [dir="<<smeared_strip_dir<<", sec="<<it.first<<", num="<<smeared_strip_num<<"]"
		  << ": initial fractionPerSectionMap[sec=" << it.first << "] = " << it.second << std::endl;
      }
    }
    ////// DEBUG

    // subtract charge per section according to relative position of the section's boundary wrt reference node
    for(auto & it : list) {
      const auto delta_pads=(int)TMath::Ceil( ((it.pos - refNodePosInMM)*myGeometryPtr->GetStripUnitVector(smeared_strip_dir))/myGeometryPtr->GetPadPitch()
					      + ( (smeared_strip_num-refStrips[smeared_strip_dir])%2 == 0 ? 0.0 : 0.5) );
      // delta_pads outside range [-Npads, Npads+eps]
      if(delta_pads<-Npads || delta_pads>Npads+abs(smeared_strip_num-refStrips[smeared_strip_dir])%2) {

	if(delta_pads<-Npads && it.previous!=GeometryTPC::outside_section) {

	  ////// DEBUG
	  if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
				   << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
				   << " [dir="<<smeared_strip_dir<<", sec="<<it.previous<<", num="<<smeared_strip_num<<"]"
				   << ": delta_pads=" << delta_pads
				   << " => zeroing sec=" << it.previous << ": new_val=0 (old=" << fractionPerSectionMap[it.previous] << ")" << std::endl;
	  ////// DEBUG

	  fractionPerSectionMap[it.previous]=0.0; // no impact on section it.next
	}
	if(delta_pads>Npads+abs(smeared_strip_num-refStrips[smeared_strip_dir])%2 && it.next!=GeometryTPC::outside_section) {

	  ////// DEBUG
	  if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
				   << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
				   << " [dir="<<smeared_strip_dir<<", sec="<<it.next<<", num="<<smeared_strip_num<<"]"
				   << ": delta_pads=" << delta_pads
				   << " => zeroing sec=" << it.next << ": new_val=0 (old=" << fractionPerSectionMap[it.next] << ")" << std::endl;
	  ////// DEBUG

	  fractionPerSectionMap[it.next]=0.0; // no impact on section it.previous
	}
	continue; // delta_pads is outside mapping range
      }
      // delta_pads is inside [-Npads, Npads+eps]
      const auto it2 = responseMapPerStripSectionStart.find(MultiKey3(smeared_strip_dir, smeared_strip_num-refStrips[smeared_strip_dir], delta_pads));

      if(it.previous!=GeometryTPC::outside_section) {
	fractionPerSectionMap[it.previous] -= smeared_fractionXY-it2->second->GetBinContent(smeared_bin);

	////// DEBUG
	if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
				 << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
				 << " [dir="<<smeared_strip_dir<<", sec="<<it.previous<<", num="<<smeared_strip_num<<"]"
				 << ": delta_pads=" << delta_pads
				 << " => subtracting from sec=" << it.previous << ": new_val=" << fractionPerSectionMap[it.previous]
				 << std::endl;
	////// DEBUG
      }
      if(it.next!=GeometryTPC::outside_section) {
	fractionPerSectionMap[it.next] -= it2->second->GetBinContent(smeared_bin);

	////// DEBUG
	if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
				 << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
				 << " [dir="<<smeared_strip_dir<<", sec="<<it.next<<", num="<<smeared_strip_num<<"]"
				 << ": delta_pads=" << delta_pads
				 << " => subtracting from sec=" << it.next << ": new_val=" << fractionPerSectionMap[it.next]
				  << std::endl;
	////// DEBUG
      }
    }
    // final cross-check
    auto smeared_fractionXY_sum=0.0;
    for(auto & it2 : fractionPerSectionMap) {
      if(it2.second<0.0) it2.second=0.0;
      auto smeared_fractionXY_per_section = it2.second;
      smeared_fractionXY_sum += smeared_fractionXY_per_section;

      ////// DEBUG
      if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
			       << myGeometryPtr->GetStripName(myGeometryPtr->GetStripByDir(smeared_strip_dir, it2.first, smeared_strip_num))
			       << " [dir="<<smeared_strip_dir<<", sec="<<it2.first<<", num="<<smeared_strip_num<<"]"
			       << ": total_charge_fraction=" << smeared_fractionXY_per_section
			       << ", merged_strip_fraction=" << smeared_fractionXY_per_section/smeared_fractionXY << std::endl;
      ////// DEBUG
    }
    ////// DEBUG
    if(debug_flag) std::cout << __FUNCTION__
			     << ": Merged strip " << myGeometryPtr->GetDirName(smeared_strip_dir) << smeared_strip_num
			     << " [dir="<<smeared_strip_dir<<", num="<<smeared_strip_num<<"]"
			     << ": total_charge_fraction=" << smeared_fractionXY_sum
			     << ", merged_strip_fraction=" << smeared_fractionXY_sum/smeared_fractionXY
			     << " (expected 1)" << std::endl;
    ////// DEBUG

    for(auto & respZ : responseMapPerTimecell) {
      const auto smeared_fractionZ=respZ.second->GetBinContent(respZ.second->FindBin(dz));
      const auto smeared_charge=charge*smeared_fractionZ*smeared_fractionXY;
      if(smeared_charge<Utils::NUMERICAL_TOLERANCE) continue; // speeds up filling
      const auto smeared_timecell=refCell+respZ.first;

      if(has_UVWprojectionsRaw) {
	fillUVWprojectionsRaw[smeared_strip_dir]->Fill(smeared_timecell*1., smeared_strip_num*1., smeared_charge);
	////// DEBUG
	//	if(debug_flag) std::cout << __FUNCTION__
	//				 << ": dir=" << smeared_strip_dir
	//				 << ", strip=" << smeared_strip_num << " (delta=" << respXY.first.key2
	//				 << "), cell=" << smeared_timecell << " (delta=" << respZ.first
	//				 << "), charge=" << smeared_charge << std::endl;
	////// DEBUG
      }
      
      if(has_UVWprojectionsInMM) {
	const auto smeared_z=myGeometryPtr->Timecell2pos(smeared_timecell, err);
	if(err) continue;
	fillUVWprojectionsInMM[smeared_strip_dir]->Fill(smeared_z, smeared_pos, smeared_charge);
	////// DEBUG
	//	if(debug_flag) std::cout << __FUNCTION__
	//				 << ": dir=" << smeared_strip_dir
	//				 << ", strip=" << smeared_strip_num << " (delta=" << respXY.first.key2
	//				 << ", posUVW=" << myGeometryPtr->Cartesian2posUVW(x, y, smeared_strip_dir, err)
	//				 << "), cell=" << smeared_timecell << " (delta=" << respZ.first
	//				 << ", z=" << smeared_z
	//				 << "), charge=" << smeared_charge << std::endl;
	////// DEBUG
      }

      // fill charge per {strip DIR, strip NUM, strip SECTION, time CELL} quadruplet
      if(aEventPtr) {
	for(auto & it2 : fractionPerSectionMap) {
	  const auto smeared_charge_per_section = charge*smeared_fractionZ*it2.second;
	  if(smeared_charge_per_section<Utils::NUMERICAL_TOLERANCE) continue; // speeds up filling
	  std::shared_ptr<StripTPC> aStrip = myGeometryPtr->GetStripByDir(smeared_strip_dir, it2.first, smeared_strip_num);
	  aEventPtr->AddValByStrip(aStrip, smeared_timecell, smeared_charge_per_section);

	  ////// DEBUG
	  //	  	  if(debug_flag) std::cout << __FUNCTION__ << ": Unmerged strip "
	  //	  				   << myGeometryPtr->GetStripName(myGeometryPtr->GetStripByDir(smeared_strip_dir, it2.first, smeared_strip_num))
	  //	  				   << " [dir="<<smeared_strip_dir<<", sec="<<it2.first<<", num="<<smeared_strip_num<<"]"
	  //	  				   << ": charge=" << smeared_charge_per_section
	  //	  				   << ", total_charge_fraction=" << smeared_charge_per_section/charge << std::endl;
	  ////// DEBUG
	}
      }
    }
  }
}

// returns vector with {u0, v0, w0} triplet corresponding to the nearest node in XY plane
std::vector<int> StripResponseCalculator::getReferenceStripNode(TVector3 position3d, TVector2 *refNodePosInMM) const {
  return getReferenceStripNode(position3d.X(), position3d.Y(), refNodePosInMM);
}

std::vector<int> StripResponseCalculator::getReferenceStripNode(TVector2 position2d, TVector2 *refNodePosInMM) const {
  return getReferenceStripNode(position2d.X(), position2d.Y(), refNodePosInMM);
}

std::vector<int> StripResponseCalculator::getReferenceStripNode(double x, double y, TVector2 *refNodePosInMM) const {
  std::vector<int> result;
  auto strip=myGeometryPtr->GetTH2PolyStrip( myGeometryPtr->GetTH2Poly()->FindBin(x, y) );
  if(!strip) {
    if(debug_flag) std::cout << __FUNCTION__
			     << KRED << ": No matching strips for given XY position!"
			     << RST << std::endl;
    return result; // return empty vector on error
  }
  // compute Cartesian position of the nearest node
  std::map<int, int> stripMap; // DIR index, STRIP index
  const auto strip_dir=strip->Dir();
  stripMap[strip_dir]=strip->Num();

  ////// DEBUG
  if(debug_flag) std::cout << __FUNCTION__ << ": strip_dir=" << strip_dir << ", strip_num=" << stripMap[strip_dir] << std::endl;
  ////// DEBUG

  const auto strip_startPos=strip->Start(); // absolute position of strip's starting point
  const auto distance_to_start=(TVector2(x,y)-strip_startPos)*myGeometryPtr->GetStripUnitVector(strip_dir); // position wrt start
  auto distance_to_node=fmod(distance_to_start, myGeometryPtr->GetPadPitch()); // position wrt start
  if(distance_to_node>0.5*myGeometryPtr->GetPadPitch()) distance_to_node-=myGeometryPtr->GetPadPitch(); // range [-0.5*pad_pitch, 0.5*pad_pitch]
  const auto nodePos=strip_startPos+myGeometryPtr->GetStripUnitVector(strip_dir)*(distance_to_start-distance_to_node); // absolute position of the nearest strip node

  // find strip numbers for 2 complementary strip directions
  for(int check_dir=DIR_U; check_dir<=DIR_W; check_dir++) {
    if(check_dir==strip_dir) continue;

    for(auto isign=-1; isign<=1; isign+=2) { // probe 2 adjacent pads for each direction index
      const auto checkPos=nodePos+myGeometryPtr->GetStripUnitVector(check_dir)*0.5*myGeometryPtr->GetPadPitch()*isign;
      const auto check_strip=myGeometryPtr->GetTH2PolyStrip( myGeometryPtr->GetTH2Poly()->FindBin(checkPos.X(), checkPos.Y()) );
      if(!check_strip) continue;
      stripMap[check_dir]=check_strip->Num();
  
      if(debug_flag) std::cout << __FUNCTION__ <<": strip_dir=" << check_dir << ", strip_num=" << stripMap[check_dir] << std::endl;
  
      break;
    }
  }
  if(stripMap.size()!=3) {
    if(debug_flag) std::cout <<__FUNCTION__
			     << KRED << ": Found only "<<stripMap.size()<<" matching strips out of 3 for given XY position!"
			     << RST << std::endl;
    return result; // return empty vector on error
  }
  // fill resulting vector with {u0, v0, w0} triplet corresponding to the nearest node
  result.resize(3, 0);
  for(auto & it : stripMap) {
    result.at(it.first)=it.second;
  }
  // optionally return absolute Cartesian coordinates of the strip node
  if(refNodePosInMM) *refNodePosInMM=nodePos;
  return result;
}

// returns t0 time cell index corresponding to Z position
int StripResponseCalculator::getReferenceTimecell(TVector3 position3d) const {
  return getReferenceTimecell(position3d.Z());
}

int StripResponseCalculator::getReferenceTimecell(double z) const {
  auto err=false;
  return myGeometryPtr->Pos2timecell(z, err);
}

// re-generate XY response histograms with arbitrary granularity
void StripResponseCalculator::initializeStripResponse(unsigned long NpointsXY, int NbinsXY) {
  // initilaize strip domain histograms (relative merged strip's index wrt reference node)
  for(auto & it : responseMapPerMergedStrip) {
    if(it.second) delete it.second;
  }
  responseMapPerMergedStrip.clear();

  // Each histogram corresponds to a single strip given by {relative strip index wrt reference node, strip direction} pair.
  // Center of each bin corresponds to the mean XY position [mm] wrt reference node of 2D normal distribution with spread sigmaXY.
  // Fill each bin with fraction of the charge corresponding to the total strip area.
  // Integration is done using Monte Carlo technique.
  //
  // create empty 2D response histograms
  for(int istrip=-Nstrips; istrip<=Nstrips; istrip++) {
    for(int strip_dir=DIR_U; strip_dir<=DIR_W; strip_dir++) {
      auto hist=
	new TH2D(getStripResponseHistogramName(strip_dir, istrip),
		 Form("Horizontal response for %s-strip %s%d;#Deltax wrt nearest strip node [mm];#Deltay wrt nearest node [mm];Charge fraction [arb.u.]",
		      myGeometryPtr->GetDirName(strip_dir),
		      (istrip==0 ? "" : (istrip<0 ? "-" : "+")), abs(istrip)),
		 NbinsXY, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize(),
		 NbinsXY, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize());
      responseMapPerMergedStrip[MultiKey2(strip_dir, istrip)]=hist;
    }
  }

  // initilaize strip domain histograms (relative strip section start in pad units wrt reference node)
  for(auto & it : responseMapPerStripSectionStart) {
    if(it.second) delete it.second;
  }
  responseMapPerStripSectionStart.clear();
  for(int istrip=-Nstrips; istrip<=Nstrips; istrip++) {
    for(int strip_dir=DIR_U; strip_dir<=DIR_W; strip_dir++) {
      //      if(myGeometryPtr->GetDirSectionIndexList(strip_dir).size()<2) continue; // just one section per strip direction
      //      if(myGeometryPtr->GetDirNSections(strip_dir)<2) continue; // just one section per strip direction
      for(int ipad=-Npads; ipad<=Npads+abs((istrip%2)); ipad++) { // add 1 extra pad for odd relative strip index
	auto hist=
	  new TH2D(getStripSectionStartResponseHistogramName(strip_dir, istrip, ipad),
		   Form("Horizontal response for %s-strip %s%d | Section @ Pad%s%d;#Deltax wrt nearest strip node [mm];#Deltay wrt nearest node [mm];Charge fraction [arb.u.]",
			myGeometryPtr->GetDirName(strip_dir),
			(istrip==0 ? "" : (istrip<0 ? "-" : "+")), abs(istrip),
			(ipad==0 ? "" : (ipad<0 ? "-" : "+")), abs(ipad)),
		   NbinsXY, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize(),
		   NbinsXY, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize());
	responseMapPerStripSectionStart[MultiKey3(strip_dir, istrip, ipad)]=hist;

	////// DEBUG
	//	if(debug_flag) std::cout << __FUNCTION__ << ": responseMapPerSectionStripStart [dir=" << strip_dir << ", delta_strip=" << istrip << ", delta_pad=" << ipad << "]=" << hist << std::endl;
	////// DEBUG
      }
    }
  }

  ////// DEBUG
  if(debug_flag) {
    std::cout << __FUNCTION__ << ": responseMapPerMergedStrip size=" << responseMapPerMergedStrip.size() << std::endl;
    std::cout << __FUNCTION__ << ": responseMapPerSectionStripStart size=" << responseMapPerStripSectionStart.size() << std::endl;
  }
  ////// DEBUG

  // find central node of UVW active area
  double xmin, xmax, ymin, ymax;
  std::tie(xmin, xmax, ymin, ymax)=myGeometryPtr->rangeXY();
  TVector2 refNodePosInMM(0,0);
  const auto refStrips = getReferenceStripNode(0.5*(xmin+xmax), 0.5*(ymin+ymax), &refNodePosInMM); // vector with {u0, v0, w0} triplet, range [1-1024]
  bool err=(refStrips.size()!=3);
  if(err) {
    std::cout<<__FUNCTION__<<KRED<<": Cannot find central node!"<<RST<<std::endl;
    exit(-1);
  }

  // generate random points around central node from 2D normal distribution
  // and fill 2D response histograms
  TF2 gauss2d("f_gauss2d", [](double* x, double*p) { return 1./TMath::TwoPi()/p[0]/p[0] * exp( -0.5*(x[0]*x[0]+x[1]*x[1])/p[0]/p[0] ); },
	      -5*sigma_xy, 5*sigma_xy,
	      -5*sigma_xy, 5*sigma_xy, 1, 2); // 1 parameter, 2 dimensions
  gauss2d.SetParameter(0, sigma_xy);
  gauss2d.SetNpx(NbinsXY*10);
  gauss2d.SetNpy(NbinsXY*10);
  const auto weight=1./(double)NpointsXY;
  const auto hist=responseMapPerMergedStrip.begin()->second;
  ////// DEBUG - optimized for speed
  const auto R2=myGeometryPtr->GetPadSize()*myGeometryPtr->GetPadSize() + hist->GetXaxis()->GetBinWidth(1)*hist->GetYaxis()->GetBinWidth(1);
  ////// DEBUG - optimized for speed
  for(unsigned long ipoint=0L; ipoint<NpointsXY; ipoint++) {
    double delta_x, delta_y;
    gauss2d.GetRandom2(delta_x, delta_y);
    if(debug_flag && ( (ipoint<1000 && ipoint%100==0) ||
		       ipoint%1000==0 )) {
      std::cout << __FUNCTION__ << ": Generating point=" << ipoint << std::endl;
    }
    for(auto ibin1=1; ibin1<=hist->GetNbinsX(); ibin1++) {
      double c1=hist->GetXaxis()->GetBinCenter(ibin1);
      for(auto ibin2=1; ibin2<=hist->GetNbinsY(); ibin2++) {
	double c2=hist->GetYaxis()->GetBinCenter(ibin2);
	////// DEBUG - optimized for speed
	if(c1*c1+c2*c2>R2) continue; // stay within radius of PAD SIZE
	////// DEBUG - optimized for speed
        const auto x=c1+delta_x; // [mm] wrt reference strip node
	const auto y=c2+delta_y; // [mm] wrt reference strip node
	const auto strip=myGeometryPtr->GetTH2PolyStrip( myGeometryPtr->GetTH2Poly()->FindBin(refNodePosInMM.X()+x, refNodePosInMM.Y()+y) );
	if(!strip) continue;

	// fill charge fraction for merged strips
	const auto it=responseMapPerMergedStrip.find(MultiKey2(strip->Dir(), strip->Num()-refStrips[strip->Dir()]));
	if(it==responseMapPerMergedStrip.end()) continue;
	it->second->Fill(c1, c2, weight);

	// fill charge fraction contained in the 1st section wrt total charge of the merged strip (two sections)
	// when the 1st section ends / 2nd section starts between -Npads and +Npads from the reference strip node postion:
	// * step-1 - each random hit is projected onto the reference strip axis
	// * step-2 - relative pad number is computed (delta_pad) for each relative strip index
	// * step-3A - EVEN relative strip index: delta_pad=0 if projection of the start of the 1st pad of the next section
	//   is loacated at the reference node position
	// * step-3B - ODD relative strip index: delta_pad=0 if projection of the centre of 1st pad of the next section
	//   is located at the reference node position (i.e. shift by 0.5*pad_length towards lower pad numbers of a given strip)
	// * step-4 - populate histograms with section's starting pad postion ranging from MAX(delta_pad, -Npads) to (Npads+eps),
	//   where: eps=0 for EVEN relative strip index. 1 for ODD relative strip index
	/*
	      ___+3         ___+3    Nstrips=1 x Npads=2 case:
	     /\      ___+2 /\
	     \/__+2 /\     \/__+2    REF strip node point is at
             /\     \/     /\        (relative strip index)=0, (relative pad start index)=0
	     \/     /\     \/
	     /\     \/__0  /\
	     \/__0  /\ REF \/__0
	     /\     \/     /\         A  strip_unit_vector
	     \/     /\     \/         |
	     /\     \/__-2 /\         |
             \/__-2        \/__-2     |
	                              +------->>  strip_pitch_vector
             odd    even   odd
	     N-1    N+0    N+1
	*/
	const auto delta_pads=(int)TMath::Ceil( (TVector2(x,y)*myGeometryPtr->GetStripUnitVector(strip->Dir()))/myGeometryPtr->GetPadPitch()
					    + ( (strip->Num()-refStrips[strip->Dir()])%2 == 0 ? 0.0 : 0.5) );
	for(auto ipad=std::max(-Npads, delta_pads); ipad<=Npads+abs(strip->Num()-refStrips[strip->Dir()])%2; ipad++) {
	  const auto it2=responseMapPerStripSectionStart.find(MultiKey3(strip->Dir(), strip->Num()-refStrips[strip->Dir()], ipad));
	  if(it2==responseMapPerStripSectionStart.end()) continue;
	  it2->second->Fill(c1, c2, weight);
	}
      }
    }
  }

  ////// DEBUG
  if(debug_flag) {
    auto hist=responseMapPerMergedStrip.begin()->second; // same bins for: responseMapPerMergedStrip, responseMapPerStripSectionStart
    for(auto ibin1=1; ibin1<=hist->GetNbinsX(); ibin1++) {
      auto c1=hist->GetXaxis()->GetBinCenter(ibin1);
      for(auto ibin2=1; ibin2<=hist->GetNbinsY(); ibin2++) {
	auto sum=0.0;
	auto c2=hist->GetYaxis()->GetBinCenter(ibin2);
	for(auto & it2 : responseMapPerMergedStrip) {
	  sum+=it2.second->GetBinContent(ibin1, ibin2);
	}
	if(sum==0) continue;
	std::cout << "center=[" << c1 << ", " << c2 << "], total_integral=" << sum << " (expected 1)" << std::endl;
	//	if(!has_StripSections) continue;
	for(auto & it3 : responseMapPerStripSectionStart) {
	  int key1 = std::get<0>(it3.first);
	  int key2 = std::get<1>(it3.first);	  
	  auto it2=responseMapPerMergedStrip.find(MultiKey2(key1, key2));
	  auto frac_per_strip = (it2->second)->GetBinContent(ibin1, ibin2);
	  auto frac_per_section = (it3.second)->GetBinContent(ibin1, ibin2);
	  if(frac_per_strip==0) continue;
	  std::cout << it3.second->GetName()
		    << ": center=[" << c1 << ", " << c2 << "], merged_strip_fraction=" << frac_per_section/frac_per_strip << std::endl;
	}
      }
    }
    std::cout << __FUNCTION__ << ": Initialized " << responseMapPerMergedStrip.size() << " horizontal response 2D histograms (merged strips)" << std::endl;
    std::cout << __FUNCTION__ << ": Initialized " << responseMapPerStripSectionStart.size() << " horizontal response 2D histograms (section start position)." << std::endl;
  }
  ////// DEBUG
}

// re-generate time response histograms with arbitrary granularity
void StripResponseCalculator::initializeTimeResponse(int NbinsZ) {
  for(auto & it : responseMapPerTimecell) {
    if(it.second) delete it.second;
  }
  responseMapPerTimecell.clear();
  for(auto icell=-Ntimecells; icell<=Ntimecells; icell++) {
    auto hist=
      new TH1D(getTimeResponseHistogramName(icell),
	       Form("Vertical response for time cell %s%d;Charge position within reference time cell [mm];Charge fraction [arb.u.]",
		    (icell==0 ? "" : (icell<0 ? "-" : "+")), abs(icell)),
	       NbinsZ, 0., myGeometryPtr->GetTimeBinWidth());
    responseMapPerTimecell[icell]=hist;
  }

  // Each histogram corresponds to time cell with Z-range=[a,b], where a=start of time cell [mm], b=end of time cell [mm].
  // Center of each bin corresponds to the mean value Z=c [mm] of 1D normal distribution with spread sigmaZ.
  // Fill each bin with fraction of the charge in slice [a,b] computed as:
  //   f = Integral[ exp(- 0.5 * (z-c)^2 / sigmaZ^2 ) / sqrt(2*Pi)/sigmaZ, {z, a, b}] =
  //     = 0.5 * ( Erf[(c-a)/sqrt(2)/sigmaZ] - Erf[(c-b)/sqrt(2)/sigmaZ] )
  //
  for(auto & it : responseMapPerTimecell) {
    auto hist=it.second; // histogram to be filled
    auto index=it.first; // relative time cell
    auto a=index*myGeometryPtr->GetTimeBinWidth();
    auto b=a+myGeometryPtr->GetTimeBinWidth();
    for(auto ibin=1; ibin<=hist->GetNbinsX(); ibin++) {
      auto c=hist->GetBinCenter(ibin);      
      hist->Fill(c, 0.5*(TMath::Erf((c-a)/sqrt(2)/sigma_z) - TMath::Erf((c-b)/sqrt(2)/sigma_z)) );
      ////// DEBUG
      //      std::cout << "range[a="<<a<<", b="<<b<<"], center=" << c << ", integral[a,b]=" << 0.5*(TMath::Erf((c-a)/sqrt(2)/sigma_z) - TMath::Erf((c-b)/sqrt(2)/sigma_z)) << std::endl;
      ////// DEBUG
    }
  }
  ////// DEBUG
  if(debug_flag) {
    auto hist=responseMapPerTimecell.begin()->second;
    for(auto ibin=1; ibin<=hist->GetNbinsX(); ibin++) {
      auto sum=0.0;
      auto c=hist->GetBinCenter(ibin);
      for(auto & it : responseMapPerTimecell) {
    	sum+=it.second->GetBinContent(ibin);
      }
      std::cout << "center=" << c << ", total_integral=" << sum << " (expected 1)" << std::endl;
    }    
    std::cout << __FUNCTION__ << ": Initialized " << responseMapPerTimecell.size() << " vertical response 1D histograms." << std::endl;
  }
  ////// DEBUG
}

std::shared_ptr<TH2D> StripResponseCalculator::getStripResponseHistogram(int dir, int delta_strip) {
  const auto it=responseMapPerMergedStrip.find(MultiKey2(dir, delta_strip));
  if(it==responseMapPerMergedStrip.end()) return std::shared_ptr<TH2D>();
  TH2D* hist=(TH2D*)(it->second->Clone());
  hist->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
  std::shared_ptr<TH2D> result(hist);
  ////// DEBUG
  //  if(debug_flag) std::cout << __FUNCTION__ << ": TH2D original=" << it->second << ", clone=" << hist << ", shared_ptr.get=" << result.get() << std::endl;
  ////// DEBUG
  return result;
}

std::shared_ptr<TH2D> StripResponseCalculator::getStripSectionStartResponseHistogram(int dir, int delta_strip, int delta_pad) {
  const auto it=responseMapPerStripSectionStart.find(MultiKey3(dir, delta_strip, delta_pad));
  if(it==responseMapPerStripSectionStart.end()) return std::shared_ptr<TH2D>();
  TH2D* hist=(TH2D*)(it->second->Clone());
  hist->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
  std::shared_ptr<TH2D> result(hist);
  ////// DEBUG
  //  if(debug_flag) std::cout << __FUNCTION__ << ": TH2D original=" << it->second << ", clone=" << hist << ", shared_ptr.get=" << result.get() << std::endl;
  ////// DEBUG
  return result;
}

std::shared_ptr<TH1D> StripResponseCalculator::getTimeResponseHistogram(int delta_timecell) {
  const auto it=responseMapPerTimecell.find(delta_timecell);
  if(it==responseMapPerTimecell.end()) return std::shared_ptr<TH1D>();
  TH1D* hist=(TH1D*)(it->second->Clone());
  hist->SetDirectory(0); // do not associate this clone with any TFile dir to prevent memory leaks
  std::shared_ptr<TH1D> result(hist);
  ////// DEBUG
  //  if(debug_flag) std::cout << __FUNCTION__ << ": TH1D original=" << it->second << ", clone=" << hist << ", shared_ptr.get=" << result.get() << std::endl;
  ////// DEBUG
  return result;
}
 
const char* StripResponseCalculator::getStripResponseHistogramName(int dir, int delta_strip) {
  return Form("h_respXY_%s%s%d", myGeometryPtr->GetDirName(dir),
	      (delta_strip==0 ? "" :
	       (delta_strip<0 ? "minus" : "plus")), abs(delta_strip));
}    

const char* StripResponseCalculator::getStripSectionStartResponseHistogramName(int dir, int delta_strip, int delta_pad) {
  return Form("h_respXY_%s%s%d_P%s%d", myGeometryPtr->GetDirName(dir),
	      (delta_strip==0 ? "" :
	       (delta_strip<0 ? "minus" : "plus")), abs(delta_strip),
	      (delta_pad==0 ? "" :
	       (delta_pad<0 ? "minus" : "plus")), abs(delta_pad));
}

const char* StripResponseCalculator::getTimeResponseHistogramName(int delta_timecell) {
  return Form("h_respZ_%s%d", (delta_timecell==0 ? "" : (delta_timecell<0 ? "minus" : "plus")), abs(delta_timecell));
}
