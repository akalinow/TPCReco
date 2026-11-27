#include <cstdlib>
#include <iostream>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TString.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include <boost/program_options.hpp>

#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/dEdxFitter.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/ConfigManager.h"
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/RunIdParser.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/MakeUniqueName.h"
#include "TPCReco/colorText.h"
#include "TPCReco/HistoManager.h"

#include "TPCReco/EventTPC.h"
/////////////////////////////////////
/////////////////////////////////////

int tpcReco(boost::property_tree::ptree & aConfig);

/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
 
  int nEntriesProcessed = tpcReco(myConfig);

  aStopwatch.Stop();
  std::cout<<KBLU<<"Real time:       "<<RST<<aStopwatch.RealTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"CPU time:        "<<RST<<aStopwatch.CpuTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"Processing rate: "<<RST<<nEntriesProcessed/aStopwatch.RealTime()<< " ev/s"<<std::endl;

  return 0;
}
/////////////////////////////
////////////////////////////
// Define some simple structures
typedef struct {Float_t eventId, frameId,
    eventType,
    length,
    horizontalLostLength, verticalLostLength,
    alphaEnergy, carbonEnergy,
    alphaRange, carbonRange,
    cosPhiSegments,
    charge, cosTheta, phi, chi2,
    cosThetaDET, phiDET,
    xVtx, yVtx, zVtx,
    xAlphaEnd, yAlphaEnd, zAlphaEnd,
    xCarbonEnd, yCarbonEnd, zCarbonEnd,
    total_mom_x,  total_mom_y,  total_mom_z,
    lineFitLoss, dEdxFitLoss, dEdxFitSigma,
    maxProjWidth,
    chargeEnergy, chargeRatio;
    } TrackData;
