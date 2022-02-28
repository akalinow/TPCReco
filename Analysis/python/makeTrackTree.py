#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, time
import psutil

################################################################
################################################################
def countRunningProcesses(procName):
    counter = 0
    for proc in psutil.process_iter():
        counter += proc.name().find("makeTrackTree")>-1

    return counter                
################################################################
################################################################
def waintUnitilProcCount(procName, procCount):

     counter = countRunningProcesses("makeTrackTree")
     while counter>procCount:
         counter = countRunningProcesses("makeTrackTree")                
         print("Number of jobs running:",counter," Waiting one minutue.")
         time.sleep(60)    
################################################################
################################################################
def analyzeSingleFile(dataPath, fileName, geometryFile, command):

    filePath =  os.path.join(dataPath, fileName)
    timestamp_index = fileName.rfind("EventTPC_")
    if timestamp_index<0:
        timestamp_index = fileName.rfind("AsAd_ALL_")
    timestamp = fileName[timestamp_index+9:timestamp_index+32]
    timestamp = timestamp.replace(":","-")
    outputName = timestamp + ".out"        
    if not os.path.isdir(timestamp):
        os.mkdir(timestamp)
    arguments = " --geometryFile " + geometryFile + " --dataFile " + filePath + " > "+outputName+" 2>&1 &"
    print("Running job for file:"+fileName)                    
    os.chdir(timestamp)
    os.system("ln -s ../*Formats* ./")
    os.system(command+arguments)
    os.chdir("../")
################################################################
################################################################
def analyzeDataInDirectory(dataPath, geometryFile):

    procName = "makeTrackTree"
    command = "time ../../bin/"+procName
    procCount = 4

    for root, dirs, files in os.walk(dataPath):
        for fileName in files:
            if (fileName.find(".root")!=-1 and fileName.find("EventTPC")!=-1) or (fileName.find(".graw")!=-1 and fileName.find("CoBo")!=-1):
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
    ("/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210621_extTrg_CO2_250mbar_DT1470ET/",
     "../geometry_ELITPC_250mbar_12.5MHz.dat"),
]
################################################
################################################      
for dataPath, geometryFile in runs:
    analyzeDataInDirectory(dataPath, geometryFile)




