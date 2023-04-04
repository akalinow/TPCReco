#include "ConfigManager.h"

ConfigManager::ConfigManager(){}

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){
    
    boost::program_options::options_description cmdLineOptDesc("Allowed options");
    cmdLineOptDesc.add_options()
        ("help", "produce help message")
        ("beamDir", boost::program_options::value<BeamDirection>(), "string - LAB gamma beam direction [\"x\" xor \"-x\"]")
        ("beamEnergy", boost::program_options::value<float>(), "float - LAB gamma beam energy [MeV]")
        ("chunk", boost::program_options::value<size_t>(), "uint - file chunk")
        ("clusterDeltaStrips", boost::program_options::value<int>(), "int - Envelope in strip units around seed hits for clustering")
        ("clusterDeltaTimeCells", boost::program_options::value<int>(), "int - Envelope in time cell units around seed hits for clustering")
        ("clusterEnable", boost::program_options::value<bool>(), "bool - Flag to enable clustering")
        ("clusterThreshold", boost::program_options::value<float>(), "float - ADC threshold above pedestal used for clustering")
        ("dataFile",  boost::program_options::value<std::string>(), "string - path to data file (OFFLINE) or directory (ONLINE). Overrides the value from the config file. In multi-GRAW mode specify several files separated by commas.")
        ("directory,d", boost::program_options::value<std::string>(),"string - directory to browse. Mutually exclusive with \"files\"")
        //("dry-run", "testing without modyfing files")
        ("ext",boost::program_options::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{std::string{".graw"}},".graw"),"allowed extensions")
        ("files,f",boost::program_options::value<std::vector<std::string>>()->multitoken(),"strings - list of files to browse. Mutually exclusive with \"directory\"")
        ("frameLoadRange", boost::program_options::value<unsigned int>(), "int - maximal number of frames to be read by event builder in single-GRAW mode")
        ("geometryFile",  boost::program_options::value<std::string>(), "string - path to the geometry file.")
        ("hitThr", boost::program_options::value<unsigned int>(), "int - minimal hit charge after pedestal subtraction [ADC units]")
        ("inplace", "overwrites the input file, mutally exclusive with 'output'")
        ("input,i", boost::program_options::value<std::string>(),"input root file")
        ("matchRadiusInMM", boost::program_options::value<float>(), "float - matching radius for strips and time cells from different U/V/W directions [mm]")
        ("ms", boost::program_options::value<int>(),"int - delay in ms")
        // ("no-info", "skip printing file and tree info on every line")
        // ("no-presence", "skip printing extra and missing events")
        // ("no-segments", "skip comparing number of segments")
        // ("no-type", "skip comparing event type")
        ("noTree", boost::program_options::bool_switch()->default_value(false), "skip creating additional TTree for 1,2,3-prongs (true = runs a bit faster)")
        ("output,o", boost::program_options::value<std::string>(),"output root file,mutally exclusive with 'inplace'")
        ("outputFile", boost::program_options::value<std::string>(), "string - path to the output ROOT file")
        ("pressure", boost::program_options::value<float>(), "float - CO2 pressure [mbar]")
        ("recoClusterDeltaStrips",  boost::program_options::value<int>(), "int - Envelope in strip units around seed hits for RECO cluster.")
        ("recoClusterDeltaTimeCells",  boost::program_options::value<int>(), "int - Envelope in time cell units around seed hits for RECO cluster.")
        ("recoClusterEnable",  boost::program_options::value<bool>(), "bool - Flag to enable RECO cluster.")
        ("recoClusterThreshold",  boost::program_options::value<double>(), "double - ADC threshold above pedestal for RECO cluster.")
        ("reference", boost::program_options::value<std::string>(),"reference root file")
        ("referenceDataFile",  boost::program_options::value<std::string>(), "string - path to reference data file")
        ("removePedestal",  boost::program_options::value<bool>(), "bool - Flag to control pedestal removal. Overrides the value from config file.")
        ("separator",boost::program_options::value<std::string>()->default_value("\n"),"string - separator")
        ("singleAsadGrawFile", boost::program_options::bool_switch()->default_value(false), "flag indicating multi-GRAW mode (default=FALSE)")
        ("testDataFile",  boost::program_options::value<std::string>(), "string - path to test data file")
        ("totalChargeThr", boost::program_options::value<unsigned int>(), "int - minimal event total charge after pedestal subtraction [ADC units]")
        ("verbose,v", "prints message for every duplicate");
    
    boost::program_options::variables_map varMap;        
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    boost::program_options::notify(varMap); 

    if (varMap.count("help")) {
        std::cout<<cmdLineOptDesc<<std::endl;
        exit(1);
    }
    return varMap;
}

