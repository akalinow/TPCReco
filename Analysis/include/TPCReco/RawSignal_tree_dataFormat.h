#ifndef _RawSignal_tree_dataFormat_H_
#define _RawSignal_tree_dataFormat_H_

struct Event_rawsignal{ // diagnostic data per event
  long runId;
  unsigned int eventId;
  double unixTimeSec; // [s] absolute unix timestamp since 1970/1/1 epoch with millisecond precision (combined runID and GET counter)
  double runTimeSec; // [s] elapsed time since start of the runID with 10ns precision from GET counter (100 MHz clock)
  double deltaTimeSec; // [s] diference with previous event from GET counter converted to seconds (prone to glitches while switching between runIDs)
  bool clusterFlag; // is clustering enabled?
  float clusterThr; // clustering threshold in ADC units for seed hits
  int clusterDeltaStrips; // clustering envelope size in +/- strip units aroud seed hits
  int clusterDeltaTimeCells; // clustering envelope size in +/- time cell units around seed hits
  long nHits; // # of cluster/event hits in (electronic channel) x (time cell) domain
  long nHitsPerDir[3]; // # of cluster/event hits in (electronic channel) x (time cell) domain per strip direction
  float totalCharge; // total charge integral from all strips for cluster/event
  float totalChargePerDir[3]; // charge integral per strip direction for cluster/event
  float maxCharge; // maximal pulse-height from all strips for cluster/event
  float maxChargePerDir[3]; // maximal pulse-height per strip direction for cluster/event
  float maxChargePositionPerDir[3]; // [mm] postion of the maximal pulse-height per strip direction for cluster/event
  float horizontalWidthPerDir[3]; // [mm] horizontal (strip domain) cluster size per strip direction
  float verticalWidthPerDir[3]; // [mm] vertical (time domain) cluster size per strip direction
  float horizontalChargePerDirHalf[3][2]; // [mm] horizontal (strip domain) integrated charge fraction per strip direction per half-cluster
  float verticalChargePerDirHalf[3][2]; // [mm] vertical (time domain) integrated charge asymmetry per strip direction per half-cluster
  
};

#endif
