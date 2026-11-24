#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#include <TROOT.h>

#include <boost/property_tree/ptree.hpp>

#include "TPCReco/ConfigManager.h"
#include "TPCReco/DirectoryWatch.h"
#include "TPCReco/WebGUIController.h"
#include "TPCReco/WebGUIServer.h"

namespace fs = std::filesystem;

namespace {
  /// 128 random bits as 32 hexadecimal characters
  std::string makeAccessToken(){
    std::random_device device;
    std::stringstream aStream;
    for(int i=0;i<4;++i) aStream<<std::hex<<std::setw(8)<<std::setfill('0')<<device();
    return aStream.str();
  }
}

/// Web based GUI of TPCReco: same configuration and functionality as tpcGUI,
/// displayed in a web browser. Usage:
///   tpcWebGUI --meta.configJson config.json [--web.port 8080] [--web.bindAddress 0.0.0.0]
/// Remote connections (any address other than 127.0.0.1) require an access token:
/// web.accessToken, or a random one generated at start-up and printed in the URL.
int main(int argc, char** argv) {

  ConfigManager cm;
  boost::property_tree::ptree config = cm.getConfig(argc, argv);
  if(cm.isHelpMode()) return 0;

  ROOT::EnableThreadSafety();
  gROOT->SetBatch(true);

  std::string documentRoot = config.get<std::string>("web.documentRoot", "");
  if(documentRoot.empty()){
    // installed layout: <prefix>/bin/tpcWebGUI and <prefix>/web
    documentRoot = (fs::canonical("/proc/self/exe").parent_path().parent_path()/"web").string();
  }

  std::string bindAddress = config.get<std::string>("web.bindAddress", "0.0.0.0");
  std::string accessToken = config.get<std::string>("web.accessToken", "");
  bool isLocalOnly = bindAddress=="127.0.0.1" || bindAddress=="localhost" || bindAddress=="::1";
  if(accessToken.empty() && !isLocalOnly) accessToken = makeAccessToken();

  WebGUIController controller(config);
  WebGUIServer server(controller, documentRoot, config.get<std::string>("input.resourcesPath", ""),
                      accessToken, config.get<std::string>("web.urlPrefix", ""));

  DirectoryWatch directoryWatch;
  if(controller.isOnline()){
    directoryWatch.setUpdateInterval(controller.getOnlineUpdateInterval());
    directoryWatch.setCallback([&server](const std::string & fileList){ server.loadOnlineFiles(fileList); });
    // DirectoryWatch::watch() never returns; the thread ends with the process
    std::thread(&DirectoryWatch::watch, &directoryWatch, controller.getOnlineDirectory()).detach();
  }

  server.run(bindAddress, config.get<int>("web.port", 8080));
  controller.close();
  std::cout << "tpcWebGUI: finished." << std::endl;
  return 0;
}
