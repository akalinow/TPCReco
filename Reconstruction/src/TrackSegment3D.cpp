#include "TrackSegment3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBiasTangent(const TVector3& aBias, const TVector3& aTangent) {

	myBias = aBias;
	myTangent = aTangent.Unit();

	double lambda = 30;//FIXME what value shouldbe here?
	myStart = myBias;
	myEnd = myStart + lambda * myTangent;

	initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const TVector3& aStart, const TVector3& aEnd) {

	myStart = aStart;
	myEnd = aEnd;

	myTangent = (myEnd - myStart).Unit();
	myBias = myStart;

	initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const double* par) {

	TVector3 start(par[0], par[1], par[2]);
	TVector3 end(par[3], par[4], par[5]);
	setStartEnd(start, end);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setRecHits(const std::map<direction, std::shared_ptr<TH2D>>& aRecHits) {

	std::for_each(/*std::execution::par,*/ dirs.begin(), dirs.end(), [&](auto strip_dir) { //C++17
		auto hRecHits = aRecHits.at(strip_dir);
		for (int iBinX = 1; iBinX < hRecHits->GetNbinsX(); ++iBinX) {
			for (int iBinY = 1; iBinY < hRecHits->GetNbinsY(); ++iBinY) {
				auto charge = hRecHits->GetBinContent(iBinX, iBinY);
				auto x = hRecHits->GetXaxis()->GetBinCenter(iBinX);
				auto y = hRecHits->GetYaxis()->GetBinCenter(iBinY);
				if (charge > 0.0) myRecHits.at(strip_dir).push_back({ x, y, charge });
			}
		}
	});

	calculateRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::initialize() {

	double lambda = -myBias.X() / myTangent.X();
	myBiasAtX0 = myBias + lambda * myTangent;

	lambda = -myBias.Y() / myTangent.Y();
	myBiasAtY0 = myBias + lambda * myTangent;

	lambda = -myBias.Z() / myTangent.Z();
	myBiasAtZ0 = myBias + lambda * myTangent;

	myLenght = (myEnd - myStart).Mag();

	calculateRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> TrackSegment3D::getStartEndXYZ() const {

	std::vector<double> coordinates(6);
	double* data = coordinates.data();

	getStart().GetXYZ(data);
	getEnd().GetXYZ(data + 3);

	return coordinates;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment3D::getPointOn2DProjection(double lambda, direction strip_dir) const {

	TVector3 stripPitchDirection(cos(phiPitchDirection.at(strip_dir)),
		sin(phiPitchDirection.at(strip_dir)), 0);

	const TVector3& start = getStart();
	const TVector3& tangent = getTangent();
	TVector3 aPointOnLine = start + lambda * tangent;
	double directionTime = aPointOnLine.Z();
	double directionWire = aPointOnLine * stripPitchDirection;

	return { directionTime, directionWire, 0.0 };
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackSegment3D::get2DProjection(direction strip_dir, double lambdaStart, double lambdaEnd) const {

	TVector3 start = getPointOn2DProjection(lambdaStart, strip_dir);
	TVector3 end = getPointOn2DProjection(lambdaEnd, strip_dir);

	TrackSegment2D a2DProjection(strip_dir);
	a2DProjection.setStartEnd(start, end);

	return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedHitDistance(double lambdaCut) const {

#ifndef _cpp17_
	return std::inner_product(dirs.begin(), dirs.end(), myRecHits.begin(), 0.0, std::plus<>(), [&](direction strip_dir, auto& aRecHits)->double {
		auto&& aTrack2DProjection = this->get2DProjection(strip_dir, 0, lambdaCut);
		return aTrack2DProjection.getIntegratedHitDistance(lambdaCut, aRecHits.second);
	});
#else
	return std::transform_reduce(std::execution::par_unseq, dirs.begin(), dirs.end(), myRecHits.begin(), 0.0, std::plus<>(), [&](direction strip_dir, auto& aRecHits)->auto {
		TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambdaCut);
		return aTrack2DProjection.getIntegratedHitDistance(lambdaCut, aRecHits.second);
	});

#endif // !_cpp17_
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedCharge(double lambdaCut) const {

#ifndef _cpp17_
	return std::inner_product(dirs.begin(), dirs.end(), myRecHits.begin(), 0.0, std::plus<>(), [&](direction strip_dir, auto& aRecHits)->auto {
		auto&& aTrack2DProjection = this->get2DProjection(strip_dir, 0, lambdaCut);
		return aTrack2DProjection.getIntegratedCharge(lambdaCut, aRecHits.second);
	});
#else
	return std::transform_reduce(std::execution::par_unseq, dirs.begin(), dirs.end(), myRecHits.begin(), 0.0, std::plus<>(), [&](direction strip_dir, auto& aRecHits)->auto {
		TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambdaCut);
		return aTrack2DProjection.getIntegratedCharge(lambdaCut, aRecHits.second);
	});

#endif // !_cpp17_
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2() const {

	return std::accumulate(myProjectionsChi2.begin(), myProjectionsChi2.end(), 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::calculateRecHitChi2() {

	for (auto strip_dir : dirs) {
		auto&& aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
		myProjectionsChi2[(int)strip_dir] = aTrack2DProjection.getRecHitChi2(myRecHits[strip_dir]);
	};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double* par) {

	TVector3 start(par[0], par[1], par[2]);
	TVector3 end(par[3], par[4], par[5]);
	setStartEnd(start, end);
	return getRecHitChi2();
	///////////////////////////

	/*
	double tangentTheta = par[0];
	double tangentPhi = par[1];

	double biasComponentA = par[2];
	double biasComponentB = par[3];

	TVector3 perpPlaneBaseUnitA, perpPlaneBaseUnitB;

	perpPlaneBaseUnitA.SetMagThetaPhi(1.0, pi/2.0 + tangentTheta, tangentPhi);
	perpPlaneBaseUnitB.SetMagThetaPhi(1.0, pi/2.0, pi/2.0 + tangentPhi);

	TVector3 aBias = perpPlaneBaseUnitA*biasComponentA + perpPlaneBaseUnitB*biasComponentB;
	TVector3 aTangent;
	aTangent.SetMagThetaPhi(1.0, tangentTheta, tangentPhi);
	setBiasTangent(aBias, aTangent);

	return getRecHitChi2();
	*/
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream& operator << (std::ostream& out, const TrackSegment3D& aSegment) {

	const TVector3& start = aSegment.getStart();
	const TVector3& end = aSegment.getEnd();

	out << "(" << start.X() << ", " << start.Y() << ", " << start.Z() << ")"
		<< " -> "
		<< "(" << end.X() << ", " << end.Y() << ", " << end.Z() << ") "
		<< "[" << aSegment.getRecHitChi2() << "]";

	return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
