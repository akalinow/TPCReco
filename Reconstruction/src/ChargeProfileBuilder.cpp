#include <algorithm>
#include <cmath>

#include <TGraphErrors.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/ChargeProfileBuilder.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH1F ChargeProfileBuilder::makeHisto(double maxLength) const {

  double minX = -0.2*maxLength;
  double maxX = 1.2*maxLength;
  int nBins = std::max(10, int(std::ceil((maxX-minX)/mySettings.binWidth)));
  // not attached to the current directory (e.g. an open output file)
  const bool addDirectoryStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  TH1F histo("hChargeProfile", ";d [mm];charge/mm", nBins, minX, maxX);
  TH1::AddDirectory(addDirectoryStatus);
  return histo;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ChargeProfileBuilder::addSegment(TH1F & histo, const TrackSegment3D & aSegment, double shift) const {

  const double length = aSegment.getLength();
  if(length<1) return;
  const auto & recHits = aSegment.getRecHits();
  const TrackSegment2D::ChargeWeight gainCorrection = aSegment.getGainCorrection(length);
  const double binWidth = histo.GetBinWidth(1);

  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    if(strip_dir>=int(recHits.size())) break;
    TrackSegment2D aProjection = aSegment.get2DProjection(strip_dir, 0, length);
    // a short projection (segment along the strips) gives a noisy, compressed profile
    double projLength = aProjection.getLength();
    if(projLength<mySettings.minProjLength || projLength<mySettings.minProjFraction*length) continue;
    TGraphErrors aGraph = aProjection.getChargeProfile(recHits.at(strip_dir), mySettings.radiusCut, gainCorrection);
    // points: x = fraction of the segment length (0 = vertex), ex = half width of the hit
    double x, y;
    for(int iPoint=0;iPoint<aGraph.GetN();++iPoint){
      aGraph.GetPoint(iPoint, x, y);
      // hits near the segment only: the line continues beyond the vertex (x = 0), where the other
      // segments are (e.g. the carbon behind the vertex for the alpha): no hits behind the vertex,
      // a margin beyond the segment end only (Bragg peak smeared by the diffusion). A margin at the
      // vertex, as a fraction of a long alpha, would take the whole carbon (+30% charge, MC).
      if(x<0 || x>1+mySettings.endMargin) continue;
      double ex = aGraph.GetErrorX(iPoint);
      double low = (x-ex)*length + shift;
      double high = (x+ex)*length + shift;
      double value = y*binWidth/length; // charge per unit fraction -> charge in the hit interval
      int binLow = histo.FindBin(low);
      int binHigh = histo.FindBin(high);
      for(int iBin=binLow;iBin<=binHigh;++iBin){
        // fraction of bin iBin covered by the hit interval
        double coverage = 0.0;
        if(binLow==binHigh) coverage = high-low;
        else if(iBin==binLow) coverage = histo.GetXaxis()->GetBinUpEdge(iBin) - low;
        else if(iBin==binHigh) coverage = high - histo.GetXaxis()->GetBinLowEdge(iBin);
        else coverage = histo.GetBinWidth(iBin);
        coverage /= histo.GetBinWidth(iBin);
        histo.SetBinContent(iBin, histo.GetBinContent(iBin) + value*coverage);
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<TH1F> ChargeProfileBuilder::getSegmentProfiles(const Track3D & aTrack) const {

  std::vector<TH1F> profiles;
  double maxLength = 0.0;
  for(const auto & aSegment: aTrack.getSegments()) maxLength = std::max(maxLength, aSegment.getLength());
  if(maxLength<1) return profiles;

  for(const auto & aSegment: aTrack.getSegments()){
    TH1F histo = makeHisto(maxLength);
    addSegment(histo, aSegment, maxLength - aSegment.getLength()); // segment end at x = maxLength
    histo.Scale(1.0/histo.GetBinWidth(1));                         // charge/mm
    profiles.push_back(histo);
  }
  return profiles;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH1F ChargeProfileBuilder::getTrackProfile(const Track3D & aTrack) const {

  std::vector<TH1F> profiles = getSegmentProfiles(aTrack);
  if(profiles.empty()) return makeHisto(1.0);
  TH1F hStack = profiles.front();
  for(size_t iSegment=1;iSegment<profiles.size();++iSegment) hStack.Add(&profiles.at(iSegment));
  return hStack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ChargeProfileBuilder::fill(Track3D & aTrack) const {

  aTrack.setChargeProfile(getTrackProfile(aTrack));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
