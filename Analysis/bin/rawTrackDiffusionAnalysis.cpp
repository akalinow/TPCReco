#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include <TFile.h>
#include <TGraph.h>
#include <TH2D.h>
#include <set>
#include <sstream>
#include <TBranch.h>
#include <TLeaf.h>
#include <TTreeIndex.h>

#include "TPCReco/colorText.h"
#include "TPCReco/GeometryTPC.h"
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/EventSourceROOT.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/TrackDiffusion_tree_analysis.h"
#include "TPCReco/RunIdParser.h"

int analyzeTrackDiffusion(const boost::property_tree::ptree aConfig);

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc
    ("Fills a ROOT tree with MEAN and RMS of 2D hits projected onto the transverse axis\n"
     "w.r.t. the direction of the leading track in each event.\n\n"
     "The transverse axis is defined by rotating the track direction clockwise by 90 deg\n"
     "(NOTE: this is important for correct interpretatation of the signed MEAN value).\n"
     "Separate MEAN and RMS sets are computed for each of UZ, VZ and WZ projections\n"
     "as well as for combination of all three strip direcions.\n"
     "For a given 2D projection only hits within certain rectangular region of interest (ROI)\n"
     "of WIDTH x HEIGHT area are accepted, where:\n\n"
     "- WIDTH = 2 * trackDistaneMM\n"
     "- HEIGHT = track_length * (1 - trackFractionStart - trackFractionEnd)\n"
     "- trackFractionStart > 0, trackFractionEnd > 0, trackFractionStart + trackFractionEnd < 1.\n\n"
     "In case of overlapping ROIs for multiple tracks, the MEAN and RMS will not be calculated\n"
     "for a given strip projection, and the corresponding quality flag will be set to FALSE.\n\n"
     "Hits are weighted by their charge; only hits with charge above hitThreshold (default 0) are used,\n"
     "so that negative (pedestal-subtracted) noise does not widen the distribution.\n\n"
     "recoFile: a makeTrackTree output with the TPCRecoData tree (--output.recoTree true), or a plain\n"
     "makeTrackTree ntuple (trackTree): then the tracks are rebuilt from the vertex and the alpha/carbon\n"
     "end points (alpha: vertex->alpha end; alpha+C events also vertex->carbon end).\n\n"
     "Optional event display dump (JSON block \"eventDump\"): {\"file\": out.root, \"eventIds\": \"id1,id2\",\n"
     "\"only\": true}. For the listed events the hit-filtered U/V/W-time projections [mm] (hU_<id>, hV_<id>,\n"
     "hW_<id>) and the 2D projections of the track segments (gU_<id>_<iSeg>: start, end) are written to file;\n"
     "with only=true the other events are skipped.\n\n"
     "Allowed command line options");

  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to TPC geometry file")
    ("recoFile",  boost::program_options::value<std::string>(), "string - path to a RECO file with Track3D collection corresponding to raw data file(s)")
#ifdef WITH_GET
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to a raw data file in ROOT/GRAW format (or list of comma-separated files in multi-GRAW mode) ")
    ("frameLoadRange", boost::program_options::value<unsigned int>(), "int - maximal number of frames to be read by event builder in single-GRAW mode")
    ("singleAsadGrawFile", boost::program_options::bool_switch()->default_value(false), "bool - flag indicating multi-GRAW mode (default=FALSE)")
    ("removePedestal",  boost::program_options::value<bool>(), "bool - flag to control pedestal removal. Overrides the value from config file.")
