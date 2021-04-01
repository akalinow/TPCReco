#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <sys/inotify.h>
#include <sys/types.h>

#include "colorText.h"
#include "DirectoryWatch.h"

////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
DirectoryWatch::DirectoryWatch(){

  updateIterval = 3000;
  
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DirectoryWatch::setUpdateInterval(int aInterval){
  if(aInterval<1000){
    std::cout<<KRED<<"Time interval: "<<aInterval<<" too low. "
	     <<RST<<"Setting to 1000 ms."<<std::endl;
    aInterval = 1000;
  }
  updateIterval = aInterval;
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DirectoryWatch::watch(const std::string & dirName){

  std::string fName, fullPath;
  int MAX_EVENTS = 32; /*Max. number of events to process at one go*/
  int LEN_NAME = 32; /*Assuming that the length of the filename won't exceed 16 bytes*/
  int EVENT_SIZE = ( sizeof (struct inotify_event) ); /*size of one event*/
  int BUF_LEN =    ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )); /*buffer to store the data of events*/  
  uint32_t inotifyEventMask = IN_MODIFY;
  
  int fileDescriptor = inotify_init();
  if(fileDescriptor < 0) {
    std::cerr<<"Couldn't initialize inotify"<<std::endl;
  }
  int wd = inotify_add_watch(fileDescriptor, dirName.c_str(), inotifyEventMask);
  if(wd == -1) std::cerr<<"Couldn't add watch to "<<dirName<<std::endl;

  char buffer[BUF_LEN];

  while(true){
    int nbytesRead = read(fileDescriptor, buffer, BUF_LEN);
    if(nbytesRead<0) std::cerr<<"Problem reading the inotify state"<<std::endl;
    int eventIndex = 0;
    while(eventIndex < nbytesRead) {
      struct inotify_event *event = ( struct inotify_event * ) &buffer[eventIndex];
      if(event->len && !(event->mask & IN_ISDIR)){
	fName = std::string(event->name);
	if(fName.find(".graw")!=std::string::npos){
	  std::this_thread::sleep_for(std::chrono::milliseconds(updateIterval));
	  fullPath = dirName+"/"+fName;
	  Message(fullPath.c_str());
	}      
      }
      eventIndex += EVENT_SIZE + event->len;
    }
  }
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////



