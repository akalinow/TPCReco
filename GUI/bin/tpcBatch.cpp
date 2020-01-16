#include <iostream>

#include "DataManager.h"
#include "SigClusterTPC.h"
#include "TrackBuilder.h"

int main() {

    //TEST ---
    std::string pathFileName = "paths.txt";
    std::string geometryFileName;
    std::string dataFileName;
    std::fstream path_file;
    path_file.open(pathFileName, std::ios::in);
    path_file >> geometryFileName;
    path_file >> dataFileName;
    std::cout << geometryFileName << std::endl;
    std::cout << dataFileName << std::endl;
    path_file.close();
    //dataFileName = "*poza buildem";

    DataManager aDataManager;
    TrackBuilder aTkBuilder;

    aDataManager.loadGeometry(geometryFileName);
    checkpoint;
    aDataManager.loadDataFile(dataFileName);
    aDataManager.loadTreeEntry(3);

    auto aEvent = aDataManager.getCurrentEvent();
    if (aEvent == nullptr) {
        std::cout << __FUNCTION__ << " NULL EventTPC pointer" << std::endl;
        return -1;
    }

    aTkBuilder.setEvent(aEvent);
    aTkBuilder.reconstruct();

    return 0;
}