#else
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to a raw data file in ROOT format")
#endif
    // optional value (not a switch with a default): a switch is always "set", so it overwrote the JSON config
    ("recoClusterEnable",  boost::program_options::value<bool>(), "bool - enable clustering. Overrides hitFilter.recoClusterEnable of the config file (default if absent in both: FALSE)")
    ("recoClusterThreshold",  boost::program_options::value<float>(), "float - ADC threshold above pedestal used for clustering")
    ("recoClusterDeltaStrips",  boost::program_options::value<int>(), "int - envelope in strip units around seed hits for clustering")
    ("recoClusterDeltaTimeCells",  boost::program_options::value<int>(), "int - envelope in time cell units around seed hits for clustering")
    ("trackFractionStart",  boost::program_options::value<float>(), "float - hit veto region at the beginning of the track as a fraction [0-1] of track length")
    ("trackFractionEnd",  boost::program_options::value<float>(), "float - hit veto region at the end of the track as a fraction [0-1] of track length")
    ("trackDistanceMM",  boost::program_options::value<float>(), "float - maximal allowed hit distance [mm] from the track axis")
    ("hitThreshold",  boost::program_options::value<float>(), "float - only hits with charge above this value [ADC] are used for MEAN and RMS (default=0: negative pedestal-subtracted noise is excluded)")
    ("outputFile", boost::program_options::value<std::string>(), "string - path to the output ROOT file")
    ("maxNevents", boost::program_options::value<unsigned int>()->default_value(0), "int - number of events to process (0=all)");

  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << std::endl
		<< "rawTrackDiffusionAnalysis config.json [options]" << std::endl << std::endl;
      std::cout << cmdLineOptDesc << std::endl;
      exit(1);
    }
    boost::program_options::notify(varMap);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::cout << cmdLineOptDesc << std::endl;
    exit(1);
  }

  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<2){
    std::cout << std::endl
	      << "rawTrackDiffusionAnalysis config.json [options]" << std::endl << std::endl;
    return 0;
  }
  else {
    std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
    boost::property_tree::read_json(argv[1], tree);
  }

  // optional overrides of the JSON config file
  if (varMap.count("outputFile")) {
    tree.put("outputFile", varMap["outputFile"].as<std::string>());
  }
  if (varMap.count("geometryFile")) {
    tree.put("geometryFile", varMap["geometryFile"].as<std::string>());
  }
  if (varMap.count("recoFile")) {
    tree.put("recoFile", varMap["recoFile"].as<std::string>());
  }
  if (varMap.count("dataFile")) {
    tree.put("dataFile", varMap["dataFile"].as<std::string>());
  }
  if (varMap.count("maxNevents")) {
    tree.put("maxNevents", varMap["maxNevents"].as<unsigned int>());
  }
  if(varMap.count("recoClusterEnable")) {
    tree.put("hitFilter.recoClusterEnable", varMap["recoClusterEnable"].as<bool>());
  }
  else if(!tree.get_optional<bool>("hitFilter.recoClusterEnable")) { // neither command line nor config file: disabled
    tree.put("hitFilter.recoClusterEnable", false);
  }
  if(tree.get<bool>("hitFilter.recoClusterEnable")==false) { // skip threshold, delta-strip, delta-timecells when clustering is disabled
    tree.put("hitFilter.recoClusterThreshold", 0.0);
    tree.put("hitFilter.recoClusterDeltaStrips", 0);
    tree.put("hitFilter.recoClusterDeltaTimeCells", 0);
  } else { // look for threshold, delta-strip, delta-timecells only when clustering is enabled
    if(varMap.count("recoClusterThreshold")) {
      tree.put("hitFilter.recoClusterThreshold", varMap["recoClusterThreshold"].as<float>());
    }
    if(varMap.count("recoClusterDeltaStrips")) {
      tree.put("hitFilter.recoClusterDeltaStrips", varMap["recoClusterDeltaStrips"].as<int>());
    }
    if(varMap.count("recoClusterDeltaTimeCells")) {
      tree.put("hitFilter.recoClusterDeltaTimeCells", varMap["recoClusterDeltaTimeCells"].as<int>());
    }
  }
  if(varMap.count("trackFractionStart")) {
    tree.put("trackDiffusion.trackFractionStart", varMap["trackFractionStart"].as<float>());
  }
  if(varMap.count("trackFractionEnd")) {
    tree.put("trackDiffusion.trackFractionEnd", varMap["trackFractionEnd"].as<float>());
  }
  if(varMap.count("trackDistanceMM")) {
    tree.put("trackDiffusion.trackDistanceMM", varMap["trackDistanceMM"].as<float>());
  }
  if(varMap.count("hitThreshold")) {
    tree.put("trackDiffusion.hitThreshold", varMap["hitThreshold"].as<float>());
  }
