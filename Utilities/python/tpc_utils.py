import os
import re
import csv
import json
import glob
import datetime
import time
import psutil


################################################################
################################################################
def countRunningProcesses(procName):
    counter = 0
    for proc in psutil.process_iter():
        try:
            counter += proc.name().find(procName)>-1
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass  # process ended while scanning the process list
    return counter
################################################################
################################################################
def waintUnitilProcCount(procName, procCount):

    counter = countRunningProcesses(procName)
    while counter>=procCount:
        print("Number of jobs running:",counter," Waiting two minutues.")
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
        index = aFile.find("AsAd")
        asadId = aFile[index+4:index+5]
        if index<0:
            asadId = "0"
        
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
# CoBo0_AsAd2_<time>.<ms>_<chunk>.graw (one file per AsAd) or CoBo_<time>.<ms>_<chunk>.graw (one file for all)
GRAW_NAME_PATTERN = re.compile(
    r"CoBo\d*(?:_AsAd(\d))?_(\d{4}-\d{2}-\d{2}T\d{2}[:-]\d{2}[:-]\d{2})\.\d+_(\d{4})\.graw$")
################################################################
################################################################
def getRunChunks(dataPath, runId, windowSeconds=2):
    """Find GRAW files of a single run and group them into chunks.

    runId is the run start time, e.g. "2022-08-22T22:03:11" (":" or "-"
    separators). AsAd boards open their files a few ms apart, so a file
    belongs to the run if its timestamp is within windowSeconds after runId.
    Returns an ordered dict {chunk number: comma separated list of the
    chunk's files, ordered by AsAd number}.
    """
    runStart = datetime.datetime.strptime(runId.replace(":", "-"), "%Y-%m-%dT%H-%M-%S")
    chunks = {}
    for aFile in glob.glob(os.path.join(dataPath, "*.graw")):
        match = GRAW_NAME_PATTERN.search(os.path.basename(aFile))
        if not match:
            continue
        asadId, timestamp, chunkId = match.groups()
        fileStart = datetime.datetime.strptime(timestamp.replace(":", "-"), "%Y-%m-%dT%H-%M-%S")
        delta = (fileStart - runStart).total_seconds()
        if 0 <= delta <= windowSeconds:
            chunks.setdefault(chunkId, []).append((asadId or "", aFile))
    return {chunkId: ",".join(f for _, f in sorted(files))
            for chunkId, files in sorted(chunks.items())}
################################################################
################################################################
def readDataSets(path):
    """Read the data-set JSON files of a run list directory (see Analysis/python/makeRunList.py).

    Returns the full data sets (name, description, run, input, conditions, beamParameters,
    and for MC also controller), ordered by run_id. Files without a "run" section are skipped.
    """
    dataSets = []
    for jsonFile in sorted(glob.glob(os.path.join(path, "*.json"))):
        with open(jsonFile) as f:
            dataSet = json.load(f)
        if dataSet.get("run"):
            dataSets.append(dataSet)
    return sorted(dataSets, key=lambda dataSet: dataSet["run"]["run_id"])
################################################################
################################################################
def readRunList(path):
    """Read the per-run metadata list (see Analysis/python/makeRunList.py).

    path: directory with one data-set JSON file per run (the "run" section of each file),
    or a CSV file (older run lists). Returns a list of {column: string value}, ordered by
    run_id; missing values are empty strings.
    """
    if os.path.isdir(path):
        return [{key: "" if value is None else str(value) for key, value in dataSet["run"].items()}
                for dataSet in readDataSets(path)]
    with open(path, newline="") as f:
        return list(csv.DictReader(f))
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
