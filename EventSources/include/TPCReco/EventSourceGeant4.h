#ifndef _EventSourceGeant4_H_
#define _EventSourceGeant4_H_

#include <memory>
#include <boost/property_tree/json_parser.hpp>

#include <TH1F.h>
#include <TRandom3.h>

#include "TPCReco/EventSourceBase.h"
#include "TPCReco/UVWprojector.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/EventGenerator.h"

class EventSourceGeant4: public EventSourceBase {
  
public:

  EventSourceGeant4(){};

  EventSourceGeant4(const std::string & geometryFileName, std::shared_ptr<EventGenerator> eventGenerator);

  ~EventSourceGeant4();

  void loadFileEntry(unsigned long int iEntry);

  void loadEventId(unsigned long int iEvent);

  void loadDataFile(const std::string & fileName);

  std::shared_ptr<EventTPC> getNextEvent();

  std::shared_ptr<EventTPC> getPreviousEvent();

  std::shared_ptr<EventTPC> getLastEvent();

  const Track3D & getGeneratedTrack(unsigned int index=0) const {return myTracks3D.at(index);}

  pid_type getGeneratedEventType() const {return myGenEventType;}

  unsigned long int numberOfEvents() const;

  void loadGeometry(const std::string & fileName);
  
 private:

  TGraph* braggGraph_alpha, *braggGraph_12C;
  double braggGraph_alpha_energy, braggGraph_12C_energy;
  double keVToChargeScale{1.0};

  std::shared_ptr<EventGenerator> myEventGenerator;
  mutable TRandom3 myRndm{0};
  std::shared_ptr<UVWprojector> myProjectorPtr;
  mutable IonRangeCalculator myRangeCalculator;
  TH3D my3DChargeCloud;
  std::vector<Track3D> myTracks3D;
  pid_type myGenEventType;

  TVector3 createVertex() const;
  TrackSegment3D createSegment(const TVector3 vertexPos, pid_type ion_id, double energy) const;  
  TH1F createChargeProfile(double ion_range, pid_type ion_id) const;  
  Track3D createTrack(const TVector3 & aVtx, pid_type ion_id, double energy) const;

  void generateSingleProng(pid_type ion_id=pid_type::ALPHA);
  void generateTwoProng();
  void generateThreeProng();

  void fill3DChargeCloud(const Track3D & aTrack);
  void fillPEventTPC(const TH3D & h3DChargeCloud, const Track3D & aTrack);
  void generateEvent();

  
  
  
  
};
#endif

