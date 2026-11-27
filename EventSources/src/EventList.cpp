#include <fstream>
#include <sstream>
#include <stdexcept>

#include "TPCReco/EventList.h"
#include "TPCReco/GrawFile.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<ListedEvent> readEventList(const std::string & fileName){

  std::ifstream in(fileName);
  if(!in) throw std::runtime_error("input.eventList: cannot open "+fileName);
  std::vector<ListedEvent> events;
  std::string line;
  unsigned long lineNumber = 0;
  while(std::getline(in, line)){
    ++lineNumber;
    std::string fieldsLine = line;
    for(auto & c : fieldsLine) if(c==',' || c=='\t' || c==';') c=' ';
    std::istringstream fields(fieldsLine);
    std::vector<std::string> items;
    std::string item;
    while(fields>>item) items.push_back(item);
    if(items.empty() || items.front()[0]=='#') continue;
    if(items.size()>3){
      throw std::runtime_error("input.eventList: more than 3 fields in line "+std::to_string(lineNumber)
                               +" of "+fileName+": "+line);
    }
    ListedEvent aEvent;
    try {
      std::size_t used = 0;
      auto number = [&used](const std::string & text){
        const long value = std::stol(text, &used);
        if(used!=text.size() || value<0) throw std::invalid_argument(text);
        return value;
      };
      aEvent.eventId = static_cast<unsigned long>(number(items.back()));
      if(items.size()>1) aEvent.runId = number(items.front());
      if(items.size()==3) aEvent.entry = number(items[1]);
    }
    catch(const std::exception &){
      throw std::runtime_error("input.eventList: cannot parse line "+std::to_string(lineNumber)
                               +" of "+fileName+": "+line);
    }
    events.push_back(aEvent);
  }
  return events;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::size_t grawFramesPerChunk(const std::string & grawFile){

  graw::GrawFile aFile;
  if(!aFile.open(grawFile)) return 0;
  return aFile.nFrames();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int listedEventChunk(const ListedEvent & aEvent, std::size_t framesPerChunk){

  if(!aEvent.hasEntry() || !framesPerChunk) return -1;
  if(aEvent.eventId<static_cast<unsigned long>(aEvent.entry)) return -1;
  return static_cast<int>((aEvent.eventId - aEvent.entry)/framesPerChunk);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