boost::property_tree::ptree getConfig(int argc, char **argv){

    boost::property_tree::ptree tree;
    boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
    if(argc<2){
        std::cout<<" Usage: masterConfig.json"<<std::endl;
        boost::property_tree::read_json("/scratch/TPCReco/Utilities/config/masterConfig.json",tree);
        return tree;
    }
    else {
        std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
        boost::property_tree::read_json(argv[1], tree);
    }


    if (varMap.count("beamDir")) {
        tree.put("beamDir", varMap["beamDir"].as<BeamDirection>());
    }
    if (varMap.count("beamEnergy")) {
        tree.put("beamEnergy", varMap["beamEnergy"].as<float>());
    }
    if (varMap.count("chunk")) {
        tree.put("chunk", varMap["chunk"].as<size_t>());
    }
    if (varMap.count("clusterEnable")) {
        tree.put("clusterEnable", varMap["clusterEnable"].as<bool>());
	        if (tree.get<bool>("clusterEnable") == false) { // skip threshold, delta-strip, delta-timecells when clustering is disabled
			tree.put("clusterThreshold", 0.0);
			tree.put("clusterDeltaStrips", 0);
			tree.put("clusterDeltaTimeCells", 0);
		}
		else { // look for threshold, delta-strip, delta-timecells only when clustering is enabled
			if (varMap.count("clusterThreshold")) {
				tree.put("clusterThreshold", varMap["clusterThreshold"].as<float>());
			}
			if (varMap.count("clusterDeltaStrips")) {
				tree.put("clusterDeltaStrips", varMap["clusterDeltaStrips"].as<int>());
			}
			if (varMap.count("clusterDeltaTimeCells")) {
				tree.put("clusterDeltaTimeCells", varMap["clusterDeltaTimeCells"].as<int>());
			}
		}
    }
    if (varMap.count("directory")) {
        tree.put("directory", varMap["directory"].as<std::string>());
    }
    // if (varMap.count("dry-run")) {
    //     auto count = boost::size(tpcreco::utilities::filterDuplicates(
    //             inputTree, varMap.count("verbose")));
    //     std::cout << "Removed " << inputTree->GetEntries() - count
    //             << " duplicated entries keeping the younger (dry run)\n";
    //     inputFile->Close();
    //     return 0;
    // }
    if (varMap.count("ext")) {
        tree.put("ext", varMap["ext"].as<std::vector<std::string>>());
    }
    if (varMap.count("files")) {
        tree.put("files", varMap["files"].as<std::vector<std::string>>());
    }
    if (varMap.count("geometryFile")) {
        tree.put("geometryFile", varMap["geometryFile"].as<std::string>());
    }
    if (varMap.count("hitThr")) {
        tree.put("hitThr", varMap["hitThr"].as<unsigned int>());
    }
    if (varMap.count("input")) {
        tree.put("input", varMap["input"].as<std::string>());
    }
    if (varMap.count("inplace")) {
        tree.put("output", varMap["input"].as<std::string>());
    }
    if (varMap.count("matchRadiusInMM")) {
        tree.put("matchRadiusInMM", varMap["matchRadiusInMM"].as<float>());
    }
    if (varMap.count("ms")) {
        tree.put("ms", varMap["ms"].as<int>());
    }
    // if (varMap.count("no-info")) {
    //     tree.put("no-info", true);
    // }
    // if (varMap.count("no-presence")) {
    //     tree.put("no-presence", true);
    // }
    // if (varMap.count("no-segments")) {
    //     tree.put("no-segments", true);
    // }
    // if (varMap.count("no-type")) {
    //     tree.put("no-type", true);
    // }
    if (varMap.count("noTree")) {
        tree.put("noTree", varMap["noTree"].as<bool>());
    }
    if (varMap.count("output")) {
        tree.put("output", varMap["output"].as<std::string>());
    }
    if (varMap.count("outputFile")) {
        tree.put("outputFile", varMap["outputFile"].as<std::string>());
    }
    if (varMap.count("pressure")) {
        tree.put("pressure", varMap["pressure"].as<float>());
    }
    if (varMap.count("recoClusterDeltaStrips")) {
        tree.put("hitFilter.recoClusterDeltaStrips", varMap["recoClusterDeltaStrips"].as<int>());
    }
    if(varMap.count("recoClusterDeltaTimeCells")){
        tree.put("hitFilter.recoClusterDeltaTimeCells", varMap["recoClusterDeltaTimeCells"].as<int>());
    }
    if (varMap.count("recoClusterEnable")) {
        tree.put("hitFilter.recoClusterEnable", varMap["recoClusterEnable"].as<bool>());
    }

    if (varMap.count("recoClusterThreshold")) {
        tree.put("hitFilter.recoClusterThreshold", varMap["recoClusterThreshold"].as<double>());
    }

    if (varMap.count("reference")) {
        tree.put("reference", varMap["reference"].as<std::string>());
    }

    if (varMap.count("referenceDataFile")) {
        tree.put("referenceDataFile", varMap["referenceDataFile"].as<std::string>());
    }

    if (varMap.count("removePedestal")) {
        tree.put("removePedestal", varMap["removePedestal"].as<bool>());
    }

    if (varMap.count("separator")) {
        tree.put("separator", varMap["separator"].as<std::string>());
    }

    if ((tree.find("singleAsadGrawFile") == tree.not_found() || // if not present in config JSON
		tree.get<bool>("singleAsadGrawFile") == false) && // or single-GRAW mode is FALSE
		varMap.count("singleAsadGrawFile")) { // then allow to override JSON settings
	tree.put("singleAsadGrawFile", varMap["singleAsadGrawFile"].as<bool>());
    }
    if (tree.get<bool>("singleAsadGrawFile") == false && // if in single-GRAW mode
		varMap.count("frameLoadRange")) { // then allow to override JSON settings
		tree.put("frameLoadRange", varMap["frameLoadRange"].as<unsigned int>());
    }
    if (varMap.count("testDataFile")) {
        tree.put("testDataFile", varMap["testDataFile"].as<std::string>());
    }

    if (varMap.count("totalChargeThr")) {
        tree.put("totalChargeThr", varMap["totalChargeThr"].as<unsigned int>());
    }

    if (varMap.count("verbose")) {
        tree.put("verbose", true);
    }


}

void setEventType(boost::property_tree::ptree &tree, event_type evtype){
    tree.put("eventType",evtype)
}