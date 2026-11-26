#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate and reconstruct Monte Carlo O16(gamma,alpha)C12 events with
makeMCTrackTree for the HIgS 2022 setups (beam energy, pressure, drift
voltage, geometry) found in the run data sets HIgS_2022_runs/ (makeRunList.py).

For each setup the MC controller config (montecarlo_2prong_gun_PEventTPC.json)
is adapted: gamma energy (Gaussian, sigma = beamEnergySpread x E), gas
pressure and geometry. Events are generated and reconstructed in one pass
(input.fromGenerator), no raw event files are written.

Run from the resources directory of the build:

  cd build/resources
  ../python/makeMCTrackTree.py

Outputs go to HIgS_2022_MC/<E>MeV_<p>mbar_<U>V/:
  job_<n>/           job directory with MC_<setup>_<n>_<date>.root, log, configs
  <setup>.root       hadd of the job outputs
"""

import os
import json
import time
import glob
import shutil
from fileLoop import readRunList
from tpc_utils import countRunningProcesses

################################################
# Settings
################################################
runListFile = os.path.join(os.path.dirname(os.path.abspath(__file__)), "HIgS_2022_runs")
selectedEnergies = []        # [MeV]; empty = all energies with Physics runs
selectedSetups = []          # setup names, e.g. ["12.7MeV_190mbar_3626V"]; empty = all
nEventsPerSetup = 16000      # generated events per setup
nJobsPerSetup = 4            # parallel jobs per setup (independent random seeds)
beamEnergySpread = 0.02      # relative sigma of the gamma energy
maxProcs = 15                # limit on makeMCTrackTree + makeTrackTree processes
procName = "makeMCTrackTree"
otherProcNames = ["makeTrackTree"]   # data jobs sharing the machine
topDirName = "/scratch_hdd/akalinow/ELITPC/WarsawTPC_Claude/data/HIgS_2022_MC"
controllerTemplate = "../config/montecarlo_2prong_gun_PEventTPC.json"
appConfig = "../config/config_Writer_MC.json"
# Strip-response model (TPCDigitizerSRC, 232 ns shaping, TPCReco >= de1e73b). sigmaXY and sigmaZ from the diffusion
# fit of WarsawTPC_Claude/code/7_diffusion.ipynb at 130 mbar (1.0 / 1.2 mm); sigmaZ scaled with sqrt(130/p).
# The files StripResponseModel_..._T<sigmaXY>mm_L<sigmaZ>mm_P232ns.root must exist in stripResponsePath.
stripResponsePath = "/scratch_hdd/akalinow/ELITPC/TPCReco/build_noGET/MonteCarlo/Modules/StripResponseModel/"
sigmaXY = 1.0                                   # [mm]
sigmaZByPressure = {130: 1.2, 190: 1.0, 250: 0.85}   # [mm], key: pressure [mbar]
################################################

def getSetups(runList):
    """Unique (energy, pressure, drift voltage, sampling, geometry) of good Physics runs."""
    setups = {}
    for run in runList:
        if run["run_type"] != "Physics" or run["quality"] != "good":
            continue
        energy = float(run["beam_energy_MeV"])
        if selectedEnergies and not any(abs(energy - e) < 1E-3 for e in selectedEnergies):
            continue
        key = (energy, float(run["pressure_mbar"]), float(run["drift_V"]))
        setups.setdefault(key, {"energy": energy, "pressure": float(run["pressure_mbar"]),
                                "driftV": float(run["drift_V"]),
                                "sampling": float(run["sampling_MHz"]),
                                "geometry": run["geometry_file"]})
    result = [setups[k] for k in sorted(setups)]
    if selectedSetups:
        result = [s for s in result if setupName(s) in selectedSetups]
    return result

def setupName(setup):
    return "{:g}MeV_{:g}mbar_{:g}V".format(setup["energy"], setup["pressure"], setup["driftV"])

def makeControllerConfig(setup, fileName):
    with open(controllerTemplate) as f:
        config = json.load(f)
    config["GeometryConfig"] = os.path.abspath(setup["geometry"])
    generator = config["ModuleConfiguration"]["Generator"]["EventGenerator"]
    generator["Beam"]["GammaEnergy"] = {
        "distribution": "EProviderGaus",
        "parameters": {"meanE": setup["energy"],
                       "sigmaE": round(beamEnergySpread*setup["energy"], 4)}}
    # [bar]
    config["ModuleConfiguration"]["GeantSim"]["gas_mixture"]["co2"] = setup["pressure"]/1000.0
    config["ModuleConfiguration"]["ToyIonizationSimulator"]["Pressure"] = setup["pressure"]/1000.0
    digitizer = config["ModuleConfiguration"]["TPCDigitizerSRC"]
    digitizer["StripResponsePath"] = stripResponsePath
    digitizer["sigmaXY"] = sigmaXY
    digitizer["sigmaZ"] = sigmaZByPressure[int(round(setup["pressure"]))]
    with open(fileName, "w") as f:
        json.dump(config, f, indent=2)

def waitForSlot():
    while (countRunningProcesses(procName)
           + sum(countRunningProcesses(p) for p in otherProcNames)) >= maxProcs:
        time.sleep(60)

def submitJob(setup, iJob, nEvents):
    jobDir = os.path.join(topDirName, setupName(setup), "job_{}".format(iJob))
    os.makedirs(jobDir, exist_ok=True)
    resources = os.getcwd()
    controllerFile = os.path.join(jobDir, "controller.json")
    makeControllerConfig(setup, controllerFile)
    arguments = ""
    arguments += " --input.geometryFile " + os.path.join(resources, setup["geometry"])
    arguments += " --input.controllerConfigPath " + os.path.abspath(controllerFile)
    arguments += " --input.dataFile MC_{}_{}.root".format(setupName(setup), iJob)
    arguments += " --input.readNEvents {}".format(nEvents)
    arguments += " --input.fromGenerator true"
    arguments += " --conditions.pressure {:g}".format(setup["pressure"])
    arguments += " --conditions.samplingRate {:g}".format(setup["sampling"])
    arguments += " --beamParameters.energy {:g}".format(setup["energy"])
    arguments += " --eventFilter.enabled false"
    bin = os.path.join(resources, "../bin")
    os.system("cd {} && {}/dumpConfig {} {} > /dev/null 2>&1".format(
        jobDir, bin, os.path.join(resources, appConfig), arguments))
    command = "cd {} && {}/{} {} {} > job.out 2>&1 &".format(
        jobDir, bin, procName, os.path.join(resources, appConfig), arguments)
    print(command, flush=True)
    os.system(command)
    time.sleep(2)

def finalize(setups):
    for setup in setups:
        setupDir = os.path.join(topDirName, setupName(setup))
        files = sorted(glob.glob(os.path.join(setupDir, "job_*", "MC_*.root")))
        if not files:
            continue
        command = "hadd -f {}.root {}".format(setupDir, " ".join(files))
        print(command, flush=True)
        os.system(command)
    os.system("git log -1 > {}/git_version.dat".format(topDirName))

################################################
setups = getSetups(readRunList(runListFile))
print("{} setups, {} events each in {} jobs".format(len(setups), nEventsPerSetup, nJobsPerSetup))
# longest jobs (highest pressure) first
jobs = [(setup, iJob) for setup in sorted(setups, key=lambda s: -s["pressure"])
        for iJob in range(nJobsPerSetup)]
nEventsPerJob = nEventsPerSetup//nJobsPerSetup
for setup, iJob in jobs:
    waitForSlot()
    submitJob(setup, iJob, nEventsPerJob)

while countRunningProcesses(procName) > 0:
    time.sleep(60)
finalize(setups)