#ifdef WITH_GET
  if (varMap.count("removePedestal")) {
    tree.put("removePedestal", varMap["removePedestal"].as<bool>());
  }
  if( (tree.find("singleAsadGrawFile")==tree.not_found() || // if not present in config JSON
       tree.get<bool>("singleAsadGrawFile")==false) && // or single-GRAW mode is FALSE
      varMap.count("singleAsadGrawFile")){ // then allow to override JSON settings
    tree.put("singleAsadGrawFile", varMap["singleAsadGrawFile"].as<bool>());
  }
  if( tree.get<bool>("singleAsadGrawFile")==false && // if in single-GRAW mode
      varMap.count("frameLoadRange")) { // then allow to override JSON settings
    tree.put("frameLoadRange", varMap["frameLoadRange"].as<unsigned int>());
  }
#endif

  //sanity checks
  if(tree.find("recoFile")==tree.not_found() ||
     tree.find("dataFile")==tree.not_found() ||
     tree.find("geometryFile")==tree.not_found() ||
     tree.find("outputFile")==tree.not_found() ||
#ifdef WITH_GET
     tree.find("singleAsadGrawFile")==tree.not_found() ||
     (tree.find("singleAsadGrawFile")!=tree.not_found() &&
      tree.find("frameLoadRange")==tree.not_found()) ||
     tree.find("removePedestal")==tree.not_found() ||
#endif
     tree.get_child("hitFilter").find("recoClusterEnable")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterThreshold")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterDeltaStrips")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterDeltaTimeCells")==tree.not_found() ||
     tree.find("maxNevents")==tree.not_found()
     ) {
    std::cerr << std::endl
	      << __FUNCTION__ << KRED << ": Some configuration options are missing!" << RST << std::endl << std::endl;
    std::cout << "recoFile: " << tree.count("recoFile") << std::endl;
    std::cout << "dataFile: " << tree.count("dataFile") << std::endl;
    std::cout << "geometryFile: " << tree.count("geometryFile") << std::endl;
    std::cout << "outputFile: " << tree.count("outputFile") << std::endl;
    std::cout << "recoClusterEnable: " << tree.get_child("hitFilter").count("recoClusterEnable") << std::endl;
    std::cout << "recoClusterThreshold: " << tree.get_child("hitFilter").count("recoClusterThreshold") << std::endl;
    std::cout << "recoClusterDeltaStrips: " << tree.get_child("hitFilter").count("recoClusterDeltaStrips") << std::endl;
    std::cout << "recoClusterDeltaTimeCells: " << tree.get_child("hitFilter").count("recoClusterDeltaTimeCells") << std::endl;
#ifdef WITH_GET
    std::cout << "singleAsadGrawFile: " << tree.count("singleAsadGrawFile") << std::endl;
    std::cout << "frameLoadRange: " << tree.count("frameLoadRange") << std::endl;
    std::cout << "removePedestal: " << tree.count("removePedestal") << std::endl;
#endif
    std::cout << "maxNevents:" << tree.count("maxNevents") << std::endl;
    exit(1);
  }

  // start analysis job
  int nEventsProcessed = analyzeTrackDiffusion(tree);
  return 0;
}

