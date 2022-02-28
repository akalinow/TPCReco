//#include <cstdint>
//#include <memory>
//#include <vector>
#include <map>
#include <iostream>
#include "EventRaw.h"
#include "MultiKey.h"

// overloading << operator
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::ChannelRaw& craw) {
  os << "ChannelRaw: cellMask.size=" << craw.cellMask.size()
     << ", cellData.size=" << craw.cellData.size();
  return os;
}
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::AgetRaw& araw) {
  os << "AgetRaw: channelMask.size=" << araw.channelMask.size()
     << ", channelData.size=" << araw.channelData.size();

  os << std::endl
     << "Channel: 0    5    0    5    0    5    0    5    0    5    0    5    0    5 7"
     << std::endl
     << "         0____0____1____1____2____2____3____3____4____4____5____5____6____6_6"
     << std::endl
     << "   Mask: ";

  unsigned int ch=0;
  for(auto it=araw.channelMask.begin(); it!=araw.channelMask.end(); it++) {
    const unsigned char bitmask=*it;
    for(auto bit=0; bit<8; bit++) {
      os << ( (bitmask>>bit)&1 ? "1" : "_" );
      ch++;
    }
  }
  return os;
}
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventInfo& einfo) {
  os << "EventInfo: id=" << einfo.eventId << ", timestamp=" << einfo.timestamp;
  return os;
}
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventData& edata) {
  os << "EventData: AgetRawMap.size=" << edata.data.size();
  for(auto it=edata.data.begin(); it!=edata.data.end(); it++) {
    os << std::endl
       << "[cobo=" << (unsigned int)(it->first).key1
       << ", asad=" << (unsigned int)(it->first).key2
       << ", aget=" << (unsigned int)(it->first).key3 << "]:" // << std::endl
       << it->second; // << std::endl << std::flush;
  }
  return os;
}
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventRaw& eraw) {
  const eventraw::EventInfo& einfo=eraw;
  const eventraw::EventData& edata=eraw;
  os << einfo << std::endl
     << edata;
  return os;
}


