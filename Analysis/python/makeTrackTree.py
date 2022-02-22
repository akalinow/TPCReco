#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, time
import psutil

################################################################
################################################################
runs = [ ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210623_extTrg_CO2_250mbar_DT1470ET/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25T13/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25T14/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"),
         ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25T14-20/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat")
]
################################################################
################################################################
def countRunningProcesses(procName):
    counter = 0
    for proc in psutil.process_iter(attrs=['name'], ad_value=None):
                    counter += proc.info["name"].find("makeTrackTree")>-1
    return counter                
################################################################
################################################################
def waintUnitilProcCount(procName, procCount):

     counter = countRunningProcesses("makeTrackTree")
     while counter>procCount:
         counter = countRunningProcesses("makeTrackTree")                
         if counter>=10:
             print("Number of jobs running:",counter," Waiting one minutue.")
             time.sleep(60)    
################################################################
################################################################
def analyzeSingleFile(dataPath, fileName, geometryFile, command):

    filePath =  os.path.join(dataPath, fileName)
    outputName = fileName + ".out"
    timestamp_index = fileName.rfind("EventTPC_")
    timestamp = fileName[timestamp_index+9:timestamp_index+32]
    timestamp = timestamp.replace(":","-")
    if not os.path.isdir(timestamp):
        os.mkdir(timestamp)
    arguments = " --geometryFile " + geometryFile + " --dataFile " + filePath + " >& "+outputName+" &"
    print("Running job for file:"+fileName)                    
    os.chdir(timestamp)
    os.system(command+arguments)
    os.chdir("../")
################################################################
################################################################
def analyzeDataInDirectory(dataPath, geometryFile):

    procName = "makeTrackTree"
    command = "time ../../bin/"+procName
    procCount = 10

    for root, dirs, files in os.walk(dataPath):
        for fileName in files:
            if fileName.find(".root")!=-1 and fileName.find("EventTPC")!=-1:
                waintUnitilProcCount(procName, procCount)
                analyzeSingleFile(dataPath, fileName, geometryFile, command)
################################################
################################################                
runs = [ ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ##
          ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210617_extTrg_CO2_250mbar_DT1470ET",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ##
         ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ##
         ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210623_extTrg_CO2_250mbar_DT1470ET/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ##
         ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25_12.5MHz/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
         ##
         ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25_25MHz/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_25.0MHz.dat"),
         ##
]

runs = [
    ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210623_extTrg_CO2_250mbar_DT1470ET/",
     "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
]
################################################
################################################      
for dataPath, geometryFile in runs:
    analyzeDataInDirectory(dataPath, geometryFile)




