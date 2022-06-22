#include "TH2D.h"
#include "TF1.h"

#include "GeometryTPC.h"
#include "RecHitBuilder.h"

#include "colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecHitBuilder::RecHitBuilder(){

  noiseShape = TF1("noiseShape","pol0");
  signalShape = TF1("signalShape","gaus");
  emptyShape = TF1("emptyShape","0");

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecHitBuilder::~RecHitBuilder(){}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecHitBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){

  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeRecHits(const TH2D & hProjection){

  hRecHits = hProjection;
  hRecHits.Reset();
  hRecHits.SetTitle(adaptHistoTitle(hProjection.GetTitle()).c_str());

  if(!myGeometryPtr){
    std::cerr<<__FUNCTION__<<KRED<<" NULL myGeometryPtr"<<RST<<std::endl; 
    return hRecHits;
  }

  TH2D hCleanClusters = makeCleanCluster(hProjection);
  makeTimeProjectionRecHits(hCleanClusters);
  double recHitsSum = hRecHits.Integral();
  double clusterSum = hProjection.Integral();
  double ratio = recHitsSum/clusterSum;
  if(ratio<0.2) makeStripProjectionRecHits(hProjection);
  cleanRecHits();
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeTimeProjectionRecHits(const TH2D & hProjection){

  TH1D *h1DProj;
  double hitStripPos = -999.0;
  double hitTimePos = -999.0;
  double hitTimePosError = -999.0;
  double hitCharge = -999.0;
  double initialSigma = 2*myGeometryPtr->GetTimeBinWidth();//1* for 12.5MHz, 2* for 25.0 MHz
  for(int iBinY=1;iBinY<=hProjection.GetNbinsY();++iBinY){
    h1DProj = hProjection.ProjectionX("h1DProjX",iBinY, iBinY);
    const TF1 &fittedShape = fit1DProjection(h1DProj, initialSigma);
    if(fittedShape.GetNpar()<3){
      delete h1DProj;
      continue;      
    }
    hitCharge = fittedShape.GetParameter(0);
    hitTimePos = fittedShape.GetParameter(1);
    hitTimePosError = fittedShape.GetParameter(2);
    hitStripPos = hProjection.GetYaxis()->GetBinCenter(iBinY);    
    hitCharge *= sqrt(2.0)*M_PI*hitTimePosError;
    hRecHits.Fill(hitTimePos, hitStripPos, hitCharge);    
    delete h1DProj;
  }
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeStripProjectionRecHits(const TH2D & hProjection){

  TH1D *h1DProj;
  double hitStripPos = -999.0;
  double hitTimePos = -999.0;
  double hitStripPosError = -999.0;
  double hitCharge = -999.0;
  double initialSigma = myGeometryPtr->GetStripPitch();
  for(int iBinX=1;iBinX<=hProjection.GetNbinsX();++iBinX){
    h1DProj = hProjection.ProjectionY("h1DProjY",iBinX, iBinX);
    const TF1 &fittedShape = fit1DProjection(h1DProj, initialSigma);
    if(fittedShape.GetNpar()<3) continue;
    hitCharge = fittedShape.GetParameter(0);
    hitStripPos = fittedShape.GetParameter(1);
    hitStripPosError = fittedShape.GetParameter(2);
    hitTimePos = hProjection.GetXaxis()->GetBinCenter(iBinX);    
    hitCharge *= sqrt(2.0)*M_PI*hitStripPosError;
    hRecHits.Fill(hitTimePos, hitStripPos, hitCharge);      
    delete h1DProj;
  }
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fit1DProjection(TH1D* hProj, double initialSigma){

  int maxValueBin = hProj->GetMaximumBin();
  double maxValue = hProj->GetBinContent(maxValueBin);
  
  int lowBin = FindFirstBinAbove(hProj, maxValue/4.0, 1, maxValueBin-projection1DHalfSize);
  int highBin = FindLastBinAbove(hProj, maxValue/4.0, 1, maxValueBin+projection1DHalfSize);
  if(lowBin<0) lowBin = maxValueBin-projection1DHalfSize;
  if(highBin<0) highBin = maxValueBin+projection1DHalfSize;
  int delta = std::max(std::abs(lowBin-maxValueBin),
		       std::abs(highBin-maxValueBin));
  
  lowBin = maxValueBin-delta;
  highBin= maxValueBin+delta;
  
  if(lowBin<0) lowBin = 1;
  if(highBin>hProj->GetNbinsX()) highBin = hProj->GetNbinsX();
  
  double minX = hProj->GetBinCenter(lowBin);
  double maxX = hProj->GetBinCenter(highBin);
  double windowIntegral = hProj->Integral(lowBin, highBin);

  hProj->GetXaxis()->SetRange(lowBin, highBin);
  hProj->SetMaximum(1.1*maxValue);

  if(maxValue<maxValueThr || windowIntegral<windowIntegralThr) return emptyShape;
  
  const TF1 & noiseFit = fitNoise(hProj, minX, maxX);
  const TF1 & singleHitFit = fitSingleHit(hProj, minX, maxX, maxValue, initialSigma);

  double noiseMSE = getMSE(*hProj, noiseFit);
  double singleHitMSE = getMSE(*hProj, singleHitFit);
 
  if(singleHitMSE/noiseMSE<0.9) return singleHitFit;
  return noiseFit;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fitNoise(TH1D* hProj, double minX, double maxX){

  noiseShape.SetRange(minX, maxX);
  TFitResultPtr noiseFitResult = hProj->Fit(&noiseShape, "QRBSWN");
  return noiseShape;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fitSingleHit(TH1D* hProj,
					double minX, double maxX,
					double initialMax,
					double initialSigma){
 
  double meanX = (minX+maxX)/2.0;
  double minMeanX = meanX - (maxX - minX)*0.5*0.8;
  double maxMeanX = meanX + (maxX - minX)*0.5*0.8;
  signalShape.SetRange(minX, maxX);
  
  signalShape.SetParameter(0, initialMax);
  signalShape.SetParameter(1, meanX);
  signalShape.SetParameter(2, initialSigma);

  signalShape.SetParLimits(0, 0.5*initialMax, initialMax*1.5);
  signalShape.SetParLimits(1, minMeanX, maxMeanX);   
  signalShape.SetParLimits(2, initialSigma, 2.0*initialSigma);
  
  TFitResultPtr fitResult = hProj->Fit(&signalShape, "QBRSWN");
  return signalShape;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecHitBuilder::cleanRecHits(){

  double maxCharge = hRecHits.GetMaximum();
  double threshold = 0.1*maxCharge+1E-3;//FIX ME optimize threshold
  int nEntries = 0;
  for(int iBin=0;iBin<hRecHits.GetNcells();++iBin){
    if(hRecHits.GetBinContent(iBin)<threshold){
      hRecHits.SetBinContent(iBin, 0.0);
    }
    else{
      ++nEntries;
    }
  }
  hRecHits.SetEntries(nEntries);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double RecHitBuilder::getMSE(const TH1D &hProj, const TF1 & aFunc) const{

  double mse = 0.0;
  double value = 0.0;
  double x = 0.0;
  int nBins = hProj.GetXaxis()->GetLast() - hProj.GetXaxis()->GetFirst() + 1;
  for(int iBinX=hProj.GetXaxis()->GetFirst();
      iBinX<=hProj.GetXaxis()->GetLast();++iBinX){
    x = hProj.GetBinCenter(iBinX);
    value = hProj.GetBinContent(iBinX);
    mse += (value>0)*std::pow(value - aFunc.Eval(x), 2);
  }
  
  return mse/nBins; 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<int> RecHitBuilder::fillClusterAndGetBonduary(const std::vector<int> & neighboursBinsIndices,
						     const TH2D & aHisto,
						     TH2D & aCluster){

  std::vector<int> bounduaryBins;
  double binValue = 0.0;
  
  for(auto iGlobalBin: neighboursBinsIndices){
    binValue = aHisto.GetBinContent(iGlobalBin);
    if(aCluster.GetBinContent(iGlobalBin)<emptyBinThreshold && binValue>emptyBinThreshold){
      aCluster.SetBinContent(iGlobalBin, binValue);
      bounduaryBins.push_back(iGlobalBin);
    }
  }
  return bounduaryBins;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<int> RecHitBuilder::getKernelIndices(int iBin, const TH2D & aHisto){

  std::vector<int> kernelIndices;
  int windowSizeX = 3;
  int windowSizeY = 3;

  int iGlobalBin;
  int iBinX, iBinY, iBinZ;
  aHisto.GetBinXYZ(iBin, iBinX, iBinY, iBinZ);
  
  for(int iStepX=-windowSizeX/2;iStepX<=windowSizeX/2;++iStepX){
    for(int iStepY=-windowSizeY/2;iStepY<=windowSizeY/2;++iStepY){
      iGlobalBin = aHisto.GetBin(iBinX+iStepX,
				 iBinY+iStepY,
				 iBinZ);
      kernelIndices.push_back(iGlobalBin);
    }
  }
  return kernelIndices;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double RecHitBuilder::getKernelSum(const std::vector<int> & kernelBins, const TH2D & aHisto){

  double sum = 0.0;
  for(auto iBin :kernelBins){
    sum+=aHisto.GetBinContent(iBin);
  }
  return sum;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D RecHitBuilder::makeCleanCluster(const TH2D & aHisto){

  TH2D aClusterHisto(aHisto);
  aClusterHisto.Reset();

  std::vector<int> tmpVec;
  std::vector<int> bounduaryBins;
  std::vector<int> newBounduaryBins;
  std::vector<int> kernelBins;
  double kernelSum = 0.0;
  int maxValueBin = aHisto.GetMaximumBin();
  bounduaryBins.push_back(maxValueBin);

  while(bounduaryBins.size()){
    for(auto iGlobalBin: bounduaryBins){
      kernelBins = getKernelIndices(iGlobalBin, aHisto);
      kernelSum = getKernelSum(kernelBins, aHisto);
      if(kernelSum<kernelSumThreshold) continue;
      tmpVec = fillClusterAndGetBonduary(kernelBins, aHisto, aClusterHisto);
      newBounduaryBins.insert(newBounduaryBins.end(),tmpVec.begin(),tmpVec.end());	    
    }
    bounduaryBins = newBounduaryBins;
    newBounduaryBins.clear();
  }
 
  return aClusterHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string RecHitBuilder::adaptHistoTitle(const std::string title) const{
  std::string adaptedTitle = title;
  if(adaptedTitle.find("from")!=std::string::npos){
    std::string eventNumber = title.substr(0,title.find(":"));
    adaptedTitle.replace(0, title.find("from"),"");
    adaptedTitle = eventNumber+": Reco hits "+adaptedTitle;
  }
  return adaptedTitle;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Int_t RecHitBuilder::FindFirstBinAbove(TH1* histo, Double_t threshold,
				       Int_t axis, Int_t firstBin, Int_t lastBin) const{

  if(!histo) return -1;

  if (firstBin < 1) {
    firstBin = 1;
  }
  
  if (axis == 1) {
    if (lastBin < 0 || lastBin > histo->GetNbinsX()) {
      lastBin = histo->GetNbinsX();
    }
    for (Int_t binx = firstBin; binx <= lastBin; binx++) {
      if (histo->GetBinContent(binx) > threshold) return binx;
    }
  }
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Int_t RecHitBuilder::FindLastBinAbove(TH1* histo, Double_t threshold,
				      Int_t axis, Int_t firstBin, Int_t lastBin) const{

  if(!histo) return -1;

  if (firstBin < 1) {
    firstBin = 1;
  }

  if (lastBin < 0 || lastBin > histo->GetNbinsX()) {
    lastBin = histo->GetNbinsX();
  }
  for (Int_t binx = lastBin; binx >= firstBin; binx--) {
    if (histo->GetBinContent(binx) > threshold) return binx;
  } 
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// get ALL three projections on: XY, XZ, YZ planes
/*
std::vector<TH2D*> RecHitBuilder::Get2D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) { 

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
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// get 3D histogram of clustered hits
TH3D *RecHitBuilder::Get3D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) {

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
    
      //const std::map<MultiKey2, std::vector<int> > & hitListByTimeDir = cluster.GetHitListByTimeDir();
      
      //for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
      
      //if((hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_U))==hitListByTimeDir.end()) ||
      //(hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_V))==hitListByTimeDir.end()) ||
      //(hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_W))==hitListByTimeDir.end())) continue;
      
      //std::vector<int> hits[3] = {
      //hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_U))->second,
      //hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_V))->second,
      //hitListByTimeDir.find(MultiKey2(icell, projection_type::DIR_W))->second};
    
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
*/
