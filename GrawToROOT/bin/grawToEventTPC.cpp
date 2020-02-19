#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventCharges.h"
#include "PedestalCalculator.h"

#include "TFile.h"
#include "TTree.h"

#include "utl/Logging.h"
#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"
#include "get/TGrawFile.h"
#include "mfm/FrameDictionary.h"

int main(int argc, char* argv[]) {

	if (argc < 3) return -1;

	int minSignalCell = 51;
	int maxSignalCell = 500;
	bool skipEmptyEvents = false;

	///Load TPC geometry
	std::string geomFileName = "geometry_mini_eTPC.dat";
	std::string dataFileName = "/data/edaq/CoBo_2018-05-11T14:23:14.736_0000.graw";
	std::string timestamp = "";
	if (argc > 3) {
		geomFileName = std::string(argv[1]);
		std::cout << "geomFileName: " << geomFileName << std::endl;

		dataFileName = std::string(argv[2]);
		std::cout << "dataFileName: " << dataFileName << std::endl;

		timestamp = std::string(argv[3]);
		std::cout << "timestamp: " << timestamp << std::endl;
	}
	GeometryTPC& myGeometry = Geometry(geomFileName);

	EventCharges myEvent;

	PedestalCalculator myPedestalCalculator;

	///Create ROOT Tree
	std::string rootFileName = "EventTPC_" + timestamp + ".root";

	TFile aFile(rootFileName.c_str(), "RECREATE");
	TTree aTree("TPCData", "");

	aTree.Branch("Event", &myEvent);

	///Load data
	GET::GDataFrame dataFrame;
	TGrawFile f(dataFileName.c_str());
	long lastevent = f.GetGrawFramesNumber();

	myEvent().RunId(0);

	for (long eventId = 0; eventId < lastevent; ++eventId) {
		myEvent.Clear();
		myEvent().EventId(eventId);
		bool eventRead = f.GetGrawFrame(dataFrame, eventId);
		if (!eventRead) {
			std::cerr << "ERROR: cannot read event " << eventId << std::endl;
			return -1;
		}

		myPedestalCalculator.CalculateEventPedestals(dataFrame);

		int COBO_idx = 0;
		int ASAD_idx = 0;

		for (int32_t agetId = 0; agetId < myGeometry.GetAgetNchips(); ++agetId) {
			// loop over normal channels and update channel mask for clustering
			for (int32_t chanId = 0; chanId < myGeometry.GetAgetNchannels(); ++chanId) {
				int iChannelGlobal = myGeometry.Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)

				GET::GDataChannel* channel = dataFrame.SearchChannel(agetId, myGeometry.Aget_normal2raw(chanId));
				if (!channel) continue;

				for (int32_t i = 0; i < channel->fNsamples; ++i) {
					GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
					// skip cells outside signal time-window
					int32_t icell = sample->fBuckIdx;
					if (icell < 2 || icell>509 || icell<minSignalCell || icell>maxSignalCell) continue;

					double rawVal = sample->fValue;
					double corrVal = rawVal - myPedestalCalculator.GetPedestalCorrection(iChannelGlobal, agetId, icell);
					myEvent.AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);

				} // end of loop over time buckets	    
			} // end of AGET channels loop	
		} // end of AGET chips loop

	  ///Skip empty events
		if (skipEmptyEvents && myEvent.GetMaxCharge() < 100) continue;
		/////////////////////
		aTree.Fill();
	}

	aTree.Write();
	aFile.Close();
	return 0;
}