int analyzeTrackDiffusion(const boost::property_tree::ptree aConfig){

  auto geometryFileName = aConfig.get<std::string>("geometryFile");
  auto recoFileName = aConfig.get<std::string>("recoFile");
  auto dataFileName = aConfig.get<std::string>("dataFile");
  auto outputFileName = aConfig.get<std::string>("outputFile");
  auto clusterEnable = aConfig.get<bool>("hitFilter.recoClusterEnable");
  auto clusterThreshold = ( clusterEnable ? aConfig.get<float>("hitFilter.recoClusterThreshold") : 0 );
  auto clusterDeltaStrips = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaStrips") : 0 );
  auto clusterDeltaTimeCells = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaTimeCells") : 0 );
  auto trackFractionStart = aConfig.get<float>("trackDiffusion.trackFractionStart");
  auto trackFractionEnd = aConfig.get<float>("trackDiffusion.trackFractionEnd");
  auto trackDistanceMM = aConfig.get<float>("trackDiffusion.trackDistanceMM");
  auto hitThreshold = aConfig.get<float>("trackDiffusion.hitThreshold", 0.0);
#ifdef WITH_GET
  auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
  auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only
  auto removePedestal = aConfig.get<bool>("removePedestal");
#endif
  auto maxNevents = aConfig.get<unsigned int>("maxNevents");

  std::cout << std::endl << "analyzeRawEvents: Parameter settings: " << std::endl << std::endl
	    << "Reco file                    = " << recoFileName << std::endl
	    << "Data file(s)                 = " << dataFileName << std::endl
	    << "TPC geometry file            = " << geometryFileName << std::endl
	    << "Output file                  = " << outputFileName << std::endl
	    << "RecoCluster enable           = " << clusterEnable << std::endl
	    << "RecoCluster threshold        = " << clusterThreshold << std::endl
	    << "RecoCluster delta strips     = " << clusterDeltaStrips << std::endl
	    << "RecoCluster delta time cells = " << clusterDeltaTimeCells << std::endl
	    << "Track fraction at start      = " << trackFractionStart << std::endl
	    << "Track fraction at end        = " << trackFractionEnd << std::endl
	    << "Max. distance to track       = " << trackDistanceMM << " mm" << std::endl
	    << "Hit charge threshold         = " << hitThreshold << " ADC" << std::endl
#ifdef WITH_GET
	    << "Frame load range             = " << frameLoadRange << std::endl
	    << "Multi-GRAW mode              = " << singleAsadGrawFile << std::endl
	    << "Pedestal removal enable      = " << removePedestal << std::endl
#endif
	    << "Max. events to process       = " << maxNevents << " (0=all)" << std::endl;

  // sanity checks
  if(recoFileName.find(".root") == std::string::npos ||
#ifdef WITH_GET
     (dataFileName.find(".graw") == std::string::npos && dataFileName.find(".root") == std::string::npos) ||
#else
     dataFileName.find(".root") == std::string::npos ||
#endif
     geometryFileName.find(".dat") == std::string::npos ||
     outputFileName.find(".root") == std::string::npos) {
    std::cerr << __FUNCTION__ << KRED << ": Wrong input argument(s)." << std::endl
#ifdef WITH_GET
	      << "Check that input ROOT/GRAW file(s) and geometry file are correct." << std::endl
#else
	      << "Check that input ROOT file and geometry file are correct." << std::endl
#endif
	      << "The output ROOT file must not be present."
              << RST << std::endl;
    exit(1);
  }
  if(trackDistanceMM<=0.0 || trackFractionStart<0.0 || trackFractionEnd<0.0 || (trackFractionStart+trackFractionEnd)>1.0) {
    std::cerr << __FUNCTION__ << KRED << ": Wrong input argument(s)." << std::endl
	      << "Check that (trackDistanceMM >0) and (trackFractionStart >=0) and (trackFractionEnd >=0)"
	      << " and (trackFractionStart+trackFractionEnd <=1)." << RST << std::endl;
    exit(1);
  }

  // initialize input RECO file with Track3D collection
  auto myRecoFilePtr = new TFile(recoFileName.c_str(), "READ");
  // std::shared_ptr<TFile> myRecoFilePtr;
  // myRecoFilePtr.reset(new TFile(recoFileName.c_str(), "OLD"));
  if(!myRecoFilePtr && recoFileName.size()>0) {
    std::cerr<<"ERROR: Cannot open input RECO file:" << recoFileName << "!" << std::endl;
    exit(1);
  }

  // Reco input: the TPCRecoData tree (Track3D + EventInfo) or, if absent, the trackTree ntuple of makeTrackTree.
  // The ntuple stores the vertex and the alpha / carbon end points, which is all that the diffusion analysis
  // uses from the track (segment start, end and PID); it has no run number (one run per raw-data chunk).
  struct RecoEntry { long runId; unsigned long eventId; Long64_t entry; };
  std::vector<RecoEntry> recoEntries;
  auto aTrack = new Track3D();
  auto aEventInfo = new eventraw::EventInfo();
  TBranch *aBranch = nullptr, *aBranchInfo = nullptr;
  auto myTreePtr = (TTree*)myRecoFilePtr->Get("TPCRecoData");
  auto myNtuplePtr = myTreePtr ? nullptr : (TTree*)myRecoFilePtr->Get("trackTree");
  std::vector<Track3D> ntupleTracks; // tracks rebuilt from the ntuple
  if(myTreePtr) {
    aBranch = myTreePtr->GetBranch("RecoEvent");
    if(!aBranch) {
      std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
      exit(1);
    }
    aBranch->SetAddress(&aTrack);
    aBranchInfo = myTreePtr->GetBranch("EventInfo");
    if(!aBranchInfo) {
      std::cerr<<"ERROR: Cannot find 'EventInfo' branch!"<<std::endl;
      exit(1);
    }
    aBranchInfo->SetAddress(&aEventInfo);
    for(Long64_t iEntry=0; iEntry<myTreePtr->GetEntries(); ++iEntry) {
      aBranchInfo->GetEntry(iEntry);
      recoEntries.push_back({(long)aEventInfo->GetRunId(), (unsigned long)aEventInfo->GetEventId(), iEntry});
    }
  } else if(myNtuplePtr) {
    std::cout << "Reco file without TPCRecoData: tracks rebuilt from the trackTree ntuple" << std::endl;
    auto leaf = [&](const char *name) {
      auto aLeaf = myNtuplePtr->GetLeaf("track", name);
      if(!aLeaf) {
        std::cerr<<"ERROR: Cannot find leaf 'track."<<name<<"' in trackTree!"<<std::endl;
        exit(1);
      }
      return aLeaf;
    };
    auto lEventId = leaf("eventId"), lEventType = leaf("eventType");
    auto lxV = leaf("xVtx"), lyV = leaf("yVtx"), lzV = leaf("zVtx");
    auto lxA = leaf("xAlphaEnd"), lyA = leaf("yAlphaEnd"), lzA = leaf("zAlphaEnd");
    auto lxC = leaf("xCarbonEnd"), lyC = leaf("yCarbonEnd"), lzC = leaf("zCarbonEnd");
    for(Long64_t iEntry=0; iEntry<myNtuplePtr->GetEntries(); ++iEntry) {
      myNtuplePtr->GetEntry(iEntry);
      // eventType = sum of the segment PIDs (makeTrackTree): ALPHA=1, ALPHA+CARBON_12=3, ALPHA+CARBON_14=4, 0=no fit
      const int eventType = (int)std::lround(lEventType->GetValue());
      const TVector3 vertex(lxV->GetValue(), lyV->GetValue(), lzV->GetValue());
      const TVector3 alphaEnd(lxA->GetValue(), lyA->GetValue(), lzA->GetValue());
      const TVector3 carbonEnd(lxC->GetValue(), lyC->GetValue(), lzC->GetValue());
      Track3D track;
      TrackSegment3D first;
      first.setStartEnd(vertex, alphaEnd);
      first.setPID(eventType>=1 ? pid_type::ALPHA : pid_type::UNKNOWN);
      track.addSegment(first);
      if(eventType==3 || eventType==4) {
        TrackSegment3D recoil;
        recoil.setStartEnd(vertex, carbonEnd);
        recoil.setPID(eventType==3 ? pid_type::CARBON_12 : pid_type::CARBON_14);
        track.addSegment(recoil);
      }
      ntupleTracks.push_back(track);
      recoEntries.push_back({-1, (unsigned long)std::lround(lEventId->GetValue()), (Long64_t)ntupleTracks.size()-1});
    }
  } else {
    std::cerr<<"ERROR: Cannot find 'TPCRecoData' or 'trackTree' tree!"<<std::endl;
    exit(1);
  }
  const unsigned int nEntries = recoEntries.size();

  // ascending order of {runId, eventId}, as in the raw data
  std::sort(recoEntries.begin(), recoEntries.end(), [](const RecoEntry &a, const RecoEntry &b) {
    return a.runId!=b.runId ? a.runId<b.runId : a.eventId<b.eventId; });

  // initialize input EventSource  
  const char del = ','; // delimiter character
  std::set<std::string> fileNameList; // list of unique strings
  std::stringstream sstream(dataFileName);
  std::string fileName;
  while (std::getline(sstream, fileName, del)) {
    if(fileName.size()>0) fileNameList.insert(fileName);
  };
  
  std::shared_ptr<EventSourceBase> myEventSource;
#ifdef WITH_GET
  if(dataFileName.find(".graw")!=std::string::npos && dataFileName.find(".root")==std::string::npos) {
    if(singleAsadGrawFile) {
      myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
      unsigned int AsadNboards=dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
      if (fileNameList.size()>AsadNboards) {
	std::cerr << __FUNCTION__ << KRED << ": Provided too many input GRAW files. Expected up to " << AsadNboards << RST << std::endl;
	exit(1);
      }
    } else {
      myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
      dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(frameLoadRange);
      if (fileNameList.size()>1) {
	std::cerr << __FUNCTION__ << KRED << ": Provided too many input GRAW files. Expected 1." << RST << std::endl;
	exit(1);
      }
    }
  } else {
#endif
    if((dataFileName.find(".root")!=std::string::npos && dataFileName.find(".graw")!=std::string::npos) ||
       fileNameList.size()>1) {
      std::cerr << __FUNCTION__ << KRED << ": Provided too many input ROOT files. Expected 1." << RST << std::endl;
      exit(1);
    }

    myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
#ifdef WITH_GET
  }
#endif
  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEvents() << " frames loaded."
	    << std::endl;
  myEventSource->loadFileEntry(0); // load 1st frame (NOTE: otherwise LoadEventId does not work)

#ifdef WITH_GET
  if(dataFileName.find(".graw")!=std::string::npos) {
    auto id = RunIdParser(dataFileName);
    std::cout << "Parsing list of file names: " << dataFileName
	      << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << std::endl;
    
    // initialize pedestal removal parameters for EventSource: cell windows from the optional "pedestal"
    // block, removal from removePedestal (was find("pedestal")->second: a crash without the block, and
    // removePedestal / --removePedestal were ignored)
    boost::property_tree::ptree pedestalConfig = aConfig.get_child("pedestal", boost::property_tree::ptree());
    pedestalConfig.put("remove", removePedestal);
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(pedestalConfig);
    }
#endif

  // initialize TrackDiffusion_tree_analysis
  TrackDiffusion_tree_analysis::TrackDiffusionConfig myConfig;
  myConfig.clusterEnable = clusterEnable;
  myConfig.clusterThreshold = clusterThreshold;
  myConfig.clusterDeltaStrips = clusterDeltaStrips;
  myConfig.clusterDeltaTimeCells = clusterDeltaTimeCells;
  myConfig.trackFractionStart = trackFractionStart;
  myConfig.trackFractionEnd = trackFractionEnd;
  myConfig.trackDistanceMM = trackDistanceMM;
  myConfig.hitThreshold = hitThreshold;
  TrackDiffusion_tree_analysis myAnalysis(myEventSource->getGeometry(), myConfig, outputFileName);

  // optional event display dump
  std::set<unsigned long> dumpEventIds;
  std::unique_ptr<TFile> dumpFile;
  const bool dumpOnly = aConfig.get<bool>("eventDump.only", false);
  if(aConfig.get_optional<std::string>("eventDump.file")) {
    std::stringstream ids(aConfig.get<std::string>("eventDump.eventIds", ""));
    std::string anId;
    while(std::getline(ids, anId, ',')) if(!anId.empty()) dumpEventIds.insert(std::stoul(anId));
    dumpFile = std::make_unique<TFile>(aConfig.get<std::string>("eventDump.file").c_str(), "RECREATE");
    std::cout << "Event dump: " << dumpEventIds.size() << " events to " << dumpFile->GetName()
              << (dumpOnly ? " (only these events)" : "") << std::endl;
  }

  // loop over ALL events
  Long64_t currentEventIdx=-1;
  Long64_t counter=0;
  Long64_t maxevents=(maxNevents<=0 ? nEntries : std::min((unsigned int)maxNevents, nEntries));
  for(auto ievent=0; ievent<maxevents; ievent++) {
    const auto &recoEntry = recoEntries[ievent];
    if(myTreePtr) {
      aBranch->GetEntry(recoEntry.entry);
    } else {
      *aTrack = ntupleTracks[recoEntry.entry];
    }
    const long runId = recoEntry.runId; // -1: not known (ntuple input)
    const unsigned long eventId = recoEntry.eventId;
    const int nTracks = aTrack->getSegments().size();
    const bool dumpThis = dumpFile && dumpEventIds.count(eventId);
    if(dumpOnly && !dumpThis) continue;

    ////// DEBUG
    std::cout << __FUNCTION__ << ": RECO: run=" << runId << ", evt=" << eventId << ", ntracks=" << nTracks << std::endl;
    ////// DEBUG

    // sanity check
    if(nTracks==0) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event without any tracks: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }

    // load EventTPC from the RAW file
    myEventSource->loadEventId(eventId);
    auto aEventTPC = myEventSource->getCurrentEvent();
    auto currentEventId = aEventTPC->GetEventInfo().GetEventId();
    auto currentRunId = aEventTPC->GetEventInfo().GetRunId();

    // sanity check
    if((runId>=0 && (long)currentRunId!=runId) || currentEventId!=eventId) {
      std::cout << __FUNCTION__ << KRED << ": Skipping event with missing RAW data: run=" << runId << ", event=" << eventId
		<< RST << std::endl << std::flush;
      continue;
    }
    ////// DEBUG
    std::cout << __FUNCTION__ << ": RAW DATA: " << aEventTPC->GetEventInfo() << std::endl;
    ////// DEBUG
    
    // assign correct geometry pointer to track segments
    for(auto &aSeg: aTrack->getSegments()) {
      aSeg.setGeometry(myEventSource->getGeometry());
    }

    // get sorted list of tracks (descending order by track length)
    auto coll=aTrack->getSegments();
    std::sort(coll.begin(), coll.end(),
    	      [](const TrackSegment3D& a, const TrackSegment3D& b) {
    		return a.getLength() > b.getLength();
    	      });
    
    // fill diagnostic tree with raw signal properties
    myAnalysis.fillTree(myEventSource->getCurrentEvent(), aTrack);

    if(dumpThis) {
      // fillTree has set the hit filter of the event: the same hits as used by the analysis
      const filter_type filterType = clusterEnable ? filter_type::threshold : filter_type::none;
      const char *dirName[3] = {"U", "V", "W"};
      dumpFile->cd();
      for(int strip_dir=0; strip_dir<3; ++strip_dir) {
        auto h = aEventTPC->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
        if(h) {
          TH2D *hCopy = (TH2D*)h->Clone(Form("h%s_%lu", dirName[strip_dir], eventId));
          hCopy->SetDirectory(dumpFile.get());
          hCopy->Write();
        }
        int iSeg = 0;
        for(const auto &aSeg: aTrack->getSegments()) {
          TrackSegment2D aProj = aSeg.get2DProjection(strip_dir, 0, aSeg.getLength());
          TGraph g(2);
          g.SetPoint(0, aProj.getStart().X(), aProj.getStart().Y());
          g.SetPoint(1, aProj.getEnd().X(), aProj.getEnd().Y());
          g.SetTitle(Form("pid=%d", (int)aSeg.getPID()));
          g.Write(Form("g%s_%lu_%d", dirName[strip_dir], eventId, iSeg++));
        }
      }
    }

    counter++;
  } // end of main event loop...
  
  if(dumpFile) dumpFile->Close();
  if(myRecoFilePtr) myRecoFilePtr->Close(); // this explicit Close() prevents code crashes
  return counter;
}
