#ifndef _EventRaw_H_
#define _EventRaw_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <iostream>
#include "MultiKey.h"

#include "EventInfo.h"

namespace eventraw {

  class ChannelRaw {
  public:
    
  ChannelRaw() : cellMask(std::vector<uint8_t>(64, 0u)) { };
    
    std::vector<uint8_t>  cellMask; // 64-byte word, only first 512 bits are valid
    std::vector<uint16_t> cellData; // up to 512 elements
    
    friend std::ostream& operator<<(std::ostream& os, const eventraw::ChannelRaw& craw);
  };
  
  class AgetRaw {
  public:
    
  AgetRaw() : channelMask(std::vector<uint8_t>(9, 0u)) { };
    
    std::vector<uint8_t>    channelMask; // 9-byte word, only first 68 bits are valid
    std::vector<ChannelRaw> channelData; // up to 68 elements

    friend std::ostream& operator<<(std::ostream& os, const eventraw::AgetRaw& araw);
  };
  
  typedef std::map< MultiKey3_uint8, AgetRaw, multikey3_uint8_less > AgetRawMap_t; // index = { cobo[>=0], asad[0-3], aget[0-3] }
 
  class EventData {
  public:
    EventData(){};

    ~EventData(){};
    
    inline EventData(std::shared_ptr<EventData> &edata) : data(edata->data) { };
    inline EventData(std::shared_ptr<AgetRawMap_t> &map) : data(*map) { };
    
    AgetRawMap_t data;
    
    friend std::ostream& operator<<(std::ostream& os, const eventraw::EventData& edata);
  };
  
  class EventRaw : public EventInfo, public EventData {
  public:
    EventRaw(){};

    ~EventRaw(){};
    
    inline EventRaw(std::shared_ptr<EventInfo> &einfo, std::shared_ptr<EventData> &edata):EventInfo(einfo) {
      data = edata->data;
    };

    friend std::ostream& operator<<(std::ostream& os, const eventraw::EventRaw& eraw);
  };

  std::ostream& operator<<(std::ostream& os, const eventraw::AgetRaw& araw);
  std::ostream& operator<<(std::ostream& os, const eventraw::ChannelRaw& craw);
  std::ostream& operator<<(std::ostream& os, const eventraw::EventData& edata);  
  std::ostream& operator<<(std::ostream& os, const eventraw::EventRaw& eraw);

}
#endif

