#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

class TH2D;

class GeometryTPC;
class EventTPC;

class HistoManager {
public:
  
  HistoManager();
  
  ~HistoManager();

  void setEvent(EventTPC* aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  std::shared_ptr<TH2D> getStripVsTime(int aDir);
   
private:
    
  EventTPC *myEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

