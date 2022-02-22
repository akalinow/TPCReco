#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/inotify.h>
#include <sys/types.h>
#include <set>

#include "colorText.h"
#include "DirectoryWatch.h"

////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
DirectoryWatch::DirectoryWatch(){

  updateInterval = 3000;
  
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DirectoryWatch::setUpdateInterval(int aInterval){
  if(aInterval<1000){
    std::cout<<KRED<<"Time interval: "<<aInterval<<" too low. "
	     <<RST<<"Setting to 1000 ms."<<std::endl;
    aInterval = 1000;
  }
  updateInterval = aInterval;
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DirectoryWatch::watch(const std::string & dirName){

  std::string fName, fullPath;
  int MAX_EVENTS = (4L/* MAX expected number of GRAW streams */
		    *100L/* MAX expected rate in Hz */
		    *updateInterval/1000L/* interval in seconds */
		    ); //  32; /*Max. number of events to process at one go*/
  int LEN_NAME = 32; /*Assuming that the length of the filename won't exceed 16 bytes*/
  int EVENT_SIZE = ( sizeof (struct inotify_event) ); /*size of one event*/
  int BUF_LEN =    ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )); /*buffer to store the data of events*/  
  //#ifdef DEBUG
  //  uint32_t inotifyEventMask = IN_MODIFY | IN_ATTRIB; // MC : for test purpose ONLY with "touch" command line !!!!
  //#else
  uint32_t inotifyEventMask = IN_MODIFY;
  //#endif
  
  int fileDescriptor = inotify_init();
  if(fileDescriptor < 0) {
    std::cerr<<"Couldn't initialize inotify"<<std::endl;
  }
  int wd = inotify_add_watch(fileDescriptor, dirName.c_str(), inotifyEventMask);
  if(wd == -1) std::cerr<<"Couldn't add watch to "<<dirName<<std::endl;

  char buffer[BUF_LEN];

#ifdef DIRECTORYWATCH_ONE_MESSAGE_DISABLE
  //
  // begin of separate Message() calls for individual files
  //
  while(true){
    int nbytesRead = read(fileDescriptor, buffer, BUF_LEN);
    if(nbytesRead<0) std::cerr<<"Problem reading the inotify state"<<std::endl;
    int eventIndex = 0;
    while(eventIndex < nbytesRead) {
      struct inotify_event *event = ( struct inotify_event * ) &buffer[eventIndex];
      if(event->len && !(event->mask & IN_ISDIR)){
	  fName = std::string(event->name);
	  if(fName.substr(fName.find_last_of(".") + 1) == "graw"){
	    fullPath = dirName+"/"+fName;
#ifdef DEBUG
	    std::cout << __FUNCTION__ << ": Message=" << fullPath.c_str() << std::endl; 
#endif
	    Message(fullPath.c_str());
	  }
      }
      eventIndex += EVENT_SIZE + event->len;
    }
#ifdef DEBUG
    std::cout << __FUNCTION__ << ": Entering sleep for " << updateInterval << " msec." << std::endl;
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(updateInterval));	  
  }
  //
  // end of separate Message() calls for individual files
  //  
#else
  //
  // begin of single Message() for all files
  //
  while(true){
    int nbytesRead = read(fileDescriptor, buffer, BUF_LEN);
    if(nbytesRead<0) std::cerr<<"Problem reading the inotify state"<<std::endl;
    int eventIndex = 0;
    bool is_first=true;
    fullPath="";
    std::set<std::string> uniqueFnameList{};
    while(eventIndex < nbytesRead) {
      struct inotify_event *event = ( struct inotify_event * ) &buffer[eventIndex];
      if(event->len && !(event->mask & IN_ISDIR)){
	  fName = std::string(event->name);
	  if(fName.substr(fName.find_last_of(".") + 1) == "graw" &&
	     uniqueFnameList.find(fName)==uniqueFnameList.end()){
	    if(is_first) is_first=false; else fullPath += ","; // comma separated list of file names
	    fullPath += dirName+"/"+fName;
	    uniqueFnameList.insert(fName);
	  }
      }
      eventIndex += EVENT_SIZE + event->len;
    }
    if(fullPath.size()) {
#ifdef DEBUG
      std::cout << __FUNCTION__ << ": Message=" << fullPath.c_str() << std::endl; 
#endif
      Message(fullPath.c_str());
    }
#ifdef DEBUG
    std::cout << __FUNCTION__ << ": Entering sleep for " << updateInterval << " msec." << std::endl;
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(updateInterval));	  
  }
  //
  // end of single Message() for all files
  //
#endif
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////



