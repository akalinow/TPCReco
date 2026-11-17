#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Run makeTrackTree on HIgS 2022 GRAW data, one job per chunk (4 AsAd files).

Runs are taken from the per-run list HIgS_2022_runs.csv (made by
makeRunList.py from the experiment logbooks), which gives for each run the
beam energy, TPC working point, geometry file and data location.
Select the runs and the number of chunks with the settings at the end of
this file, then run from the resources directory of the build:

  cd build/resources
  ../python/makeTrackTree.py

Job outputs are collected in resources/HIgS_2022/<E>MeV/ (see finalizeHIgS).
"""

import subprocess
import os
import shutil
import functools
from fileLoop import *

################################################################
################################################################
def moveRunDir(source, destination):
    """Move the job directory of a run to its final location.

    Files already present in the destination (e.g. from an earlier
    processing of the same run) are overwritten. Symbolic links to
    resources/ (GET format files, geometry) are removed, since they
    would be broken after the move.
    """
    print("Moving", source, "to", destination)
    os.makedirs(destination, exist_ok=True)
    for entry in os.listdir(source):
        sourcePath = os.path.join(source, entry)
        destinationPath = os.path.join(destination, entry)
        if os.path.islink(sourcePath):
            os.remove(sourcePath)
            continue
        if os.path.isdir(destinationPath) and not os.path.islink(destinationPath):
            shutil.rmtree(destinationPath)
        os.replace(sourcePath, destinationPath)
    os.rmdir(source)
################################################################
################################################################
def energyDirName(run):
    """Output subdirectory for a run, e.g. "9.85MeV"."""
    return "{:g}MeV".format(float(run["beam_energy_MeV"]))
################################################################
################################################################
def finalize(topDirName, runList):

    for run in runList:

        item = run["run_id"].replace(":","-")
        if not os.path.isdir(item):
            continue

        energyDir = topDirName+"/"+energyDirName(run)
        os.makedirs(energyDir+"/plots", exist_ok=True)

        # Merge ROOT files
        path = energyDir+"/"+item
        moveRunDir(item, path)
        command = "mkdir -p "+energyDir+"/plots/"+item
        print(command)
        os.system(command)
        command = "hadd -f "+path+".root "+path+"/*.root"
        print(command)
        os.system(command)
  
        ## Run HIGS analysis on the reco files
        command = "../bin/recoEventsAnalysis "+path+"/configDump.json --input.dataFile "+path+".root"
        print(command)
        os.system(command)

        ## Make the plots
        energy = subprocess.check_output(["grep", "energy", path+"/configDump.json"])
        energy = subprocess.check_output(["cut","-f4","-d\""], input=energy)
        energy = energy.decode("utf-8").rstrip("\n")
        command = "root -b -q \"../examples/makePlots_HIGS.cpp(\\\"Histos.root\\\", "+energy+")\" "
        print(command)
        os.system(command)
        
        ## Move plots to the plots directory
        command = "mv *.pdf Trees.root Histos.root "+energyDir+"/plots/"+item
        os.system(command)

        ## Save the git version
        command = "git log -1 > "+energyDir+"/git_version.dat"
        os.system(command)
################################################################
################################################################
def finalizeHIgS(runList):
    """Final outputs go to HIgS_2022/<E>MeV/ (relative to resources/):
         <run>/            job directory with per-chunk TrackTree_*.root, logs, configDump.json
         <run>.root        hadd of the run's TrackTree_*.root
         plots/<run>/      Histos.root, Trees.root, *.pdf
         git_version.dat
    """
    topDirName = "HIgS_2022"

    return functools.partial(finalize, topDirName, runList)
################################################
################################################                
calibration_runs = [ ("/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET",
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
                     ##
]
################################################
################################################
# Per-run metadata (beam energy, TPC working point, geometry, data location)
# from the HIgS 2022 logbooks. Regenerate with makeRunList.py.
runListFile = os.path.join(os.path.dirname(os.path.abspath(__file__)), "HIgS_2022_runs.csv")

################################################
# Run selection. An empty list means "no selection" on that field.
#
# Beam energies [MeV] of good Physics runs in HIgS_2022_runs.csv:
#
#  energy  campaign          p [mbar]  Udrift [V]  runs  chunks
#   8.51   HIgS_August_2022    130       1372         8     19
#   8.66   HIgS_August_2022    130       1372        23     78
#   8.86   HIgS_April_2022     130       1764         1      2
#   8.86   HIgS_April_2022     130       1372         2      5
#   8.86   HIgS_August_2022    130       1372        14     50
#   9.16   HIgS_August_2022    130       1568         3     18
#   9.36   HIgS_August_2022    130       1764         1     15
#   9.56   HIgS_August_2022    130       1764         2      9
#   9.85   HIgS_April_2022     130       1764         4     16
#   9.85   HIgS_August_2022    130       1764         1     10
#  11.1    HIgS_April_2022     190       3332         5     44
#  11.5    HIgS_April_2022     190       3332         2     66
#  11.9    HIgS_April_2022     190       3332         3     46
#  12.3    HIgS_April_2022     190       3332         1     66
#  12.7    HIgS_April_2022     190       3626         2     83
#  13.1    HIgS_April_2022     190       3920         2     53
#  13.1    HIgS_April_2022     250       2744         3    108
#  13.5    HIgS_April_2022     250       2744         4    149
#  13.9    HIgS_April_2022     250       2744         4    139
#
# Energies 8.86, 9.85 and 13.1 MeV were measured in more than one setup:
# use selectedCampaigns and/or selectedPressures to pick one.
################################################
selectedEnergies = [8.86]                 # [MeV], e.g. [8.66, 8.86]
selectedCampaigns = ["HIgS_August_2022"]  # "HIgS_April_2022", "HIgS_August_2022"
selectedPressures = []                    # [mbar], e.g. [190.0]
selectedRunIds = []                       # run_id prefixes, e.g. ["2022-08-22T22:03:11"]

def isSelected(value, selection, tolerance=1E-3):
    return not selection or any(abs(float(value) - float(s)) < tolerance for s in selection)

def selectRun(run):
    return (run["run_type"] == "Physics" and run["quality"] == "good"
            and isSelected(run["beam_energy_MeV"], selectedEnergies)
            and (not selectedCampaigns or run["campaign"] in selectedCampaigns)
            and isSelected(run["pressure_mbar"], selectedPressures)
            and (not selectedRunIds or run["run_id"].startswith(tuple(selectedRunIds))))

runList = [run for run in readRunList(runListFile) if selectRun(run)]

################################################
# Number of chunks (sets of 4 AsAd GRAW files) to process; -1 = all.
# Chunks are taken in run order, then chunk order (_0000, _0001, ...).
maxChunksPerRun = 1   # e.g. 1 for a quick test on every selected run
maxChunks = 1         # limit on the total over all selected runs
###
###
procName = "makeTrackTree"
################################################
################################################
runListLoop(runList, procName, finalizeHIgS(runList),
            maxChunksPerRun=maxChunksPerRun, maxChunks=maxChunks)
################################################
################################################
