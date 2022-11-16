#ifndef __RUNCONDITIONS_H__
#define __RUNCONDITIONS_H__

#include <iostream>

class RunConditions {
  
 public:

  RunConditions(){};

  ~RunConditions(){};
  
  void setDriftVelocity(double v);

  void setSamplingRate(double r);

  void setTriggerDelay(double d);

  double getDriftVelocity() const noexcept;

  double getSamplingRate() const noexcept;

  double getTriggerDelay() const noexcept;

 private:

  double vdrift{0.0};                   // electron drift velocity in [cm / micosecond]
  double sampling_rate{0.0};            // electronics sampling rate in [MHz]
  double trigger_delay{0.0};            // delay in [microseconds] of the external "t0" trigger signal (for accelerator beam)
};

std::ostream & operator << (std::ostream &out, const RunConditions &aConditions);

#endif
