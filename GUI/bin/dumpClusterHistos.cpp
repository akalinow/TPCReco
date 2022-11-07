#ifdef WITH_GET

#include "TFile.h"

#include "HistoManager.h"
//#include "EventSourceROOT.h"
#include "EventSourceGRAW.h"

#include <boost/property_tree/json_parser.hpp>

int main(int argc, char *argv[]) {
  if (argc < 5) {
    std::cout << "Wrong number of arguments. Should be:" << std::endl
              << argv[0]
              << " <input_file.graw> <file_entry_number> <geometry_file.dat> "
                 "<result_file.root>"
              << std::endl
              << std::endl;
    return -1;
  }
  std::string dataFileName(argv[1]);
  unsigned int FileEntry = atoi(argv[2]);
  std::string geometryFileName(argv[3]);
  std::string rootFileName(argv[4]);
  std::shared_ptr<EventSourceBase> myEventSource;
  if (dataFileName.find(".graw") != std::string::npos &&
      geometryFileName.find(".dat") != std::string::npos &&
      rootFileName.find(".root") != std::string::npos) {

    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    dynamic_cast<EventSourceGRAW *>(myEventSource.get())
        ->setFrameLoadRange(4); // 4 frames

    //    myEventSource = std::make_shared<EventSourceGRAW>();
    //    myEventSource->loadGeometry(geometryFileName);
    myEventSource->loadDataFile(dataFileName);
    std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
              << std::endl;
  } else {
    std::cout << "Wrong input arguments. Should be:" << std::endl
              << argv[0]
              << " <input_file.graw> <file_entry_number> <geometry_file.dat> "
                 "<result_file.root>"
              << std::endl
              << std::endl;
    return -1;
  }
  HistoManager myHistoManager;
  myHistoManager.setGeometry(myEventSource->getGeometry());

  TFile aFile(rootFileName.c_str(), "RECREATE");

  if (FileEntry < myEventSource->numberOfEntries()) {
    myEventSource->loadFileEntry(FileEntry);
    std::cout << "EventID: " << myEventSource->currentEventNumber() << std::endl
              << std::flush;
    myHistoManager.setEvent(myEventSource->getCurrentEvent());
    myHistoManager.reconstruct(); // triggers raw data clustering


    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto projType = get2DProjectionType(strip_dir);
      // Raw STRIP VS TIME plots per STRIP direction:
      myHistoManager.get2DProjection(projType, filter_type::none, scale_type::raw)->Write();
      // Raw STRIP VS TIME plots in milimiters per STRIP direction:
      myHistoManager.get2DProjection(projType, filter_type::none, scale_type::mm)->Write();
      // Clustered STRIP VS TIME plots per STRIP direction:
      myHistoManager.get2DProjection(projType, filter_type::threshold, scale_type::raw)->Write();
      // Clustered STRIP VS TIME plots in milimiters per STRIP direction:
      myHistoManager.get2DProjection(projType, filter_type::threshold, scale_type::mm)->Write();
      // Raw STRIP projection per STRIP direction
      myHistoManager.get1DProjection(static_cast<projection_type>(strip_dir), filter_type::none, scale_type::raw)->Write();
      // Raw STRIP projection in millimiters per STRIP direction
      myHistoManager.get1DProjection(static_cast<projection_type>(strip_dir), filter_type::none, scale_type::mm)->Write();
      // Clustered STRIP projection in per STRIP direction
      myHistoManager.get1DProjection(static_cast<projection_type>(strip_dir), filter_type::threshold, scale_type::raw)->Write();
      // Clustered STRIP projection in millimiters per STRIP direction
      myHistoManager.get1DProjection(static_cast<projection_type>(strip_dir), filter_type::threshold, scale_type::mm)->Write();
    }

    // Raw TIME projection
    myHistoManager.get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->Write();
    // Raw TIME projection in millimiters
    myHistoManager.get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->Write();
    // Clustered TIME projection
    myHistoManager.get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->Write();
    // Clustered TIME projection in millimiters
    myHistoManager.get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->Write();

    // debug plots per ASAD for cobo_id=0
    for (int cobo_id = 0;
         cobo_id <
         myEventSource->getCurrentEvent()->GetGeoPtr()->GetCoboNboards();
         cobo_id++) {
      for (int asad_id = 0;
           asad_id <
           myEventSource->getCurrentEvent()->GetGeoPtr()->GetAsadNboards(
               cobo_id);
           asad_id++) {
        auto h = myHistoManager.getChannels(cobo_id, asad_id);
        if (h)
          h->Write();
      }
    }

  } else {
    std::cout << "Wrong file entry number:" << FileEntry << std::endl;
    return -1;
  }

  aFile.Close();

  return 0;
}
#else

#include "colorText.h"
#include <iostream>

int main(){
  std::cout<<KRED<<"TPCReco was compiled without GET libraries."<<RST
	    <<" This application requires GET libraries."<<std::endl;
  return -1;
}
#endif
