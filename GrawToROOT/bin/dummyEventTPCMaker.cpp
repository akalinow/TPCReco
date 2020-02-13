#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "TFile.h"
#include "TTree.h"

int main() {

	std::string pathFileName = "paths.txt";
	std::string geomFileName;
	std::fstream path_file;
	path_file.open(pathFileName, std::ios::in);
	path_file >> geomFileName;
	path_file.close();
	GeometryTPC& myGeometry = Geometry(geomFileName);

	///Create event
	EventTPC myEvent;

	///Create ROOT Tree
	std::string rootFileName = "EventTPC_dummy.root";

	TFile aFile(rootFileName.c_str(), "RECREATE");
	TTree aTree("TPCData", "");

	aTree.Branch("Event", &myEvent);

	myEvent.Info().RunId(0);
	auto lastevent = 100;
	for (long eventId = 0; eventId < lastevent; ++eventId) {
		myEvent.Clear();
		myEvent.Info().EventId(eventId);

		int COBO_idx = 0;
		int ASAD_idx = 0;

		for (int32_t agetId = 0; agetId < myGeometry.GetAgetNchips(); ++agetId) {
			// loop over normal channels and update channel mask for clustering
			for (int32_t chanId = 0; chanId < myGeometry.GetAgetNchannels(); ++chanId) {
				int iChannelGlobal = myGeometry.Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)
				for (int32_t i = 0; i < 1; ++i) {
					double aVal = 500;
					int32_t icell = 125;
					myEvent.AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, aVal);
				} // end of loop over time buckets	    
			} // end of AGET channels loop	
		} // end of AGET chips loop
		aTree.Fill();
	}

	aTree.Write();
	aFile.Close();
	return 0;
}
