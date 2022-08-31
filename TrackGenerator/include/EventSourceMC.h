#ifndef _EventSourceMC_H_
#define _EventSourceMC_H_

#include <memory>
#include <boost/property_tree/json_parser.hpp>

#include "EventSourceBase.h"
#include "UVWprojector.h"

#include "TH1F.h"
#include "Track3D.h"

class EventSourceMC: public EventSourceBase {
  
public:

  EventSourceMC(){};

  EventSourceMC(const std::string & geometryFileName);

  ~EventSourceMC();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<EventTPC> getLastEvent();

  unsigned long int numberOfEvents() const;

  void loadGeometry(const std::string & fileName);
  
 private:

  const TVector3 & getVertex();
  const TrackSegment3D & getSegment(const TVector3 vertexPos, pid_type ion_id);  
  const TH1F & getChargeProfile(double ion_range, pid_type ion_id);  
  void createTrack();
  void fillChargeMap(const Track3D & aTrack);
  void generateEvent();

  TH1F myChargeProfile{"hChargeProfile","",1024, -0.2, 1.2};
  TVector3 myVertex;
  Track3D myTrack;
  TrackSegment3D mySegment;
};
#endif

