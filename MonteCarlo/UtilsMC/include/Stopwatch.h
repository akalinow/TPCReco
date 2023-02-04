#ifndef TPCSOFT_STOPWATCH_H
#define TPCSOFT_STOPWATCH_H

#include <sys/times.h>
#include <Units.h>


namespace utl {

    /**
       \class Stopwatch Stopwatch.h "utl/Stopwatch.h"

       \author  D. Veberic
       \date    13 May 2005
       \version $Id$
    */

    class Stopwatch {

    public:
        enum ECPUTime {
            eTotal,
            eUser,
            eSystem
        };

        Stopwatch();

        void Reset();
        void Start();
        void Stop();
        double GetCPUTime(const ECPUTime kind = eTotal);

    private:
        static const long fgClockTicks;
        bool fIsStopped;
        struct tms fStart;
        clock_t fUserTimeSum;
        clock_t fSystemTimeSum;

    };

}

#endif //TPCSOFT_STOPWATCH_H
