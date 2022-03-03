#!/usr/bin/env python
# -*- coding: utf-8 -*-


import os, commands


#path = "/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET/"
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#path = "/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210617_extTrg_CO2_250mbar_DT1470ET/";
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#path = "/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210621_extTrg_CO2_250mbar_DT1470ET/"
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#path = "/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/"
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#path = "/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210623_extTrg_CO2_250mbar_DT1470ET/"
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

#path = "/data/edaq/2021/20211125_extTrg_CO2_250mbar_DT1470ET/"
#geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_ELITPC_250mbar_12.5MHz.dat"

path = "/mnt/NAS_STORAGE_BIG/dump_20180625/data_edaq_20180625/"
geometryFile = "/home/akalinow/scratch/TPCReco/build/resources/geometry_mini_eTPC.dat"

command = "../bin/grawToEventTPC"
#command = "../bin/grawToEventRaw"

filePrefix = "EventTPC"
if command.find("EventRaw")!=-1:
    filePrefix = "EventRaw"

grawFileList = {}

for root, dirs, files in os.walk(path):
    for aFile in files:
        if aFile.find(".graw")!=-1 and aFile.find("CoBo")!=-1:
            timestamp_index = aFile.rfind("ALL_")
            if timestamp_index<0:
                timestamp_index = aFile.rfind("oBo_")
            timestamp = aFile[timestamp_index+4:timestamp_index+27]
            print("timestamp:",timestamp)
            if timestamp in grawFileList.keys():
                grawFileList[timestamp].append(os.path.join(path, aFile))
            else:
                grawFileList[timestamp] = [os.path.join(path, aFile)]

runId = 0
for aTimestamp, aRunFileList in grawFileList.items():
    runId += 1
    aRunFileList.sort()
    #if aTimestamp!="2021-06-23T19:24:16.737":
    #    continue
    
    print(aRunFileList)

    for chunkId, aFile in enumerate(aRunFileList):
        arguments = aFile+" "+geometryFile+" "+filePrefix+"_"+aTimestamp+"_"+str(chunkId)+".root"
        print(aFile, aTimestamp, arguments)
        os.system(command+" "+arguments)
        if chunkId>500:
            break
