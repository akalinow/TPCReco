#ifndef TPCSOFT_REALTIMESTOPWATCH_H
#define TPCSOFT_REALTIMESTOPWATCH_H

#include <sys/time.h>
#include "Units.h"


namespace utl {

    /**
       \class RealTimeStopwatch RealTimeStopwatch.h "utl/RealTimeStopwatch.h"

       Note: constructor starts the stopwatch. Use Reset() before starting it
       at the later time.

       \author  D. Veberic
       \date    16 May 2007
       \version $Id$
    */

    class RealTimeStopwatch {

    public:
        RealTimeStopwatch(const bool start = true);

        void Reset();
        void Start();
        /// returns time since last call to Start()
        double Stop();
        double GetTime();

    private:
        bool fIsStopped;
        struct timeval fStart;
        double fTimeSum;

    };

}

#endif //TPCSOFT_REALTIMESTOPWATCH_H
