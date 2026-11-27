#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, time
from tpc_utils import *

################################################################
################################################################
def analyzeSingleBatch(runId, fileCSV, geometryFile, command,
                       pressure=None, samplingRate=None, beamEnergy=None):

    outputName = runId + ".out"

    runId = runId[:19]
    if not os.path.isdir(runId):
        os.mkdir(runId)

    # Legacy behaviour: take the metadata from the geometry file name
    # and the beam energy from the "<E>MeV" part of the data path.
    if pressure is None:
        pressure = geometryFile.split("_")[3].rstrip("mbar")
    if samplingRate is None:
        samplingRate = geometryFile.split("_")[5].rstrip("MHz.dat")
    if beamEnergy is None:
        beamEnergy = fileCSV.split("MeV")[0].split("/")[-1]

    arguments = ""
    arguments += " --input.geometryFile " + geometryFile 
    arguments += " --input.dataFile " + fileCSV 
    arguments += " --conditions.pressure " + str(pressure)
    arguments += " --conditions.samplingRate "+ str(samplingRate)
    arguments += " --beamParameters.energy " + str(beamEnergy)
    arguments += " --eventFilter.enabled true"
    arguments += " --eventFilter.maxChargeLowerBound 1000"
    arguments += " --output.recoTree true"  # TPCRecoData tree, needed by recoEventsAnalysis

    print("Running job id:",runId,"\nfor file(s):\n\t"+ fileCSV.replace(",","\n\t"))
    
    os.chdir(runId)
    os.system("ln -s ../*Formats* ./")
    os.system("ln -s ../*.dat ./")

    configDumpCommand = "../../bin/dumpConfig"+arguments
    print(configDumpCommand)
    os.system(configDumpCommand)
    arguments += " > "+outputName+" 2>&1 &"
    print(command+arguments)
    os.system(command+arguments)
    time.sleep(2)
    os.chdir("../")
################################################
################################################
def analyzeDataInDirectory(dataPath, geometryFile, procName):
    
    command = "../../bin/"+procName
    procCount = 3

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
def runChunks(runList, maxChunksPerRun=-1, maxChunks=-1):
    """Chunks (sets of 4 AsAd GRAW files) of the runs of a run list (see readRunList).

    Yields (run, job id "<run_id>_<chunk>", comma separated GRAW files), in run order,
    then chunk order. Runs without data on disk and incomplete chunks are skipped.

    maxChunksPerRun: at most this many chunks of each run (the first ones); -1 = all.
    maxChunks:       at most this many chunks in total; -1 = all.
    """
    nChunksTotal = 0
    for run in runList:
        if 0 <= maxChunks <= nChunksTotal:
            return
        if not run["data_dir"]:
            print("No data on disk for run", run["run_id"], "- skipping.")
            continue
        chunks = getRunChunks(run["data_dir"], run["run_id"])
        nChunksRun = 0
        for chunkId, fileCSV in chunks.items():
            if 0 <= maxChunksPerRun <= nChunksRun or 0 <= maxChunks <= nChunksTotal:
                break
            if len(fileCSV.split(",")) != 4:
                print("Incomplete chunk", chunkId, "of run", run["run_id"], "- skipping.")
                continue
            nChunksRun += 1
            nChunksTotal += 1
            yield run, run["run_id"].replace(":", "-") + "_" + chunkId, fileCSV
################################################
################################################
def runListLoop(runList, procName, finalizeFunc, procCount=3,
                maxChunksPerRun=-1, maxChunks=-1):
    """Process runs from a per-run metadata list (see readRunList), one job per chunk.

    Geometry, pressure, sampling rate and beam energy are taken from the run list, not
    from file or directory names. For maxChunksPerRun and maxChunks see runChunks.
    """
    command = "../../bin/"+procName
    for run, jobId, fileCSV in runChunks(runList, maxChunksPerRun, maxChunks):
        waintUnitilProcCount(procName, procCount)
        analyzeSingleBatch(jobId, fileCSV, run["geometry_file"], command,
                           pressure=run["pressure_mbar"],
                           samplingRate=run["sampling_MHz"],
                           beamEnergy=run["beam_energy_MeV"])

    waintUnitilProcCount(procName, 1)
    finalizeFunc()
################################################
################################################
   
              

