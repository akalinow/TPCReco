#include <iostream>

#include "DataManager.h"
#include "EventHits.h"
#include "TrackBuilder.h"
#include "HistoManager.h"

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
    Geometry(geometryFileName); //initialize GeometryTPC before first use
    DataManager aDataManager;
    TrackBuilder aTkBuilder;
    checkpoint;
    aDataManager.loadDataFile(dataFileName);
    aDataManager.loadTreeEntry(3);
    //aDataManager.loadEvent(dataFileName);
    auto aEvent = aDataManager.getCurrentEvent();
    if (aEvent == nullptr) {
        std::cout << __FUNCTION__ << " NULL EventCharges pointer" << std::endl;
        return -1;
    }

    HistogramManager().setEvent(aEvent);

    return 0;
}