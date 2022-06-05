#include <iostream>

#include "MultiKey.h"
#include "GeometryTPC.h"
#include "StripResponseCalculator.h"
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
                                                 int delta_strips, int delta_timecells, // consider +/- neighbour strips, time cells
                                                 double sigma_xy, double sigma_z, // horizontal and vertical gaussian spread [mm]
                                                 bool debug_flag,
						 const char *fname)
  : myGeometryPtr(aGeometryPtr), Nstrips(abs(delta_strips)), Ntimecells(abs(delta_timecells)),
    sigma_xy(sigma_xy), sigma_z(sigma_z), has_UVWprojectionsRaw(false), has_UVWprojectionsInMM(false), debug_flag(debug_flag) {

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
  if(strlen(fname)==0 || (strlen(fname)>0 && !loadResponseHistograms(fname))) {
    initializeStripResponse();
    initializeTimeResponse();
  }
}

bool StripResponseCalculator::loadResponseHistograms(const char *fname) {						 
  TFile f(fname, "OLD");
  if(!f.IsOpen()) {
    if(debug_flag) std::cout<<__FUNCTION__<<KRED<<": Cannot open TFile: "<<fname<<"!"<<RST<<std::endl;
    return false;
  }

  // initilaize strip domain histograms
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
      //    std::cout << __FUNCTION__ << ": dir=" << strip_dir << ", delta_strip=" << icell << ", hist=" << hist << std::endl;
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
    //    std::cout << __FUNCTION__ << ": delta_timecell=" << icell << ", hist=" << hist << std::endl;
  }

  f.Close();
  return true;
}
bool StripResponseCalculator::saveResponseHistograms(const char *fname) {						 
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
  for(auto & it : responseMapPerTimecell) {
    it.second->SetTitleOffset(1.3, "X");
    it.second->SetTitleOffset(1.5, "Y");
    it.second->SetOption("HISTO");
    it.second->Write();
  }
  f.Close();
  return true;
}

// declare array of UZ/VZ/WZ histograms to be filled (channel vs time cell)
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
  myUVWprojectionsRaw.clear();
  myUVWprojectionsRaw=aUVWprojectionsRaw;
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
// declare array of UZ/VZ/WZ histograms to be filled (in mm)
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
  myUVWprojectionsInMM.clear();
  myUVWprojectionsInMM=aUVWprojectionsInMM;
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

