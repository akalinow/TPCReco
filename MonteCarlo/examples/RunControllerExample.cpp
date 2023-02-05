//#include "RunController.h"
#include <boost/property_tree/json_parser.hpp>

#include "RunController.h"
//Interesting bug that I (PP) was not able to solve:
//Without including at least one .h file with the usage of REGISTER_MODULE macro the registration does not work,
//so this dummy module is sort of a workaround this issue. The problem seems to be happening due to modules and factory being in separate libraries...
#include "../Modules/DummyModule/DummyModule.h"

namespace pt = boost::property_tree;

int main(int argc, char **argv) {
    fwk::RunController rc;
    pt::ptree topNode;
    pt::read_json(argv[1], topNode);
    rc.Init(topNode);
    rc.RunFull();
    rc.Finish();
}