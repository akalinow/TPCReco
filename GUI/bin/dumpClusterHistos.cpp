
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

    // Raw STRIP VS TIME plots per STRIP direction:
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getRawStripVsTime(strip_dir);
      if (h)
        h->Write();
    }

    // Raw STRIP VS TIME plots in milimiters per STRIP direction:
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getRawStripVsTimeInMM(strip_dir);
      if (h)
        h->Write();
    }

    // Clustered STRIP VS TIME plots per STRIP direction:
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getClusterStripVsTime(strip_dir);
      if (h)
        h->Write();
    }

    // Clustered STRIP VS TIME plots in milimiters per STRIP direction:
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getClusterStripVsTimeInMM(strip_dir);
      if (h)
        h->Write();
    }

    // Raw TIME projection
    {
      auto h = myHistoManager.getRawTimeProjection();
      if (h)
        h->Write();
    }

    // Raw TIME projection in millimiters
    {
      auto h = myHistoManager.getRawTimeProjectionInMM();
      if (h)
        h->Write();
    }

    // Raw STRIP projection per STRIP direction
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getRawStripProjection(strip_dir);
      if (h)
        h->Write();
    }

    // Raw STRIP projection in millimiters per STRIP direction
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getRawStripProjectionInMM(strip_dir);
      if (h)
        h->Write();
    }

    // Clustered TIME projection
    {
      auto h = myHistoManager.getClusterTimeProjection();
      if (h)
        h->Write();
    }

    // Clustered TIME projection in millimiters
    {
      auto h = myHistoManager.getClusterTimeProjectionInMM();
      if (h)
        h->Write();
    }

    // Clustered STRIP projection per STRIP direction
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getClusterStripProjection(strip_dir);
      if (h)
        h->Write();
    }

    // Clustered STRIP projection in millimiters per STRIP direction
    for (int strip_dir = 0; strip_dir < 3; ++strip_dir) {
      auto h = myHistoManager.getClusterStripProjectionInMM(strip_dir);
      if (h)
        h->Write();
    }

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
