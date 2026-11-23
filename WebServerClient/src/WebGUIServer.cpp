#include <chrono>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <unistd.h>

#include <crow.h>

#include "TPCReco/WebGUIController.h"
#include "TPCReco/WebGUIServer.h"

namespace fs = std::filesystem;

struct WebGUIServer::Impl {

  Impl(WebGUIController & aController, const std::string & documentRoot,
       const std::string & resourcesPath, const std::string & accessToken):
    controller(aController), docRoot(documentRoot), resources(resourcesPath), token(accessToken) {}

  WebGUIController & controller;
  fs::path docRoot, resources;
  std::string token;
  crow::SimpleApp app;

  std::mutex controllerMutex;
  std::mutex connectionsMutex;
  std::set<crow::websocket::connection*> connections;

  void setupRoutes();
  bool hasTokenParameter(const crow::request & req) const;
  bool isAuthorized(const crow::request & req) const;
  void notifyClients();
  crow::response serveFile(const fs::path & dir, const std::string & name) const;
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
WebGUIServer::WebGUIServer(WebGUIController & aController, const std::string & documentRoot,
                           const std::string & resourcesPath, const std::string & accessToken):
  myImpl(std::make_unique<Impl>(aController, documentRoot, resourcesPath, accessToken)) {

  myImpl->setupRoutes();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
WebGUIServer::~WebGUIServer() = default;
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIServer::run(const std::string & address, int port){

  if(!fs::is_regular_file(myImpl->docRoot/"index.html")){
    std::cerr<<"WebGUIServer: no index.html in the web client directory "
             <<myImpl->docRoot<<". Set web.documentRoot."<<std::endl;
    return;
  }
  std::string host = address;
  if(address=="0.0.0.0" || address=="::"){ // all interfaces: show the machine name
    char hostName[256] = {0};
    host = gethostname(hostName, sizeof(hostName)-1)==0 && hostName[0] ? hostName : "localhost";
  }
  std::string url = "http://"+host+":"+std::to_string(port)+"/";
  if(myImpl->token.size()) url += "?token="+myImpl->token;
  std::cout<<"tpcWebGUI: open "<<url<<" in a web browser."<<std::endl;
  if(myImpl->token.size()){
    std::cout<<"tpcWebGUI: the URL contains the access token: share it only with people allowed to use the GUI."<<std::endl;
  }
  myImpl->app.bindaddr(address).port(port).concurrency(4).loglevel(crow::LogLevel::Warning).run();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIServer::loadOnlineFiles(const std::string & fileList){

  {
    std::lock_guard<std::mutex> lock(myImpl->controllerMutex);
    myImpl->controller.loadOnlineFiles(fileList);
  }
  myImpl->notifyClients();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIServer::Impl::notifyClients(){

  std::lock_guard<std::mutex> lock(connectionsMutex);
  for(auto aConnection: connections) aConnection->send_text("update");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIServer::Impl::hasTokenParameter(const crow::request & req) const {

  const char *value = req.url_params.get("token");
  return value && token==value;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIServer::Impl::isAuthorized(const crow::request & req) const {

  if(token.empty() || hasTokenParameter(req)) return true;
  const std::string & cookies = req.get_header_value("Cookie");
  const std::string name = "tpcwebgui_token=";
  for(size_t pos = cookies.find(name); pos!=std::string::npos; pos = cookies.find(name, pos+1)){
    if(pos>0 && cookies[pos-1]!=' ' && cookies[pos-1]!=';') continue;
    size_t start = pos+name.size();
    size_t end = cookies.find(';', start);
    if(cookies.substr(start, end==std::string::npos ? std::string::npos : end-start)==token) return true;
  }
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::response WebGUIServer::Impl::serveFile(const fs::path & dir, const std::string & name) const {

  // only plain file names below dir
  if(name.empty() || name.find("..")!=std::string::npos || name.find('/')!=std::string::npos){
    return crow::response(404);
  }
  fs::path aPath = dir/name;
  if(!fs::is_regular_file(aPath)) return crow::response(404);
  crow::response aResponse;
  aResponse.set_static_file_info_unsafe(aPath.string());
  return aResponse;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIServer::Impl::setupRoutes(){

  const auto denied = [](){
    return crow::response(401, "Access token required: open the URL printed by tpcWebGUI at start-up.");
  };

  CROW_ROUTE(app, "/")([this, denied](const crow::request & req){
    if(!isAuthorized(req)) return denied();
    crow::response aResponse = serveFile(docRoot, "index.html");
    if(token.size() && hasTokenParameter(req)){
      // the browser sends the token with all further requests, including the WebSocket
      aResponse.add_header("Set-Cookie", "tpcwebgui_token="+token+"; Path=/; HttpOnly; SameSite=Strict");
    }
    aResponse.add_header("Cache-Control", "no-store");
    return aResponse;
  });

  CROW_ROUTE(app, "/static/<string>")([this, denied](const crow::request & req, const std::string & name){
    if(!isAuthorized(req)) return denied();
    return serveFile(docRoot, name);
  });

  CROW_ROUTE(app, "/logo/<string>")([this, denied](const crow::request & req, const std::string & name){
    if(!isAuthorized(req)) return denied();
    if(name!="FUW_znak.png" && name!="ELITEPC_znak.png") return crow::response(404);
    return serveFile(resources, name);
  });

  CROW_ROUTE(app, "/api/status")([this, denied](const crow::request & req){
    if(!isAuthorized(req)) return denied();
    std::lock_guard<std::mutex> lock(controllerMutex);
    return crow::response(controller.getStatus());
  });

  CROW_ROUTE(app, "/api/view")([this, denied](const crow::request & req){
    if(!isAuthorized(req)) return denied();
    std::lock_guard<std::mutex> lock(controllerMutex);
    return crow::response(controller.getView());
  });

  CROW_ROUTE(app, "/api/ls")([this, denied](const crow::request & req){
    if(!isAuthorized(req)) return denied();
    const char *dir = req.url_params.get("dir");
    std::lock_guard<std::mutex> lock(controllerMutex);
    return crow::response(controller.listDirectory(dir ? dir : ""));
  });

  CROW_ROUTE(app, "/api/command").methods(crow::HTTPMethod::Post)([this, denied](const crow::request & req){
    if(!isAuthorized(req)) return denied();
    crow::json::wvalue reply;
    auto command = crow::json::load(req.body);
    if(!command || !command.has("cmd")){
      reply["ok"] = false;
      reply["error"] = "Malformed command.";
      return crow::response(400, reply);
    }
    if(std::string(command["cmd"].s())=="exit"){
      reply["ok"] = true;
      // stop after the reply has been sent
      std::thread([this](){
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        app.stop();
      }).detach();
      return crow::response(reply);
    }
    std::string error;
    {
      std::lock_guard<std::mutex> lock(controllerMutex);
      try {
        error = controller.execute(command);
      }
      catch(const std::exception & e){
        error = std::string("Command failed: ")+e.what();
      }
      reply["ok"] = error.empty();
      reply["error"] = error;
      reply["view"] = controller.getView();
    }
    if(error.empty()) notifyClients(); // keep other browsers in sync
    return crow::response(reply);
  });

  CROW_WEBSOCKET_ROUTE(app, "/ws")
    .onaccept([this](const crow::request & req, void **){ return isAuthorized(req); })
    .onopen([this](crow::websocket::connection & conn){
      std::lock_guard<std::mutex> lock(connectionsMutex);
      connections.insert(&conn);
    })
    .onclose([this](crow::websocket::connection & conn, const std::string &, uint16_t){
      std::lock_guard<std::mutex> lock(connectionsMutex);
      connections.erase(&conn);
    })
    .onmessage([](crow::websocket::connection &, const std::string &, bool){});
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