void StripResponseCalculator::fillProjections(TVector3 position3d, double charge) {
  fillProjections(position3d.X(), position3d.Y(), position3d.Z(), charge);
}
void StripResponseCalculator::fillProjections(double x, double y, double z, double charge) {

  if(charge==0.0 || (!has_UVWprojectionsRaw && !has_UVWprojectionsInMM)) return; // nothing to do

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
  //  if(debug_flag) {
  //    std::cout << __FUNCTION__ << ": has_UVWprojectionsRaw=" << has_UVWprojectionsRaw
  //	      << ", has_UVWprojectionsInMM=" << has_UVWprojectionsInMM << std::endl;
  //    std::cout << __FUNCTION__ << ": dx=" << dx << ", dy=" << dy << ", dz=" << dz << std::endl;
  //  }
  ////// DEBUG

	// fill all declared UZ, VZ, WZ projection histograms 
  err=false;
  for(auto & respXY : responseMapPerMergedStrip) {
    ////// DEBUG
    //    std::cout << __FUNCTION__ << ": 1st loop:  dir=" << respXY.first.key1 << ", delta_strip=" << respXY.first.key2 << ", hist=" << respXY.second << std::endl;
    ////// DEBUG
    const auto smeared_dir=respXY.first.key1; // DIR index
    const auto smeared_strip=refStrips[smeared_dir]+respXY.first.key2; // STRIP index
    const auto smeared_fractionXY=respXY.second->GetBinContent(respXY.second->FindBin(dx ,dy));
    if(smeared_fractionXY==0.0) continue;
    for(auto & respZ : responseMapPerTimecell) {
      ////// DEBUG
      //      std::cout << __FUNCTION__ << ": 2nd loop: delta_timecell=" << respZ.first << ", hist=" << respZ.second << std::endl;
      ////// DEBUG
      const auto smeared_fractionZ=respZ.second->GetBinContent(respZ.second->FindBin(dz));
      const auto smeared_charge=charge*smeared_fractionZ*smeared_fractionXY;
      if(smeared_charge==0.0) continue;
      const auto smeared_timecell=refCell+respZ.first;

      if(has_UVWprojectionsRaw) {
	myUVWprojectionsRaw[smeared_dir]->Fill(smeared_timecell*1., smeared_strip*1., smeared_charge);
	////// DEBUG
	//	if(debug_flag) std::cout << __FUNCTION__
	//				 << ": dir=" << smeared_dir
	//				 << ", strip=" << smeared_strip << " (delta=" << respXY.first.key2
	//				 << "), cell=" << smeared_timecell << " (delta=" << respZ.first
	//				 << "), charge=" << smeared_charge << std::endl;
	////// DEBUG
      }
      
      if(has_UVWprojectionsInMM) {
	const auto smeared_pos=myGeometryPtr->Strip2posUVW(smeared_dir, smeared_strip, err);
	const auto smeared_z=myGeometryPtr->Timecell2pos(smeared_timecell, err);
	myUVWprojectionsInMM[smeared_dir]->Fill(smeared_z, smeared_pos, smeared_charge);
	////// DEBUG
	//	if(debug_flag) std::cout << __FUNCTION__
	//				 << ": dir=" << smeared_dir
	//				 << ", strip=" << smeared_strip << " (delta=" << respXY.first.key2
	//				 << ", posUVW=" << myGeometryPtr->Cartesian2posUVW(x, y, smeared_dir, err)
	//				 << "), cell=" << smeared_timecell << " (delta=" << respZ.first
	//				 << ", z=" << smeared_z
	//				 << "), charge=" << smeared_charge << std::endl;
	////// DEBUG
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

  if(debug_flag) std::cout << __FUNCTION__ << ": strip_dir=" << strip_dir << ", strip_num=" << stripMap[strip_dir] << std::endl;

  const auto strip_startPos=strip->Offset()+myGeometryPtr->GetReferencePoint()-0.5*myGeometryPtr->GetStripUnitVector(strip_dir)*myGeometryPtr->GetPadPitch();
  const auto distance_to_start=(TVector2(x,y)-strip_startPos)*myGeometryPtr->GetStripUnitVector(strip_dir);
  auto distance_to_node=fmod(distance_to_start, myGeometryPtr->GetPadPitch()); // [mm]
  if(distance_to_node>0.5*myGeometryPtr->GetPadPitch()) distance_to_node-=myGeometryPtr->GetPadPitch(); // range [-0.5*pad_pitch, 0.5*pad_pitch]
  const auto nodePos=strip_startPos+myGeometryPtr->GetStripUnitVector(strip_dir)*(distance_to_start-distance_to_node);

  // find 2 remaining strip numbers
  for(int check_dir=DIR_U; check_dir<=DIR_W; check_dir++) {
    if(check_dir==strip_dir) continue;

    for(auto isign=-1; isign<=1; isign+=2) { // probe 2 nearest pads for each direction index
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

void StripResponseCalculator::initializeStripResponse() {
  for(auto & it : responseMapPerMergedStrip) {
    if(it.second) delete it.second;
  }
  responseMapPerMergedStrip.clear();
  //
  // Each histogram corresponds to single strip given by {relative strip index wrt reference node, strip direction} pair.
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
		 Nbins, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize(),
		 Nbins, -myGeometryPtr->GetPadSize(), myGeometryPtr->GetPadSize());
      responseMapPerMergedStrip[MultiKey2(strip_dir, istrip)]=hist;
    }
  }

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
  gauss2d.SetNpx(Nbins*10);
  gauss2d.SetNpy(Nbins*10);
  double delta_x, delta_y;
  const auto weight=1./(double)Npoints;
  ////// DEBUG
  //  const auto R2=1.05*pow(myGeometryPtr->GetPadSize(), 2);
  ////// DEBUG
  const auto hist=responseMapPerMergedStrip.begin()->second;
  double c1, c2;
  StripTPC* strip=NULL;
  for(unsigned long ipoint=0L; ipoint<Npoints; ipoint++) {
    gauss2d.GetRandom2(delta_x, delta_y);
    if(debug_flag && ( (ipoint<1000 && ipoint%100==0) ||
		       ipoint%1000==0 )) {
      std::cout << __FUNCTION__ << ": Generating point=" << ipoint << std::endl;
    }
    for(auto ibin1=1; ibin1<=hist->GetNbinsX(); ibin1++) {
      c1=hist->GetXaxis()->GetBinCenter(ibin1);
      for(auto ibin2=1; ibin2<=hist->GetNbinsY(); ibin2++) {
	c2=hist->GetYaxis()->GetBinCenter(ibin2);
	////// DEBUG
	//	if(c1*c1+c2*c2>R2) continue; // stay within radius of PAD SIZE
	////// DEBUG
	strip=myGeometryPtr->GetTH2PolyStrip( myGeometryPtr->GetTH2Poly()->FindBin(refNodePosInMM.X()+c1+delta_x, refNodePosInMM.Y()+c2+delta_y) );
	if(!strip) continue;
	const auto it=responseMapPerMergedStrip.find(MultiKey2(strip->Dir(), strip->Num()-refStrips[strip->Dir()]));
	if(it==responseMapPerMergedStrip.end()) continue;
	it->second->Fill(c1, c2, weight);
      }
    }
  }
    
  if(debug_flag) {
    ////// DEBUG
    auto hist=responseMapPerMergedStrip.begin()->second;
    for(auto ibin1=1; ibin1<=hist->GetNbinsX(); ibin1++) {
      auto c1=hist->GetXaxis()->GetBinCenter(ibin1);
      for(auto ibin2=1; ibin2<=hist->GetNbinsY(); ibin2++) {
	auto sum=0.0;
	auto c2=hist->GetYaxis()->GetBinCenter(ibin2);
	for(auto & it : responseMapPerMergedStrip) {
	  sum+=it.second->GetBinContent(ibin1, ibin2);
	}
	std::cout << "center=[" << c1 << ", " << c2 << "], total_integral=" << sum << " (expected 1)" << std::endl;
      }
    }
    ////// DEBUG
    std::cout << __FUNCTION__ << ": Initialized " << responseMapPerMergedStrip.size() << " horizontal response 2D histograms." << std::endl;
  }
}
void StripResponseCalculator::initializeTimeResponse() {
  for(auto & it : responseMapPerTimecell) {
    if(it.second) delete it.second;
  }
  responseMapPerTimecell.clear();
  for(auto icell=-Ntimecells; icell<=Ntimecells; icell++) {
    auto hist=
      new TH1D(getTimeResponseHistogramName(icell),
	       Form("Vertical response for time cell %s%d;Charge position within reference time cell [mm];Charge fraction [arb.u.]",
		    (icell==0 ? "" : (icell<0 ? "-" : "+")), abs(icell)),
	       Nbins, 0., myGeometryPtr->GetTimeBinWidth());
    responseMapPerTimecell[icell]=hist;
  }
  //
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
  if(debug_flag) {
    ////// DEBUG
    auto hist=responseMapPerTimecell.begin()->second;
    for(auto ibin=1; ibin<=hist->GetNbinsX(); ibin++) {
      auto sum=0.0;
      auto c=hist->GetBinCenter(ibin);
      for(auto & it : responseMapPerTimecell) {
    	sum+=it.second->GetBinContent(ibin);
      }
      std::cout << "center=" << c << ", total_integral=" << sum << " (expected 1)" << std::endl;
    }    
    ////// DEBUG
    std::cout << __FUNCTION__ << ": Initialized " << responseMapPerTimecell.size() << " vertical response 1D histograms." << std::endl;
  }
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
const char* StripResponseCalculator::getTimeResponseHistogramName(int delta_timecell) {
  return Form("h_respZ_%s%d", (delta_timecell==0 ? "" : (delta_timecell<0 ? "minus" : "plus")), abs(delta_timecell));
}
