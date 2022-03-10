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
     while counter>=procCount:
         print("Number of jobs running:",counter," Waiting one minutue.")
         time.sleep(60)
         counter = countRunningProcesses("makeTrackTree")                
################################################################
################################################################
def analyzeSingleFile(dataPath, fileName, geometryFile, command):

    filePath =  os.path.join(dataPath, fileName)
    timestamp_index = fileName.rfind("EventTPC_")
    file_timestamp = fileName[timestamp_index+9:timestamp_index+32]
        
    if timestamp_index<0:
        timestamp_index = fileName.rfind("AsAd_ALL_")
        file_timestamp = fileName[timestamp_index+9:timestamp_index+37]

    run_timestamp = fileName[timestamp_index+9:timestamp_index+32]
    run_timestamp = run_timestamp.replace(":","-")    
    if not os.path.isdir(run_timestamp):
        os.mkdir(run_timestamp)

    file_timestamp = file_timestamp.replace(":","-")    
    outputName = file_timestamp + ".out"
    arguments = " --geometryFile " + geometryFile + " --dataFile " + filePath + " > "+outputName+" 2>&1 &"
    print("Running job for file:"+fileName)                    
    os.chdir(run_timestamp)
    os.system("ln -s ../*Formats* ./")
    os.system(command+arguments)
    os.chdir("../")
################################################################
################################################################
def finalize():

    procName = "makeTrackTree"
    procCount = 1
    waintUnitilProcCount(procName, procCount)

    samples_calibration_12MHz = [
        "2021-11-25T13-53-16.129",
        "2021-11-25T15-00-32.273",
        "2021-11-25T15-21-05.094",
        "2021-11-25T16-29-22.081"        
    ]
    
    samples_calibration_25MHz = [
        "2021-11-25T14-07-04.200",
        "2021-11-25T14-33-41.411",
        "2021-11-25T16-12-14.995"
    ]
    
    samples_IFJ_VdG = [
    "2021-06-16T17-46-28.582",
    "2021-06-22T12-01-56.568",
    "2021-06-23T14-16-30.884",  
    "2021-06-23T19-24-16.737",
    "2021-06-17T11-54-38.000",
    "2021-06-22T14-11-08.614",
    "2021-06-23T18-39-39.905",
    ]

    command = "mkdir 2021-11-25_12.5MHz 2021-11-25_25.0MHz IFJ_VdG 2018"
    os.system(command)

    command = "mv 2018-* 2018"
    os.system(command)
    command = "hadd -f 2018/2018.root 2018/*/*.root"

    for item in samples_calibration_12MHz:
        command = "mv "+item+" 2021-11-25_12.5MHz"
        os.system(command)
        command = "hadd -f 2021-11-25_12.5MHz/12.5MHz.root 2021-11-25_12.5MHz/*/*.root"
        os.system(command)

    for item in samples_calibration_25MHz:
        command = "mv "+item+" 2021-11-25_25.0MHz"
        os.system(command)
        command = "hadd -f 2021-11-25_25.0MHz/25.0MHz.root 2021-11-25_25.0MHz/*/*.root"
        os.system(command)

    for item in samples_IFJ_VdG:
        command = "mv "+item+" IFJ_VdG"
        os.system(command)
        command = "hadd -f IFJ_VdG/IFJ_VdG.root IFJ_VdG/*/*.root"
        os.system(command)

################################################
################################################
def analyzeDataInDirectory(dataPath, geometryFile):

    procName = "makeTrackTree"
    command = "time ../../bin/"+procName
    procCount = 10

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
          ("/scratch/akalinow/ELITPC/data/2018/",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_mini_eTPC.dat"),
]
###
###
'''
runs = [
     ("/scratch/akalinow/ELITPC/data/2018/",
      "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_mini_eTPC.dat"),
]
'''

runs = [
    ("/scratch/akalinow/ELITPC/data/calibration/2021-11-25_12.5MHz/",
     "../geometry_ELITPC_250mbar_12.5MHz.dat"),
]
'''
runs = [  ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
          "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat"),
    ]
'''
################################################
################################################      

for dataPath, geometryFile in runs:
    analyzeDataInDirectory(dataPath, geometryFile)
finalize()
################################################
################################################      
              

