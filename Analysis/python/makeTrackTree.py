#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os

dataPath = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/"
geometryFile = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"
  
#dataPath = "/scratch/akalinow/ELITPC/data/calibration/2021-11-25T13/";
#geometryFile = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#dataPath = "/scratch/akalinow/ELITPC/data/calibration/2021-11-25T14/";
#geometryFile = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"

#dataPath = "/scratch/akalinow/ELITPC/data/calibration/2021-11-25T14-20/";
#geometryFile = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"

command = "time ../bin/makeTrackTree"

for root, dirs, files in os.walk(dataPath):
    for aFile in files:
        if aFile.find(".root")!=-1 and aFile.find("EventTPC")!=-1:
                filePath =  os.path.join(dataPath, aFile)
                outputName = aFile + ".out"
                arguments = " --geometryFile " + geometryFile + " --dataFile " + filePath + " >& "+outputName+" &"
                print("Running job for file:"+aFile)
                os.system(command+arguments)


