#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, time
from tpc_utils import *

################################################################
################################################################
def analyzeSingleBatch(runId, fileCSV, geometryFile, command):

    outputName = runId + ".out"
    
    runId = runId[:-9]
    if not os.path.isdir(runId):
        os.mkdir(runId)

    arguments = " --geometryFile " + geometryFile + " --dataFile " + fileCSV + " > "+outputName+" 2>&1 &"
    print("Running job id:",runId,"\nfor file(s):\n\t"+ fileCSV.replace(",","\n\t"))
    os.chdir(runId)
    os.system("ln -s ../*Formats* ./")
    os.system("ln -s ../*.dat ./")
    print(command+arguments)
    os.system(command+arguments)
    time.sleep(1)
    os.chdir("../")
################################################
################################################
def analyzeDataInDirectory(dataPath, geometryFile, procName):
    
    command = "../../bin/"+procName
    procCount = 2

    runDataList = getCSVinputList(dataPath)

    for runId, fileCSV in runDataList.items():
        waintUnitilProcCount(procName, procCount)
        analyzeSingleBatch(runId, fileCSV, geometryFile, command)
################################################
################################################
def runLoop(runs, procName, finalizeFunc):

    for dataPath, geometryFile in runs:
        analyzeDataInDirectory(dataPath, geometryFile, procName)

    procCount = 1
    waintUnitilProcCount(procName, procCount)    
    
    finalizeFunc()
################################################
################################################      
              

