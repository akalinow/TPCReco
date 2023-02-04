#include <sys/times.h>
#include <unistd.h>
#include "Stopwatch.h"

using namespace utl;


const long Stopwatch::fgClockTicks = sysconf(_SC_CLK_TCK);


Stopwatch::Stopwatch() :
        fIsStopped(false),
        fUserTimeSum(0),
        fSystemTimeSum(0)
{
    times(&fStart);
}


void
Stopwatch::Reset()
{
    fIsStopped = true;
    fUserTimeSum = fSystemTimeSum = 0;
}


void
Stopwatch::Start()
{
    if (fIsStopped) {
        times(&fStart);
        fIsStopped = false;
    }
}


void
Stopwatch::Stop()
{
    if (!fIsStopped) {
        struct tms stop;
        times(&stop);
        fUserTimeSum += stop.tms_utime - fStart.tms_utime;
        fSystemTimeSum += stop.tms_stime - fStart.tms_stime;
        fIsStopped = true;
    }
}


double
Stopwatch::GetCPUTime(const ECPUTime kind)
{
    if (fIsStopped)
        switch (kind) {
            case eTotal:
                return double(fUserTimeSum + fSystemTimeSum)/fgClockTicks*second;
            case eUser:
                return double(fUserTimeSum)/fgClockTicks*second;
            case eSystem:
                return double(fSystemTimeSum)/fgClockTicks*second;
        }
    else {
        struct tms stop;
        times(&stop);
        switch (kind) {
            case eTotal:
                return double(fUserTimeSum + fSystemTimeSum +
                              stop.tms_utime - fStart.tms_utime +
                              stop.tms_stime - fStart.tms_stime)/fgClockTicks*second;
            case eUser:
                return double(fUserTimeSum +
                              stop.tms_utime - fStart.tms_utime)/fgClockTicks*second;
            case eSystem:
                return double(fSystemTimeSum +
                              stop.tms_stime - fStart.tms_stime)/fgClockTicks*second;
        }
    }

    return 0;
}
