#ifndef _RawSignal_tree_dataFormat_H_
#define _RawSignal_tree_dataFormat_H_

struct Event_rawsignal{ // diagnostic data per event
  long runId;
  unsigned int eventId;
  double unixTimeSec; // [s] absolute unix timestamp since 1970/1/1 epoch with millisecond precision (combined runID and GET counter)
  double runTimeSec; // [s] elapsed time since start of the runID with 10ns precision from GET counter (100 MHz clock)
  double deltaTimeSec; // [s] diference with previous event from GET counter converted to seconds (prone to glitches while switching between runIDs)
  bool clusterFlag; // is clustering enabled?
  float clusterThr; // clustering threshold in ADC units
  int clusterDeltaStrips; // cluster size in +/- strip units 
  int clusterDeltaTimeCells; // cluster size in +/- time cell units
  long nHits; // # of cluster/event hits in (electronic channel) x (time cell) domain
  float totalCharge; // total charge integral from all strips for cluster/event
  float totalChargePerDir[3]; // charge integral per strip direction for cluster/event
  float maxCharge; // maximal pulse-height from all strips for cluster/event
  float maxChargePerDir[3]; // maximal pulse-height per strip direction for cluster/event
  float maxChargePositionPerDir[3]; // [mm] postion of the maximal pulse-height per strip direction for cluster/event
  
};

#endif
