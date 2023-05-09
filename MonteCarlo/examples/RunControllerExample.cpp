//#include "RunController.h"
#include <boost/property_tree/json_parser.hpp>
#include "TRandom.h"
#include "TPCReco/RunController.h"
//Interesting bug that I (PP) was not able to solve:
//Without including at least one .h file with the usage of REGISTER_MODULE macro the registration does not work,
//so this dummy module is sort of a workaround this issue. The problem seems to be happening due to modules and factory being in separate libraries...
#include "../Modules/DummyModule/DummyModule.h"

namespace pt = boost::property_tree;

int main(int argc, char **argv) {

    if(argc<2){
        std::cout<<"Usage: "<<argv[0]<<" <path to config file> <random seed (optional)>"<<std::endl;
        return -1;
    }
    ULong64_t seed = 0;
    if(argc>2){
        seed = std::stoi(argv[2]);
    }
    gRandom->SetSeed(seed);
    fwk::RunController rc;
    pt::ptree topNode;
    pt::read_json(argv[1], topNode);
    rc.Init(topNode);
    rc.RunFull();
    rc.Finish();
}