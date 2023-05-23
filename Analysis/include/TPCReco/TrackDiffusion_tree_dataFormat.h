#ifndef _TrackDiffusion_tree_dataFormat_H_
#define _TrackDiffusion_tree_dataFormat_H_

#include <TVector3.h>

struct Event_rawdiffusion{ // diffusion measurements per leading track of an event
  long runId{0};
  unsigned int eventId{0};
  bool clusterFlag{false}; // is clustering enabled?
  float clusterThr{0}; // clustering threshold in ADC units for seed hits
  int clusterDeltaStrips{0}; // clustering envelope size in +/- strip units aroud seed hits
  int clusterDeltaTimeCells{0}; // clustering envelope size in +/- time cell units around seed hits
  int ntracks{0}; // total number of tracks
  int pid{0}; // leading track PID
  TVector3 vertexPos{0,0,0}; // [mm] event vertex
  TVector3 endPos{0,0,0}; // [mm] leading track endpoint
  float length{0}; // [mm] leading track length
  float phiDET{0}; // [rad] leading track PHI_DET
  float cosThetaDET{0}; // leading track cos(THETA_DET)
  bool flagPerDir[3]{false, false, false}; // valid (mean,rms) pair exists per U/V/W strip direction
  float meanPerDir[3]={0,0,0}; // [mm] relative position (wrt track axis) of the mean of the distribution of projected hits from the region of interest per U/V/W direction
  float sigmaPerDir[3]={0,0,0}; // [mm] standard deviation of the distribution of projected hits from the region of interest per U/V/W direction
  bool flagAll{false}; // valid (mean,rms) pair exists
  float meanAll{0}; // [mm] relative position (wrt track axis) of the mean of the distribution of projected hits from the region of interest from all directions
  float sigmaAll{0}; // [mm] standard deviation of the distribution of projected hits from the region of interest from all directions
};

#endif
