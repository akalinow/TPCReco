#ifndef _EventSourceMC_H_
#define _EventSourceMC_H_

#include <memory>
#include <boost/property_tree/json_parser.hpp>

#include "TH1F.h"
#include "TRandom3.h"

#include "EventSourceBase.h"
#include "UVWprojector.h"
#include "Track3D.h"
#include "IonRangeCalculator.h"

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

  mutable TRandom3 myRndm;
  std::shared_ptr<UVWprojector> myProjectorPtr;
  TH3D my3DChargeCloud;

  TVector3 createVertex() const;
  TrackSegment3D createSegment(const TVector3 vertexPos, pid_type ion_id) const;  
  TH1F createChargeProfile(double ion_range, pid_type ion_id) const;  
  Track3D createTrack() const;

  
  void fill3DChargeCloud(const Track3D & aTrack);
  void fillPEventTPC(const Track3D & aTrack);
  void generateEvent();

  
  
  
  
};
#endif

