#include "TPCReco/CoBoClock.h"
namespace tpcreco {
boost::posix_time::ptime CoBoClock::epoch =
    boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));
} // namespace tpcreco
