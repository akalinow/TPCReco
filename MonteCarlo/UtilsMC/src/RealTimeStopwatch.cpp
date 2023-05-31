#include "TPCReco/RealTimeStopwatch.h"


#include <sys/time.h>
#include "TPCReco/RealTimeStopwatch.h"

using namespace utl;


RealTimeStopwatch::RealTimeStopwatch(const bool start) :
        fIsStopped(!start),
        fTimeSum(0)
{
    if (start)
        gettimeofday(&fStart, 0);
}


void
RealTimeStopwatch::Reset()
{
    fIsStopped = true;
    fTimeSum = 0;
}


void
RealTimeStopwatch::Start()
{
    if (fIsStopped) {
        gettimeofday(&fStart, 0);
        fIsStopped = false;
    }
}


double
RealTimeStopwatch::Stop()
{
    if (fIsStopped)
        return fTimeSum;
    else {
        struct timeval stop;
        gettimeofday(&stop, 0);
        fIsStopped = true;
        return fTimeSum +=
                       (stop.tv_sec - fStart.tv_sec)*second +
                       (stop.tv_usec - fStart.tv_usec)*microsecond;
    }
}


double
RealTimeStopwatch::GetTime()
{
    if (fIsStopped)
        return fTimeSum;
    else {
        struct timeval stop;
        gettimeofday(&stop, 0);
        return fTimeSum + (stop.tv_sec - fStart.tv_sec)*second +
               (stop.tv_usec - fStart.tv_usec)*microsecond;
    }
}
