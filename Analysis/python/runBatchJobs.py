#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Batch processing of HIgS 2022 data and Monte Carlo with tpcReco / makeMCTrackTree.

Runs are data sets, one JSON file per run (made by makeRunList.py):
  HIgS_2022_runs/     data runs (makeRunList.py): beam energy, TPC working point,
                      geometry file and data location of each run
  HIgS_2022_MC_runs/  MC setups (makeRunList.py --mc, run_type "MC"): the same
                      sections, plus n_events, n_jobs and a "controller" section
                      merged onto the MC controller template (input.controllerConfigPath)

Data: one tpcReco job per chunk (4 AsAd GRAW files). Outputs are collected
in resources/HIgS_2022/<E>MeV/ (see finalizeData).
MC: n_jobs makeMCTrackTree jobs per setup (events generated and reconstructed in one
pass, input.fromGenerator). Outputs in <mcOutputDir>/<setup>/ (see finalizeMC).

Select the runs with the settings at the end of this file, then run from the
resources directory of the build:

  cd build/resources
  ../python/runBatchJobs.py
"""

import subprocess
import os
import re
import json
import glob
import time
import shutil
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
def finalizeData(runList, topDirName="HIgS_2022"):
    """Final outputs go to HIgS_2022/<E>MeV/ (relative to resources/):
         <run>/            job directory with per-chunk TrackTree_*.root, logs, configDump.json
         <run>.root        hadd of the run's TrackTree_*.root
         plots/<run>/      Histos.root, Trees.root, *.pdf
         git_version.dat
    """
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
def mergeConfig(base, override):
    """Merge override onto base: dicts recursively, lists element by element, other values replaced."""
    if isinstance(base, dict) and isinstance(override, dict):
        merged = dict(base)
        for key, value in override.items():
            merged[key] = mergeConfig(base[key], value) if key in base else value
        return merged
    if isinstance(base, list) and isinstance(override, list):
        return [mergeConfig(base[i], value) if i < len(base) else value
                for i, value in enumerate(override)] + base[len(override):]
    return override
################################################################
################################################################
def mcSetupName(dataSet):
    """e.g. "9.85MeV_130mbar_1764V" for run_id "MC_9.85MeV_130mbar_1764V"."""
    runId = dataSet["run"]["run_id"]
    return runId[3:] if runId.startswith("MC_") else runId
################################################################
################################################################
def makeControllerConfig(dataSet, fileName):
    """MC controller config of a job: the template with the data set's "controller" section
    and the machine dependent paths (geometry, strip response files)."""
    with open(os.path.join("../config", dataSet["input"]["controllerConfigPath"])) as f:
        config = json.load(f)
    config = mergeConfig(config, dataSet.get("controller", {}))
    config["GeometryConfig"] = os.path.abspath(dataSet["input"]["geometryFile"])
    if "TPCDigitizerSRC" in config.get("ModuleConfiguration", {}):
        config["ModuleConfiguration"]["TPCDigitizerSRC"]["StripResponsePath"] = stripResponsePath
    with open(fileName, "w") as f:
        json.dump(config, f, indent=2)
################################################################
################################################################
def submitMCJob(dataSet, iJob, nEvents):
    jobDir = os.path.join(mcOutputDir, mcSetupName(dataSet), "job_{}".format(iJob))
    os.makedirs(jobDir, exist_ok=True)
    resources = os.getcwd()
    controllerFile = os.path.join(jobDir, "controller.json")
    makeControllerConfig(dataSet, controllerFile)
    arguments = ""
    arguments += " --input.geometryFile " + os.path.join(resources, dataSet["input"]["geometryFile"])
    arguments += " --input.controllerConfigPath " + os.path.abspath(controllerFile)
    arguments += " --input.dataFile MC_{}_{}.root".format(mcSetupName(dataSet), iJob)
    arguments += " --input.readNEvents {}".format(nEvents)
    arguments += " --input.fromGenerator true"
    for section in ("conditions", "beamParameters"):
        for key, value in dataSet.get(section, {}).items():
            arguments += " --{}.{} {:g}".format(section, key, value)
    arguments += " --eventFilter.enabled false"
    bin = os.path.join(resources, "../bin")
    os.system("cd {} && {}/dumpConfig {} {} > /dev/null 2>&1".format(
        jobDir, bin, os.path.join(resources, mcAppConfig), arguments))
    command = "cd {} && {}/makeMCTrackTree {} {} > job.out 2>&1 &".format(
        jobDir, bin, os.path.join(resources, mcAppConfig), arguments)
    print(command, flush=True)
    os.system(command)
    time.sleep(2)
################################################################
################################################################
def finalizeMC(mcDataSets):
    """<mcOutputDir>/<setup>.root: hadd of the setup's job outputs <setup>/job_<n>/MC_*.root."""
    for dataSet in mcDataSets:
        setupDir = os.path.join(mcOutputDir, mcSetupName(dataSet))
        files = sorted(glob.glob(os.path.join(setupDir, "job_*", "MC_*.root")))
        if not files:
            continue
        command = "hadd -f {}.root {}".format(setupDir, " ".join(files))
        print(command, flush=True)
        os.system(command)
    if mcDataSets:
        os.system("git log -1 > {}/git_version.dat".format(mcOutputDir))
################################################################
################################################################
procNames = ["tpcReco", "makeMCTrackTree"]

def countJobs():
    return sum(countRunningProcesses(name) for name in procNames)

def waitForSlot():
    while countJobs() >= procCount:
        time.sleep(60)
