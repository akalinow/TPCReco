#include "TrackBuilder.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

	nAccumulatorRhoBins = 100;//FIX ME move to configuarable
	nAccumulatorPhiBins = 100;//FIX ME move to configuarable

	myHistoInitialized = false;

	fitter.Config().MinimizerOptions().SetMinimizerType("GSLSimAn");
	fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1E6);
	fitter.Config().MinimizerOptions().SetMaxIterations(1E6);
	fitter.Config().MinimizerOptions().SetTolerance(1E-2);
	fitter.Config().MinimizerOptions().Print(std::cout);

	///An offset used for filling the Hough transformation.
	///to avoid having very small rho parameters, as
	///orignally manty track traverse close to X=0, Time=0
	///point.
	aHoughOffest.SetX(20.0);
	aHoughOffest.SetY(40.0);
	aHoughOffest.SetZ(0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(std::shared_ptr<EventTPC> aEvent) {

	myEvent = aEvent;
	double eventMaxCharge = myEvent->GetMaxCharge();
	double chargeThreshold = 0.15 * eventMaxCharge;
	int delta_timecells = 15;
	int delta_strips = 1;

	myCluster = myEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);

	std::string hName, hTitle;
	if (!myHistoInitialized) {
		for (auto dir : dir_vec_UVW) {
			auto hRawHits = myCluster->GetStripVsTimeInMM(dir);
			double maxX = hRawHits->GetXaxis()->GetXmax();
			double maxY = hRawHits->GetYaxis()->GetXmax();
			double rho = std::hypot(maxX, maxY);
			hName = "hAccumulator_" + std::to_string(int(dir));
			hTitle = "Hough accumulator for direction: " + std::to_string(int(dir)) + ";#theta;#rho";
			myAccumulators[dir] = { hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins, -pi, pi, nAccumulatorRhoBins, 0, rho };
			myRecHits[dir] = hRawHits;
		}
		myHistoInitialized = true;
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct() {

	for (auto&& dir : dir_vec_UVW) {
		makeRecHits(dir);
		fillHoughAccumulator(dir);
		my2DSeeds[dir] = findSegment2DCollection(dir);
	}
	myTrack3DSeed = buildSegment3D();
	myFittedTrack = fitTrack3D(myTrack3DSeed);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(direction dir) {

	auto hRawHits = myCluster->GetStripVsTimeInMM(dir);
	auto hRecHits = myRecHits[dir];
	hRecHits->Reset();
	std::string tmpTitle(hRecHits->GetTitle());
	if (tmpTitle.find("Event") != std::string::npos) {
		tmpTitle.replace(tmpTitle.find("Event"), 20, "Rec hits");
		hRecHits->SetTitle(tmpTitle.c_str());
	}

	TH1D* hProj;
	double hitWirePos = -999.0;
	for (int iBinY = 1; iBinY < hRecHits->GetNbinsY(); ++iBinY) {
		hProj = hRawHits->ProjectionX("hProj", iBinY, iBinY);
		TF1 timeResponseShape = fitTimeWindow(hProj);

		hitWirePos = hRawHits->GetYaxis()->GetBinCenter(iBinY);
		for (int iSet = 0; iSet < timeResponseShape.GetNpar(); iSet += 3) {
			double hitTimePos = timeResponseShape.GetParameter(iSet + 1);
			double hitTimePosError = timeResponseShape.GetParameter(iSet + 2);
			double hitCharge = timeResponseShape.GetParameter(iSet);
			hitCharge *= sqrt(2.0) * pi * hitTimePosError;//the gausian fits are made without the normalisation factor
			if (hitCharge > 50) hRecHits->Fill(hitTimePos, hitWirePos, hitCharge);//FIXME optimize, use dynamic threshold?
		}
		delete hProj;
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TF1 TrackBuilder::fitTimeWindow(TH1D* hProj) {

	TFitResultPtr fitResult;
	TF1 timeResponseShape;
	TF1 bestTimeResponseShape;
	double bestChi2OverNDF = 1E10;

	int maxBin = hProj->GetMaximumBin();
	double maxValue = hProj->GetMaximum();
	double maxPos = hProj->GetBinCenter(maxBin);
	double windowIntegral = hProj->Integral(maxBin - 25, maxBin + 25);
	if (maxValue < 25 || windowIntegral < 50) return bestTimeResponseShape;//FIXME how to choose the thresholds?

	std::stringstream formula;
	for (int iComponent = 0; iComponent < 3; ++iComponent) {
		formula << (iComponent == 0 ? "" : "+") << "gaus(" << std::to_string(3 * iComponent) << ")";
		TF1 timeResponseShape("timeResponseShape", formula.str().c_str());
		timeResponseShape.SetRange(maxPos - 25, maxPos + 25);
		for (int iSet = 0; iSet < timeResponseShape.GetNpar(); iSet += 3) {
			timeResponseShape.SetParameter(iSet, maxValue * 2);
			timeResponseShape.SetParameter(iSet + 1, maxPos);
			timeResponseShape.SetParameter(iSet + 2, 2.0);
			///     
			timeResponseShape.SetParLimits(iSet, 0.1 * maxValue, maxValue * 2);
			timeResponseShape.SetParLimits(iSet + 1, maxPos - 15, maxPos + 15);
			timeResponseShape.SetParLimits(iSet + 2, 0.5, 8);
		}
		fitResult = hProj->Fit(&timeResponseShape, "QRBSW");

		double chi2 = 0.0;
		for (int iBinX = 1; iBinX < hProj->GetNbinsX(); ++iBinX) {
			double x = hProj->GetBinCenter(iBinX);
			chi2 += std::pow(hProj->GetBinContent(iBinX) - timeResponseShape.Eval(x), 2);
		}
		/*
		std::cout<<"nComponents: "<<iComponent+1
			 <<" histogram chi2: "<<chi2
			 <<" ndf: "<<fitResult->Ndf()
			 <<" NFreeParameters: "<<fitResult->NFreeParameters()
			 <<" chi2/ndf: "<<chi2/fitResult->Ndf()
			 <<std::endl;
		*/
		if (fitResult->Ndf() && chi2 / fitResult->Ndf() < bestChi2OverNDF) {
			bestChi2OverNDF = chi2 / fitResult->Ndf();
			timeResponseShape.Copy(bestTimeResponseShape);
		}
	}
	return bestTimeResponseShape;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const std::shared_ptr<TH2D> TrackBuilder::getRecHits2D(direction dir) const {

	return myRecHits.at(dir);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment2D& TrackBuilder::getSegment2D(direction dir, unsigned int iTrack) const {
	return my2DSeeds.at(dir).at(iTrack);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment3D& TrackBuilder::getSegment3DSeed() const {

	return myTrack3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D& TrackBuilder::getTrack3D(unsigned int iSegment) const {

	return myFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillHoughAccumulator(direction dir) {

	myAccumulators[dir].Reset();

	const auto hRecHits = getRecHits2D(dir);

	double theta = 0.0, rho = 0.0;
	double x = 0.0, y = 0.0;
	int charge = 0;
	for (int iBinX = 1; iBinX < hRecHits->GetNbinsX(); ++iBinX) {
		for (int iBinY = 1; iBinY < hRecHits->GetNbinsY(); ++iBinY) {
			x = hRecHits->GetXaxis()->GetBinCenter(iBinX) + aHoughOffest.X();
			y = hRecHits->GetYaxis()->GetBinCenter(iBinY) + aHoughOffest.Y();
			charge = hRecHits->GetBinContent(iBinX, iBinY);
			if (charge < 1) continue;
			for (int iBinTheta = 1; iBinTheta < myAccumulators[dir].GetNbinsX(); ++iBinTheta) {
				theta = myAccumulators[dir].GetXaxis()->GetBinCenter(iBinTheta);
				rho = x * cos(theta) + y * sin(theta);
				charge = 1.0; //FIX me study how to include the charge. 
				myAccumulators[dir].Fill(theta, rho, charge);
			}
		}
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2DCollection TrackBuilder::findSegment2DCollection(direction dir) {

	TrackSegment2DCollection aTrackCollection;
	for (int iPeak = 0; iPeak < 2; ++iPeak) {
		TrackSegment2D aTrackSegment = findSegment2D(dir, iPeak);
		aTrackCollection.push_back(aTrackSegment);
	}
	return aTrackCollection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackBuilder::findSegment2D(direction dir, int iPeak) const {

	int iBinX = 0, iBinY = 0, iBinZ = 0;
	int margin = 5;
	const TH2D& hAccumulator = myAccumulators.at(dir);

	TH2D* hAccumulator_Clone = (TH2D*)hAccumulator.Clone("hAccumulator_clone");
	myAccumulators.at(dir).GetMaximumBin(iBinX, iBinY, iBinZ);
	for (int aPeak = 0; aPeak < iPeak; ++aPeak) {
		for (int iDeltaX = -margin; iDeltaX <= margin; ++iDeltaX) {
			for (int iDeltaY = -margin; iDeltaY <= margin; ++iDeltaY) {
				hAccumulator_Clone->SetBinContent(iBinX + iDeltaX, iBinY + iDeltaY, 0, 0);
			}
		}
		hAccumulator_Clone->GetMaximumBin(iBinX, iBinY, iBinZ);
	}
	delete hAccumulator_Clone;

	TVector3 aTangent, aBias;
	int nHits = hAccumulator.GetBinContent(iBinX, iBinY);
	double theta = hAccumulator.GetXaxis()->GetBinCenter(iBinX);
	double rho = hAccumulator.GetYaxis()->GetBinCenter(iBinY);
	double aX = rho * cos(theta);
	double aY = rho * sin(theta);
	aBias.SetXYZ(aX, aY, 0.0);

	aBias -= aHoughOffest.Dot(aBias.Unit()) * aBias.Unit();

	aX = -rho * sin(theta);
	aY = rho * cos(theta);
	aTangent.SetXYZ(aX, aY, 0.0);

	TrackSegment2D aSegment2D{ dir };
	aSegment2D.setBiasTangent(aBias, aTangent);
	aSegment2D.setNAccumulatorHits(nHits);
	return aSegment2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D() const {

	int iTrack2DSeed = 0;
	auto& segmentU = my2DSeeds.at(direction::U)[iTrack2DSeed];
	auto& segmentV = my2DSeeds.at(direction::V)[iTrack2DSeed];
	auto& segmentW = my2DSeeds.at(direction::W)[iTrack2DSeed];

	int64_t nHits_U = segmentU.getNAccumulatorHits();
	int64_t nHits_V = segmentV.getNAccumulatorHits();
	int64_t nHits_W = segmentW.getNAccumulatorHits();

	double bZ_fromU = segmentU.getBiasAtT0().X();
	double bZ_fromV = segmentV.getBiasAtT0().X();
	double bZ_fromW = segmentW.getBiasAtT0().X();
	double bZ = (bZ_fromU * nHits_U + bZ_fromV * nHits_V + bZ_fromW * nHits_W) / (nHits_U + nHits_V + nHits_W);

	double bX_fromU = (segmentU.getBiasAtT0().Y()) * cos(phiPitchDirection.at(direction::U));
	double bY_fromV = (segmentV.getBiasAtT0().Y() - bX_fromU * cos(phiPitchDirection.at(direction::V))) / sin(phiPitchDirection.at(direction::V));
	double bY_fromW = (segmentW.getBiasAtT0().Y() - bX_fromU * cos(phiPitchDirection.at(direction::W))) / sin(phiPitchDirection.at(direction::W));
	double bY = (bY_fromV * nHits_V + bY_fromW * nHits_W) / (nHits_V + nHits_W);
	double bX = bX_fromU;
	TVector3 aBias(bX, bY, bZ);

	double tZ_fromU = segmentU.getTangentWithT1().X();
	double tZ_fromV = segmentV.getTangentWithT1().X();
	double tZ_fromW = segmentW.getTangentWithT1().X();
	double tZ = (tZ_fromU * nHits_U + tZ_fromV * nHits_V + tZ_fromW * nHits_W) / (nHits_U + nHits_V + nHits_W);

	double tX_fromU = segmentU.getTangentWithT1().Y() * cos(phiPitchDirection.at(direction::U));
	double tY_fromV = (segmentV.getTangentWithT1().Y() - tX_fromU * cos(phiPitchDirection.at(direction::V))) / sin(phiPitchDirection.at(direction::V));
	double tY_fromW = (segmentW.getTangentWithT1().Y() - tX_fromU * cos(phiPitchDirection.at(direction::W))) / sin(phiPitchDirection.at(direction::W));
	double tY = (tY_fromV * nHits_V + tY_fromW * nHits_W) / (nHits_V + nHits_W);
	double tX = tX_fromU;
	TVector3 aTangent(tX, tY, tZ);

	TrackSegment3D a3DSeed;
	a3DSeed.setBiasTangent(aBias, aTangent);
	a3DSeed.setRecHits(myRecHits);

	return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrack3D(const TrackSegment3D& aTrackSegment) const {

	Track3D aTrackCandidate;
	aTrackCandidate.addSegment(aTrackSegment);
	aTrackCandidate = fitTrackNodes(aTrackCandidate);

	TGraph aGraph = aTrackCandidate.getHitDistanceProfile();
	double maxValue = 0.0;
	double bestSplit = 0.5;
	int nDivisions = 200;
	for (int iDivision = 1; iDivision < nDivisions; ++iDivision) {
		double val = aGraph.Eval((double)iDivision / nDivisions * aTrackCandidate.getLength());
		if (val > maxValue) {
			maxValue = val;
			bestSplit = (double)iDivision / nDivisions;
		}
	}
	std::cout << "bestSplit: " << bestSplit << std::endl;
	aTrackCandidate.splitWorseChi2Segment(bestSplit);

	for (int iSplit = 0; iSplit < 0; ++iSplit) { //FIX ME
		unsigned int nSegments = aTrackCandidate.getSegments().size();
		for (unsigned int iSegment = 0; iSegment < nSegments; iSegment += 2) {
			aTrackCandidate.splitSegment(iSegment, 0.5);
			nSegments = aTrackCandidate.getSegments().size();
		}
	}


	aTrackCandidate = fitTrackNodes(aTrackCandidate);
	return aTrackCandidate;//TEST
	/*
	if(aTrackCandidate.getLength()<1.0) return aTrackCandidate;//FIX me move threshold to configuration

	bestSplit = fitTrackSplitPoint(aTrackCandidate);
	aTrackCandidate.splitWorseChi2Segment(bestSplit);
	aTrackCandidate = fitTrackNodes(aTrackCandidate);

	bestSplit = fitTrackSplitPoint(aTrackCandidate);
	aTrackCandidate.splitWorseChi2Segment(bestSplit);
	aTrackCandidate = fitTrackNodes(aTrackCandidate);

	return aTrackCandidate;
	*/
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrackNodes(const Track3D& aTrack) const {

	Track3D aTrackCandidate = aTrack;
	std::vector<double> bestParams = aTrackCandidate.getSegmentsStartEndXYZ();
	std::vector<double> params = aTrackCandidate.getSegmentsStartEndXYZ();
	int nParams = params.size();

	ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::chi2FromNodesList, nParams);
	fitter.SetFCN(fcn, params.data());

	for (int iPar = 0; iPar < nParams; ++iPar) {
		fitter.Config().ParSettings(iPar).SetStepSize(0.1);
		fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
	}

	double minChi2 = 1E10;
	for (unsigned int iStep = 0; iStep < 1; ++iStep) { //FIX ME

		std::cout << __FUNCTION__ << " iStep: " << iStep << std::endl;

		aTrackCandidate.extendToWholeChamber();
		aTrackCandidate.shrinkToHits();

		params = aTrackCandidate.getSegmentsStartEndXYZ();

		////
		std::cout << "Pre-fit: " << std::endl;
		std::cout << aTrackCandidate << std::endl;
		//return aTrackCandidate;
		//continue;
		////

		bool fitStatus = fitter.FitFCN();
		if (!fitStatus) {
			Error(__FUNCTION__, "Track3D Fit failed");
			fitter.Result().Print(std::cout);
			return aTrack;
		}
		const ROOT::Fit::FitResult& result = fitter.Result();
		aTrackCandidate.chi2FromNodesList(result.GetParams());
		aTrackCandidate.removeEmptySegments();
		aTrackCandidate.extendToWholeChamber();
		aTrackCandidate.shrinkToHits();

		std::cout << "Post-fit: " << std::endl;
		aTrackCandidate.removeEmptySegments();
		std::cout << aTrackCandidate << std::endl;

		auto chi2 = aTrackCandidate.getChi2();
		if (chi2 < minChi2) {
			minChi2 = chi2;
			bestParams = aTrackCandidate.getSegmentsStartEndXYZ();
		}
	}
	aTrackCandidate.chi2FromNodesList(bestParams.data());
	return aTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////


/*
  double tangentTheta = aTrack.getTangent().Theta();
  double tangentPhi = aTrack.getTangent().Phi();

  TVector3 perpPlaneBaseUnitA, perpPlaneBaseUnitB;
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, pi/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, pi/2.0, pi/2.0 + tangentPhi);

  double biasA = aTrack.getBias().Dot(perpPlaneBaseUnitA);
  double biasB = aTrack.getBias().Dot(perpPlaneBaseUnitB);

  std::cout<<"tangentTheta: "<<tangentTheta
	   <<" tangentPhi: "<<tangentPhi
	   <<" bias A: "<<biasA
	   <<" bias B: "<<biasB
	   <<std::endl;

  std::vector<double> params = {tangentTheta, tangentPhi, biasA, biasB};
  int nParams = params.size();


  ROOT::Math::Functor fcn(aTrack, nParams);
  fitter.SetFCN(fcn, params.data());
  // set step sizes different than default ones (0.3 times parameter values)
  for (int iPar = 0; iPar < nParams; ++iPar){
	fitter.Config().ParSettings(iPar).SetStepSize(0.01);

  }
  fitter.Config().ParSettings(0).SetLimits(0, pi);
  fitter.Config().ParSettings(1).SetLimits(-pi, pi);
  fitter.Config().ParSettings(2).SetLimits(-100, 100);
  fitter.Config().ParSettings(3).SetLimits(-100, 100);
  */
