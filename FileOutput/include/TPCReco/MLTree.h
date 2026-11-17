#ifndef _MLTree_H_
#define _MLTree_H_

// Flat TTree for machine learning: one entry per event, plain numeric
// branches only, so that it can be read by uproot without custom models.
//
//  runId, eventId, timestamp          event key
//  hitDir[], hitStrip[], hitTime[],   hits passing the hit filter
//  hitCharge[]                        (output.mlHitFilter), strip sections merged
//  sim_*                              generated track segments (MC only, else empty)
//  reco_*                             reconstructed track segments
//
// Segment branches: *_pid[], *_startX/Y/Z[], *_endX/Y/Z[] in [mm],
// one array element per segment.

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <Rtypes.h>
#include <boost/property_tree/ptree.hpp>

#include "TPCReco/CommonDefinitions.h"

class TTree;
class EventTPC;
class Track3D;

class MLTree {
public:
  explicit MLTree(const boost::property_tree::ptree &aConfig);

  // Create the branches in a given tree.
  void book(TTree *aTree);

  // Set branch buffers from the current event. aSimTrack can be nullptr (real data).
  void set(EventTPC &aEvent, const Track3D &aRecoTrack, const Track3D *aSimTrack);

private:
  struct Segments {
    std::vector<Int_t> pid;
    std::vector<Float_t> startX, startY, startZ, endX, endY, endZ;
    void clear();
    void set(const Track3D &aTrack);
    void book(TTree *aTree, const std::string &prefix);
  };

  boost::property_tree::ptree myConfig;
  filter_type myHitFilter{filter_type::threshold};

  Long64_t runId{0};
  UInt_t eventId{0};
  ULong64_t timestamp{0};

  std::vector<UChar_t> hitDir;
  std::vector<UShort_t> hitStrip, hitTime;
  std::vector<Float_t> hitCharge;

  Segments sim, reco;
  Float_t reco_length{0}, reco_loss{0}, reco_hypothesisLoss{0};

  // charge summed over strip sections: key = (dir, strip, time cell)
  std::map<std::tuple<int, int, int>, double> mergedHits;
};
#endif