/////////////////////////
int tpcReco(boost::property_tree::ptree & aConfig) {
		  
  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(aConfig);
  myEventSource->getEventFilter().setConditions(aConfig); // initialize RAW event pre-filtering

  std::string dataFileName = aConfig.get("input.dataFile","");
  std::string rootFileName = InputFileHelper::makeOutputFileName(dataFileName,"TrackTree");
  TFile outputROOTFile(rootFileName.c_str(),"RECREATE");
  TTree *tree = new TTree("trackTree", "Track tree");
  TrackData track_data;
  std::string leafNames = "";
  leafNames += "eventId:frameId:eventType:";
  leafNames += "length:";
  leafNames += "horizontalLostLength:verticalLostLength:";
  leafNames += "alphaEnergy:carbonEnergy:alphaRange:carbonRange:";
  leafNames += "cosPhiSegments:";
  leafNames += "charge:cosTheta:phi:chi2:";
  leafNames += "cosThetaDET:phiDET:";
  leafNames += "xVtx:yVtx:zVtx:";
  leafNames += "xAlphaEnd:yAlphaEnd:zAlphaEnd:";
  leafNames += "xCarbonEnd:yCarbonEnd:zCarbonEnd:";
  leafNames += "total_mom_x:total_mom_y:total_mom_z:";
  leafNames += "lineFitLoss:dEdxFitLoss:dEdxFitSigma:";
  leafNames += "maxProjWidth:";
  leafNames += "chargeEnergy:chargeRatio";
  tree->Branch("track",&track_data,leafNames.c_str());

  // optional tree with the full reconstruction (Track3D) for the analysis programs that read
  // "TPCRecoData" (recoEventsAnalysis, rawTrackDiffusionAnalysis, ...): same file, one entry per stored track
  TTree *recoTree = nullptr;
  Track3D *recoTrackPtr = new Track3D();
  eventraw::EventInfo *recoInfoPtr = new eventraw::EventInfo();
  if(aConfig.get<bool>("output.recoTree", false)){
    recoTree = new TTree("TPCRecoData", "Reconstructed events");
    recoTree->Branch("RecoEvent", &recoTrackPtr);
    recoTree->Branch("EventInfo", &recoInfoPtr);
  }

  std::string geometryFileName = aConfig.get("input.geometryFile","");
  double pressure = aConfig.get<double>("conditions.pressure"); 
  double temperature = aConfig.get<double>("conditions.temperature");
  double samplingRate = aConfig.get<double>("conditions.samplingRate");
    
  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,pressure,temperature);
  ////////////////////////////////////////////
  //
  // extra initialization for fit DEBUG plots
  const auto develMode = aConfig.get<bool>("display.develMode");
  HistoManager myHistoManager;
  const std::string rootFileNameCanvas = "tpcReco_debug_histos.root";
  TFile *outputCanvasROOTFile = nullptr;
  TCanvas *outputCanvas = nullptr;
  if(develMode) {
    outputCanvasROOTFile=new TFile(rootFileNameCanvas.c_str(), "RECREATE");
    if(!outputCanvasROOTFile) {
      std::cout<<KRED<<"Cannot create output ROOT file with debug plots!"<<RST<<std::endl;
      return -1;
    }
    outputCanvasROOTFile->cd();
    const int nx=4, ny=3;
    assert((nx*ny)>=3+1+3+3); // 3 for UVW rawdata, , 1 for dE/dx fit, 3 for UVW RecHits, 3 for UVW Hough accumulators
    outputCanvas=new TCanvas("c_result", "c_result", 1.1*400*nx, 400*ny);
    if(!outputCanvas) {
      std::cout<<KRED<<"Cannot create TCanvas with debug plots!"<<RST<<std::endl;
      return -1;
    }
    outputCanvas->Divide(nx,ny);
    myHistoManager.clearCanvas(outputCanvas, false);
    myHistoManager.setGeometry(myEventSource->getGeometry());
    myHistoManager.setTkBuilder(myEventSource->getTrackBuilder()); // the reconstruction of the event source
    myHistoManager.setConfig(aConfig); // this will pass configuration to current event pointer
    myHistoManager.toggleAutozoom(); // start auto zoom feature
  }
  //
  ////////////////////////////////////////////
  std::shared_ptr<eventraw::EventInfo> myEventInfo = std::make_shared<eventraw::EventInfo>();

  // loop over ALL events
  Long64_t nEntries = myEventSource->numberOfEvents();
  std::cout<<KBLU<<"File with "<<RST<<nEntries<<" frames loaded."<<std::endl;
  Long64_t maxNevents = aConfig.get<int>("input.readNEvents");
  Long64_t previousEventIdx=-1;
  Long64_t counter=0;
  bool readNext=false;

  //Event loop
  do {
    if(maxNevents>=0 && maxNevents==counter) break; // event limit
    // initialize hit filters
    if(previousEventIdx==-1 || develMode) { // initialize only once per session in non-debug mode and every time in debug mode
      myEventSource->getCurrentEvent()->setHitFilterConfig(filter_type::threshold, aConfig);
      myEventSource->getCurrentEvent()->setHitFilterConfig(filter_type::fraction,  aConfig);
    }
    // load very first event
    if(previousEventIdx==-1) {
      myEventSource->loadFileEntry(0);
      if(myEventSource->getEventFilter().isEnabled() &&
	 !myEventSource->getEventFilter().pass(*myEventSource)) {
	readNext = true;
      }
    }
    int iEntry = myEventSource->currentEntryNumber();
    previousEventIdx = myEventSource->currentEventId();
    // load next good event
    if(readNext){
      myEventSource->getNextEventLoop();
      if(previousEventIdx==myEventSource->currentEventId()) break; // no more good events
    }
    readNext = true;
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }

    // find tracks
    *myEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();


    ////////////////////////////////////////////
    //
    // make fit DEBUG plots
    if(develMode) {
      myHistoManager.setEvent(myEventSource->getCurrentEvent());
      gStyle->SetOptStat(0);

      // plot clustered data and dE/dx fit
      myHistoManager.drawDevelHistos(outputCanvas); // pads 1-4

      // plot RecHits per UVW direction
      int ipad=5;
      for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
	TVirtualPad *aPad=outputCanvas->cd(ipad+iDir); // pads 5-7
	if(aPad) {
	  aPad->SetFrameFillColor(kAzure-6);
	  myHistoManager.getRecHitStripVsTime(iDir)->DrawCopy("colz");
	}
      }

      // plot Hough accumulators per UVW direction
      ipad=9;
      for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
	TVirtualPad *aPad=outputCanvas->cd(ipad+iDir); // pads 9-11
	if(aPad) {
	  aPad->SetFrameFillColor(kAzure-6);
	  myHistoManager.getHoughAccumulator(iDir,0).DrawCopy("colz");
	}
      }

      outputCanvas->SetName(Form("c_run%ld_evt%ld", (unsigned long)myEventInfo->GetRunId(), (unsigned long)myEventInfo->GetEventId()));
      outputCanvas->SetTitle(outputCanvas->GetName());
      const std::vector<std::pair<double, double> > marginsLeftRight{ {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15}, {0.15, 0.15},
								      {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15}, {0.5, 0.5},
								      {0.1, 0.15}, {0.1, 0.15}, {0.1, 0.15} };
      ipad=0;
      for(auto &it : marginsLeftRight) {
	ipad++;
	TVirtualPad *aPad=outputCanvas->cd(ipad);
	if(aPad) {
	  aPad->SetLeftMargin(it.first);
	  aPad->SetRightMargin(it.second);
	  aPad->Modified();
	  aPad->Update();
	}
      }
      outputCanvas->Modified();
      outputCanvas->Update();
      outputCanvasROOTFile->cd();
      outputCanvas->Write();
    }
    //
    ////////////////////////////////////////////
    const Track3D  & aTrack3D = myEventSource->getRecoEvent();
    if(aTrack3D.getSegments().empty()) continue; // no reconstruction
    // one segment (alpha only, track without event hypothesis, dot) or
    // two segments (alpha + carbon): with one segment there is no carbon
    const bool hasCarbon = aTrack3D.getSegments().size()>1;
    const TrackSegment3D & firstSegment = aTrack3D.getSegments().front();
    
    double length = aTrack3D.getLength();
    double charge = aTrack3D.getIntegratedCharge(length);
    double chi2 = aTrack3D.getLoss();
    double hypothesisLoss = aTrack3D.getHypothesisFitLoss();
    const TVector3 & vertex = firstSegment.getStart();
    const TVector3 & alphaEnd = firstSegment.getEnd();
    const TVector3 & carbonEnd = hasCarbon ? aTrack3D.getSegments().back().getEnd() : vertex;

    double cosPhiSegments = hasCarbon ? (alphaEnd-vertex).Unit().Dot((carbonEnd-vertex).Unit()) : 0.0;
    
    const TVector3 & tangent = aTrack3D.getSegments().front().getTangent();
    double phi = atan2(-tangent.Z(), tangent.Y());
    double cosTheta = -tangent.X();

    TVector3 horizontal(0,-1,0);
    double horizontalTrackLostPart = 73.4/std::abs(horizontal.Dot(tangent));

    TVector3 vertical(0,0,-1);
    double verticalTrackLostPart = 6.0/std::abs(vertical.Dot(tangent));

    // sum of the segment PIDs: ALPHA+CARBON_12 = 3, ALPHA+CARBON_14 = 4, ALPHA = 1,
    // UNKNOWN = 0 (track without event hypothesis fit), DOT
    int eventType = firstSegment.getPID() + (hasCarbon ? aTrack3D.getSegments().back().getPID() : 0);
    double alphaRange = firstSegment.getPID()==pid_type::ALPHA ? firstSegment.getLength() : 0.0;
    // recoil: CARBON_12 (O16 -> C12 + alpha) or CARBON_14 (O18 -> C14 + alpha)
    const pid_type recoilPID = hasCarbon ? aTrack3D.getSegments().back().getPID() : pid_type::UNKNOWN;
    const bool isCarbon = recoilPID==pid_type::CARBON_12 || recoilPID==pid_type::CARBON_14;
    double carbonRange = isCarbon ? aTrack3D.getSegments().back().getLength(): 0.0;
    double alphaEnergy = alphaRange>0 ? myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA,alphaRange):0.0;
    double carbonEnergy = carbonRange>0 ? myRangeCalculator.getIonEnergyMeV(recoilPID, carbonRange):0.0;
    double m_Alpha = myRangeCalculator.getIonMassMeV(pid_type::ALPHA);
    double m_12C = myRangeCalculator.getIonMassMeV(isCarbon ? recoilPID : pid_type::CARBON_12); // recoil mass
    
    double p_alpha = sqrt(2*m_Alpha*alphaEnergy);
    double p_12C = sqrt(2*m_12C*carbonEnergy);
    TVector3 total_p = p_alpha*(alphaEnd-vertex).Unit() + p_12C*(carbonEnd-vertex).Unit();
    
    //    track_data.frameId = iEntry;
    track_data.frameId = iEntry;
    track_data.eventId = myEventInfo->GetEventId();
    track_data.eventType = eventType;
    track_data.length = length;    
    track_data.horizontalLostLength = horizontalTrackLostPart;
    track_data.verticalLostLength = verticalTrackLostPart;
    track_data.charge = charge;
    track_data.cosTheta = cosTheta;
    track_data.phi = phi;

    track_data.cosThetaDET = tangent.Z();
    track_data.phiDET = tangent.Phi();

    track_data.chi2 = chi2;
    
    track_data.xVtx = vertex.X();
    track_data.yVtx = vertex.Y();
    track_data.zVtx = vertex.Z();
    
    track_data.xAlphaEnd = alphaEnd.X();
    track_data.yAlphaEnd = alphaEnd.Y();
    track_data.zAlphaEnd = alphaEnd.Z();
    
    track_data.xCarbonEnd = carbonEnd.X();
    track_data.yCarbonEnd = carbonEnd.Y();
    track_data.zCarbonEnd = carbonEnd.Z();

    track_data.alphaEnergy = alphaEnergy;
    track_data.carbonEnergy = carbonEnergy;
    track_data.alphaRange = alphaRange;
    track_data.carbonRange = carbonRange;
    track_data.cosPhiSegments = cosPhiSegments;

    track_data.total_mom_x = total_p.x();
    track_data.total_mom_y = total_p.y();
    track_data.total_mom_z = total_p.z();

    track_data.lineFitLoss = aTrack3D.getLoss();
    track_data.dEdxFitLoss = aTrack3D.getHypothesisFitLoss();
    track_data.dEdxFitSigma = aTrack3D.getSegments().front().getDiffusion();    
    track_data.maxProjWidth = aTrack3D.getSegments().front().getMaxProjWidth();
    track_data.chargeEnergy = aTrack3D.getChargeEnergy();   // [MeV], 0 without a gain table entry
    track_data.chargeRatio = aTrack3D.getChargeRatio();     // Q/(G_raw E_dep) of the dE/dx fit, 0 if not computed
    tree->Fill();
    if(recoTree){
      *recoTrackPtr = aTrack3D;
      *recoInfoPtr = *myEventInfo;
      recoTree->Fill();
    }
  /*
  } // end of event loop
  */
    counter++;
  }
  while(1);
  outputROOTFile.Write();
  delete recoTrackPtr;
  delete recoInfoPtr;

  ////////////////////////////////////////////
  //
  // close file with fit DEBUG plots
  if(develMode) {
    outputCanvasROOTFile->Close();
    delete outputCanvasROOTFile;
    delete outputCanvas;
  }
  //
  ////////////////////////////////////////////

  return counter;
}
/////////////////////////////
////////////////////////////