################################################
################################################
# Data sets: data runs from the HIgS 2022 logbooks (makeRunList.py) and MC setups
# (makeRunList.py --mc; can be edited by hand, e.g. n_events or the controller section).
pythonDir = os.path.dirname(os.path.abspath(__file__))
runListFile = os.path.join(pythonDir, "HIgS_2022_runs")
mcRunListFile = os.path.join(pythonDir, "HIgS_2022_MC_runs")

################################################
# Data location: GRAW files of a run are in <dataPathPrefix>/<dataDirName>/<day directory>,
# e.g. .../data/HIgs_2022/20220822_extTrg_CO2_130mbar. The day directory is taken from
# the run list (data_dir); its location in the run list is ignored.
dataPathPrefix = "/scratch_hdd/akalinow/ELITPC/data"
dataDirName = "HIgs_2022"

def localDataDir(run):
    """data_dir of a run moved under dataPathPrefix/dataDirName ("" if the run has no data)."""
    if not run["data_dir"]:
        return ""
    parts = os.path.normpath(run["data_dir"]).split(os.sep)
    campaignDirs = [i for i, part in enumerate(parts) if re.fullmatch(r"HIgS_\d{4}", part, re.IGNORECASE)]
    relative = parts[campaignDirs[-1]+1:] if campaignDirs else parts[-1:]
    return os.path.join(dataPathPrefix, dataDirName, *relative)

################################################
# MC: output location, application config, strip response model files
# (StripResponseModel_..._T<sigmaXY>mm_L<sigmaZ>mm_P232ns.root for the sigmas of the MC data sets).
mcOutputDir = "/scratch_hdd/akalinow/ELITPC/WarsawTPC_Claude/data/HIgS_2022_MC"
mcAppConfig = "../config/config_GRAW2ROOT_MC.json"
stripResponsePath = "/scratch_hdd/akalinow/ELITPC/TPCReco/build_noGET/MonteCarlo/Modules/StripResponseModel/"

################################################
# Run selection. An empty list means "no selection" on that field.
#
# Beam energies [MeV] of good Physics runs in HIgS_2022_runs/:
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
# MC: one setup per (energy, pressure, drift voltage), run_id "MC_<E>MeV_<p>mbar_<U>V",
# campaign "HIgS_2022_MC".
################################################
selectedRunTypes = ["Physics"]            # "Physics" (data), "MC"
selectedEnergies = []                     # [MeV], e.g. [8.66, 8.86]
selectedCampaigns = []                    # "HIgS_April_2022", "HIgS_August_2022", "HIgS_2022_MC"
selectedPressures = []                    # [mbar], e.g. [190.0]
selectedRunIds = ["2022-08-22T22:03:11"]  # 9.85 MeV, Aug (calibration run, all 10 chunks)
                                          # run_id prefixes, e.g. "MC_9.85MeV"; [] = no selection

def isSelected(value, selection, tolerance=1E-3):
    return not selection or any(abs(float(value) - float(s)) < tolerance for s in selection)

def selectRun(run):
    return (run["run_type"] in selectedRunTypes and run["quality"] == "good"
            and isSelected(run["beam_energy_MeV"], selectedEnergies)
            and (not selectedCampaigns or run["campaign"] in selectedCampaigns)
            and isSelected(run["pressure_mbar"], selectedPressures)
            and (not selectedRunIds or run["run_id"].startswith(tuple(selectedRunIds))))

runList = [run for run in readRunList(runListFile) if selectRun(run)]
mcDataSets = [dataSet for dataSet in readDataSets(mcRunListFile) if selectRun(dataSet["run"])]
for run in runList:
    run["data_dir"] = localDataDir(run)

# Keep only the first run (with data on disk) of each setup:
# beam energy, campaign, pressure and drift voltage.
firstRunPerSetup = False
if firstRunPerSetup:
    setups = {}
    for run in runList:
        if run["data_dir"]:
            setups.setdefault((run["beam_energy_MeV"], run["campaign"],
                               run["pressure_mbar"], run["drift_V"]), run)
    runList = [run for run in runList if run in setups.values()]

################################################
# Data: number of chunks (sets of 4 AsAd GRAW files) to process; -1 = all.
# Chunks are taken in run order, then chunk order (_0000, _0001, ...).
maxChunksPerRun = -1  # e.g. 1 for a quick test on every selected run
maxChunks = -1        # limit on the total over all selected runs (a chunk takes ~40 min)
# MC: generated events per setup, split into n_jobs jobs; -1 = n_events of the data set
mcEventsPerSetup = -1 # e.g. 40 for a quick test
procCount = 14        # number of parallel jobs (tpcReco + makeMCTrackTree)
################################################
################################################
for run, jobId, fileCSV in runChunks(runList, maxChunksPerRun, maxChunks):
    waitForSlot()
    analyzeSingleBatch(jobId, fileCSV, run["geometry_file"], "../../bin/tpcReco",
                       pressure=run["pressure_mbar"],
                       samplingRate=run["sampling_MHz"],
                       beamEnergy=run["beam_energy_MeV"])

# longest jobs (highest pressure) first
for dataSet in sorted(mcDataSets, key=lambda d: -float(d["run"]["pressure_mbar"])):
    nJobs = int(dataSet["run"]["n_jobs"])
    nEvents = mcEventsPerSetup if mcEventsPerSetup >= 0 else int(dataSet["run"]["n_events"])
    for iJob in range(nJobs):
        waitForSlot()
        submitMCJob(dataSet, iJob, nEvents//nJobs)

while countJobs() > 0:
    time.sleep(60)
finalizeData(runList)
finalizeMC(mcDataSets)
################################################
################################################
