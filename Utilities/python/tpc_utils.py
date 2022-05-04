import os
import glob
import datetime
import time
import psutil


################################################################
################################################################
def countRunningProcesses(procName):
    counter = 0    
    for proc in psutil.process_iter():
        counter += proc.name().find(procName)>-1    
    return counter                
################################################################
################################################################
def waintUnitilProcCount(procName, procCount):

    counter = countRunningProcesses(procName)
    while counter>=procCount:
        print("Number of jobs running:",counter," Waiting one minutue.")
        time.sleep(2*60)
        counter = countRunningProcesses(procName)         
################################################################
################################################################
def getTimeFileDict(dataPath, filePattern):
    fileDict = {}
    fileList = glob.glob(dataPath+"/"+filePattern)
    for aFile in fileList:
        oldFileName = aFile
        index = aFile.rfind("/")+1
        aFile = aFile[index:]
        ##
        #aFile = aFile.replace(aFile[25],":")
        #aFile = aFile.replace(aFile[28],":")
        #command = "mv "+oldFileName+" "+dataPath+"/"+aFile
        #os.system(command)
        ##
        index = aFile.find("AsAd")
        asadId = aFile[index+4:index+5]
        
        index = -1
        year = 2019
        while index<0:
            index = aFile.rfind(str(year))
            year += 1
        timestamp_length = 28
        timestamp = aFile[index:index+timestamp_length]
        timestamp = timestamp.replace(":","-")
        fileId = timestamp+"_"+asadId
        fileDict[fileId] = dataPath+"/"+aFile
    return fileDict
################################################################
################################################################
def mergeASADfiles(timeFileDict):
    asadDict = {}
    for key, item in timeFileDict.items():
        index = item.find("AsAd")
        asadId = item[index+4:index+5]
        index = item.rfind("_")
        chunkId = item[index+1:index+5]
        fileId = key+"_AsAd"+str(asadId)
        asadDict[fileId] = item  
        
    chunkDict = {} 
    chunkId = ""
    for key in sorted(asadDict): 
        if key.find("AsAd0")>-1:
            chunkId = key[0:28]
            chunkDict[chunkId] = [asadDict[key]]
        else:
            chunkDict[chunkId].append(asadDict[key])
    return chunkDict
################################################################
################################################################
def getCSVinputList(dataPath):
    filePattern = "CoBo0_AsAd*"
    fileDict = getTimeFileDict(dataPath, filePattern)

    if len(fileDict):
        fileDict = mergeASADfiles(fileDict)
    else:
        filePattern = "*ALL_AsAd*"
        fileDict = getTimeFileDict(dataPath, filePattern)
    if not len(fileDict):
        filePattern = "*EventTPC*"
        fileDict = getTimeFileDict(dataPath, filePattern)
    csvDict = {}
    for key, item in fileDict.items():
        if type(item)==list:
            csvDict[key] = ",".join(item)
        else:
             csvDict[key] = item
    return csvDict   
################################################################
################################################################
def getRunTimeStamp(fileName):

    timestamp_index = fileName.rfind("EventTPC_")
    run_timestamp = "Unknown_timestamp"

    if timestamp_index>-1:
        run_timestamp = fileName[timestamp_index+9:timestamp_index+32]
    elif timestamp_index<0 and fileName.rfind("AsAd_ALL_")>-1:
        timestamp_index = fileName.rfind("AsAd_ALL_")
        run_timestamp = fileName[timestamp_index+9:timestamp_index+32]
    elif timestamp_index<0 and fileName.rfind("AsAd")>-1:
        timestamp_index = fileName.rfind("AsAd")
        run_timestamp = fileName[timestamp_index+6:timestamp_index+29]

    return run_timestamp
################################################################
################################################################
