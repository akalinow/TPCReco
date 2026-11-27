#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Build the per-run data sets (JSON) for the HIgS 2022 or 2026 campaigns.

Reads the experiment logbooks (2022: April .ods and August .xlsx spreadsheets;
2026: HIgS_August_2026_exp_notes.xlsx, sheet HIgS_August),
keeps the Physics and Background runs, assigns the geometry file matching
the logged TPC working point (pressure, drift voltage, sampling rate), and
locates the GRAW files of each run in the data directories.

Writes one JSON file per run (<run_id>.json, ':' replaced by '-') to the output
directory. Each file is a tpcWebGUI data set (Open dialog, web.dataSetDirs) and
the run list of the batch scripts (tpc_utils.readRunList):
  name, description  - shown in the tpcWebGUI Open dialog
  run                - logbook metadata (run_id, campaign, run_type, quality,
                       beam_energy_MeV, pressure_mbar, sampling_MHz, drift_V, ...,
                       data_dir, n_chunks_found)
  input              - dataFile (GRAW files of the first chunk, one per AsAd;
                       empty without data on disk), geometryFile
  conditions         - pressure, samplingRate (and temperature, if logged)
  beamParameters     - energy

With --mc, writes instead one MC data set per setup (beam energy, pressure, drift voltage)
of the good Physics runs, to HIgS_<campaign>_MC_runs/ (makeTrackTree.py, run_type "MC"):
  run                - run_id "MC_<E>MeV_<p>mbar_<U>V", run_type "MC", the setup, n_events, n_jobs
  input              - geometryFile, controllerConfigPath (MC controller template in config/)
  conditions, beamParameters - as for the data
  controller         - settings merged onto the controller template (see MC_DEFAULTS): gamma energy,
                       gas pressure, strip response sigmas. Machine dependent paths (geometry,
                       strip response files) are set by makeTrackTree.py.
Existing MC files of the same setups are overwritten, other files in the directory are kept.

Usage:
  makeRunList.py [--campaign 2022|2026] [--logbook PATH] [--output DIR] [--dataDir GLOB ...] [--mc]
Defaults per campaign: see CAMPAIGNS.
"""

import argparse
import glob
import html
import json
import os
import re
import sys
import zipfile

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../Utilities/python"))
from tpc_utils import getRunChunks

################################################################
################################################################
# (pressure [mbar], drift voltage [V], sampling rate [MHz]) -> geometry file in resources/
GEOMETRY_FILES = {
    (130, 1372, 25.0): "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat",
    (130, 1568, 25.0): "geometry_ELITPC_130mbar_1568Vdrift_25MHz.dat",
    (130, 1764, 25.0): "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat",
    (190, 3332, 25.0): "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",
    (190, 3626, 25.0): "geometry_ELITPC_190mbar_3626Vdrift_25MHz.dat",
    (190, 3920, 25.0): "geometry_ELITPC_190mbar_3920Vdrift_25MHz.dat",
    (250, 2744, 12.5): "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat",
    (110, 1470, 25.0): "geometry_ELITPC_110mbar_1470Vdrift_25MHz.dat",  # HIgS 2026 online snapshot
}

# defaults per campaign: logbook (2022: directory with both spreadsheets), data directories
# (globs; local disk first, NFS copy second), output directory
CAMPAIGNS = {
    "2022": dict(logbook="/scratch_hdd/akalinow/ELITPC/logbook",
                 dataDirs=["/scratch_hdd/akalinow/ELITPC/data/HIgs_2022/2022*_extTrg_*",
                           "/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/2022*_extTrg_*/*MeV/GRAW"],
                 output="HIgS_2022_runs"),
    "2026": dict(logbook="/scratch_hdd/akalinow/ELITPC/data/HIgS_2026/HIgS_August_2026_exp_notes.xlsx",
                 dataDirs=["/scratch_hdd/akalinow/ELITPC/data/HIgS_2026/2026*_extTrg_*"],
                 output="HIgS_2026_runs"),
}

# MC production settings of the --mc data sets.
# Strip-response model (TPCDigitizerSRC, 232 ns shaping, TPCReco >= de1e73b). sigmaXY and sigmaZ from the diffusion
# fit of WarsawTPC_Claude/code/7_diffusion.ipynb at 130 mbar (1.0 / 1.2 mm); sigmaZ scaled with sqrt(130/p).
MC_DEFAULTS = dict(
    controllerTemplate="montecarlo_2prong_gun_PEventTPC.json",  # in config/
    nEvents=16000,              # generated events per setup
    nJobs=4,                    # parallel jobs per setup (independent random seeds)
    beamEnergySpread=0.02,      # relative sigma of the gamma energy
    sigmaXY=1.0,                # [mm]
    sigmaZByPressure={130: 1.2, 190: 1.0, 250: 0.85},  # [mm], key: pressure [mbar]
)

COLUMNS = ["run_id", "campaign", "run_type", "quality", "beam_energy_MeV",
           "pressure_mbar", "sampling_MHz", "drift_V", "drift_preset",
           "trigger_delay_10ns", "geometry_file", "n_chunks_logbook",
           "n_chunks_found", "data_dir", "comment",
           "temperature_C", "n_streams"]  # n_streams: GRAW files per chunk found (4: one per AsAd, 1: one for all)
################################################################
################################################################
def readXLSX(path, sheetName=None):
    """Return rows of the first (or the named) sheet as {column letter: value}."""
    z = zipfile.ZipFile(path)
    sheetFile = "xl/worksheets/sheet1.xml"
    if sheetName:
        names = re.findall(r'<sheet [^>]*name="([^"]*)"', z.read("xl/workbook.xml").decode())
        sheetFile = "xl/worksheets/sheet{}.xml".format(names.index(sheetName) + 1)
    strings = [html.unescape(re.sub("<[^>]+>", "", s)) for s in
               re.findall(r"<si>(.*?)</si>", z.read("xl/sharedStrings.xml").decode(), re.S)]
    rows = []
    sheet = z.read(sheetFile).decode()
    for row in re.findall(r"<row [^>]*>(.*?)</row>", sheet, re.S):
        cells = {}
        for attrs, body in re.findall(r"<c ([^>]*?)(?:/>|>(.*?)</c>)", row, re.S):
            column = re.search(r'r="([A-Z]+)\d+"', attrs).group(1)
            value = re.search(r"<v>(.*?)</v>", body or "")
            if value:
                value = value.group(1)
                cells[column] = strings[int(value)] if 't="s"' in attrs else value
        rows.append(cells)
    return rows
################################################################
################################################################
def readODS(path):
    """Return rows of the first sheet as lists of cell texts."""
    content = zipfile.ZipFile(path).read("content.xml").decode()
    table = re.search(r"<table:table [^>]*>(.*?)</table:table>", content, re.S).group(1)
    rows = []
    for row in re.findall(r"<table:table-row[^>]*>(.*?)</table:table-row>", table, re.S):
        cells = []
        for attrs, body in re.findall(r"<table:table-cell([^>]*?)(?:/>|>(.*?)</table:table-cell>)", row, re.S):
            repeat = re.search(r'number-columns-repeated="(\d+)"', attrs)
            text = re.sub(r"<text:s[^>]*/>", " ", body or "")
            text = html.unescape(re.sub("<[^>]+>", "", text.replace("</text:p>", "\n"))).strip()
            cells += [text] * min(int(repeat.group(1)) if repeat else 1, 64)
        rows.append(cells)
    return rows
################################################################
################################################################
def toFloat(text):
    try:
        return float(text)
    except (TypeError, ValueError):
        return None
################################################################
################################################################
def classify(runType):
    """Map the logbook RUN type to (run_type, quality)."""
    runType = (runType or "").strip()
    quality = "bad" if "not good" in runType.lower() else "good"
    if runType.lower().startswith("physic"):
        return "Physics", quality
    if runType.lower().startswith("background"):
        return "Background", quality
    return None, quality
################################################################
################################################################
def nChunksFromLogbook(text):
    """'_0000 to _0009' -> 10, '_0000' -> 1."""
    numbers = [int(n) for n in re.findall(r"_?(\d{4})", text or "")]
    if not numbers:
        return ""
    return max(numbers) - min(numbers) + 1
################################################################
################################################################
def aprilRuns(path):
    # Column positions of the HIgS_April sheet (see the header rows).
    TS, BEAM, ENERGY, SAMPLING, DELAY, PRESET, DRIFT, PRESSURE, TYPE = 1, 10, 11, 17, 18, 22, 25, 27, 30
    runs = []
    for cells in readODS(path):
        if len(cells) <= TYPE or not re.match(r"\d{4}-\d{2}-\d{2}T", cells[TS]):
            continue
        runType, quality = classify(cells[TYPE])
        if runType is None:
            continue
        runs.append(dict(
            run_id=cells[TS], campaign="HIgS_April_2022", run_type=runType, quality=quality,
            beam_on=cells[BEAM].upper() == "ON", beam_energy_MeV=toFloat(cells[ENERGY]),
            pressure_mbar=toFloat(cells[PRESSURE]), sampling_MHz=toFloat(cells[SAMPLING]),
            drift_V=toFloat(cells[DRIFT]), drift_preset=cells[PRESET],
            trigger_delay_10ns=cells[DELAY], n_chunks_logbook="",
            comment="; ".join(c for c in cells[TYPE + 1:] if c)))
    return runs
################################################################
################################################################
def augustRuns(path):
    runs = []
    pressure = 130.0  # pressure is logged only at the start of the day
    for cells in readXLSX(path):
        if toFloat(cells.get("AD")):
            pressure = toFloat(cells["AD"])
        match = re.search(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}", cells.get("B", ""))
        if not match:
            continue
        runType, quality = classify(cells.get("AG"))
        if runType is None:
            continue
        runs.append(dict(
            run_id=match.group(0), campaign="HIgS_August_2022", run_type=runType, quality=quality,
            beam_on=cells.get("M", "").upper() == "ON", beam_energy_MeV=toFloat(cells.get("N")),
            pressure_mbar=pressure, sampling_MHz=toFloat(cells.get("T")),
            drift_V=toFloat(cells.get("AB")), drift_preset=cells.get("Y", ""),
            trigger_delay_10ns=cells.get("U", ""),
            n_chunks_logbook=nChunksFromLogbook(cells.get("C")),
            comment=cells.get("AH", "")))
    return runs
################################################################
################################################################
def august2026Runs(path):
    """HIgS 2026 logbook, sheet HIgS_August: A start time (YYYYMMDD_hhmmss), C chunks, L beam,
    M gamma energy, U sampling rate, V trigger delay, Y Multi-GRAW mode, Z HV preset, AD drift voltage,
    AF pressure (M2), AH temperature [C], AJ run type and comments."""
    runs = []
    for cells in readXLSX(path, "HIgS_August"):
        match = re.match(r"(\d{4})(\d{2})(\d{2})_(\d{2})(\d{2})(\d{2})$", cells.get("A", ""))
        if not match:
            continue
        runType, quality = classify(cells.get("AJ"))
        if runType is None:
            continue
        runId = "{}-{}-{}T{}:{}:{}".format(*match.groups())
        text = cells.get("AJ", "")
        runs.append(dict(
            run_id=runId, campaign="HIgS_August_2026", run_type=runType, quality=quality,
            beam_on=cells.get("L", "").upper() == "ON", beam_energy_MeV=toFloat(cells.get("M")),
            pressure_mbar=toFloat(cells.get("AF")), sampling_MHz=toFloat(cells.get("U")),
            drift_V=toFloat(cells.get("AD")), drift_preset=cells.get("Z", ""),
            trigger_delay_10ns=cells.get("V", ""),
            n_chunks_logbook=nChunksFromLogbook(cells.get("C")),
            temperature_C=toFloat(cells.get("AH")),
            comment=text[len(runType):].strip(" ;,()") if text.lower().startswith(runType.lower()) else text))
    return runs
################################################################
################################################################
def isCompleteChunk(files):
    """GRAW files of a chunk: one per AsAd (4), or one file for all AsAd boards (CoBo_...)."""
    files = files.split(",")
    return len(files) == 4 or (len(files) == 1 and "_AsAd" not in os.path.basename(files[0]))
################################################################
################################################################
def findData(run, dataDirs):
    """Return (directory, number of complete chunks) of the best data copy."""
    best = ("", 0)
    for pattern in dataDirs:
        for directory in sorted(glob.glob(pattern)):
            if not os.path.isdir(directory):
                continue
            chunks = getRunChunks(directory, run["run_id"])
            complete = sum(1 for files in chunks.values() if isCompleteChunk(files))
            if complete > best[1]:
                best = (directory, complete)
    return best
################################################################
################################################################
def toDataSet(run):
    """tpcWebGUI data set of a run; the logbook metadata in "run"."""
    chunks = getRunChunks(run["data_dir"], run["run_id"]) if run["data_dir"] else {}
    complete = [files for files in chunks.values() if isCompleteChunk(files)]
    run["n_streams"] = len(complete[0].split(",")) if complete else ""
    energy = run["beam_energy_MeV"]
    title = "{} {}".format(run["run_id"], run["run_type"])
    if run["run_type"] == "Physics" and energy:
        title += " {:g} MeV".format(energy)
    description = "{}, {} ({}), {:g} mbar, drift {:g} V, {:g} MHz".format(
        run["campaign"], run["run_type"], run["quality"], run["pressure_mbar"] or 0,
        run["drift_V"] or 0, run["sampling_MHz"] or 0)
    if run["comment"]:
        description += "; " + run["comment"]
    dataSet = {
        "name": title,
        "description": description,
        "run": {key: run.get(key, "") for key in COLUMNS + ["beam_on"]},
        "input": {"dataFile": complete[0] if complete else "",
                  "geometryFile": run["geometry_file"]},
        "conditions": {},
        "beamParameters": {},
    }
    if run["pressure_mbar"]:
        dataSet["conditions"]["pressure"] = run["pressure_mbar"]
    if run["sampling_MHz"]:
        dataSet["conditions"]["samplingRate"] = run["sampling_MHz"]
    if run.get("temperature_C") is not None:
        dataSet["conditions"]["temperature"] = round(run["temperature_C"] + 273.15, 2)
    if energy is not None:
        dataSet["beamParameters"]["energy"] = energy
    return dataSet
################################################################
################################################################
def mcSetupName(energy, pressure, driftV):
    return "{:g}MeV_{:g}mbar_{:g}V".format(energy, pressure, driftV)
################################################################
################################################################
def toMCDataSet(run, campaign):
    """MC data set for the setup of a (good Physics) run; None if no strip response sigmaZ for the pressure."""
    d = MC_DEFAULTS
    energy, pressure, driftV = run["beam_energy_MeV"], run["pressure_mbar"], run["drift_V"]
    sigmaZ = d["sigmaZByPressure"].get(int(round(pressure)))
    if sigmaZ is None:
        return None
    sigmaE = round(d["beamEnergySpread"]*energy, 4)
    name = mcSetupName(energy, pressure, driftV)
    return {
        "name": "MC " + name,
        "description": "HIgS {} MC, O16(gamma,alpha)C12, E_gamma Gaus({:g}, {:g}) MeV, {:g} mbar, drift {:g} V, {:g} MHz".format(
            campaign, energy, sigmaE, pressure, driftV, run["sampling_MHz"]),
        "run": {"run_id": "MC_" + name, "campaign": "HIgS_{}_MC".format(campaign),
                "run_type": "MC", "quality": "good",
                "beam_energy_MeV": energy, "pressure_mbar": pressure,
                "sampling_MHz": run["sampling_MHz"], "drift_V": driftV,
                "geometry_file": run["geometry_file"],
                "n_events": d["nEvents"], "n_jobs": d["nJobs"]},
        "input": {"geometryFile": run["geometry_file"],
                  "controllerConfigPath": d["controllerTemplate"],
                  "fromGenerator": True},
        "conditions": {"pressure": pressure, "samplingRate": run["sampling_MHz"]},
        "beamParameters": {"energy": energy},
        "controller": {"ModuleConfiguration": {
            "Generator": {"EventGenerator": {"Beam": {"GammaEnergy": {
                "distribution": "EProviderGaus",
                "parameters": {"meanE": energy, "sigmaE": sigmaE}}}}},
            "GeantSim": {"gas_mixture": {"co2": pressure/1000.0}},           # [bar]
            "ToyIonizationSimulator": {"Pressure": pressure/1000.0},        # [bar]
            "TPCDigitizerSRC": {"sigmaXY": d["sigmaXY"], "sigmaZ": sigmaZ}}},
    }
################################################################
################################################################
def writeMCDataSets(runs, campaign, output):
    """One MC data set per (energy, pressure, drift voltage) of the good Physics runs."""
    setups = {}
    for run in runs:
        if run["run_type"] != "Physics" or run["quality"] != "good" or not run["geometry_file"]:
            continue
        setups.setdefault((run["beam_energy_MeV"], run["pressure_mbar"], run["drift_V"]), run)
    os.makedirs(output, exist_ok=True)
    nWritten = 0
    for key in sorted(setups):
        dataSet = toMCDataSet(setups[key], campaign)
        if dataSet is None:
            print("WARNING: no strip response sigmaZ for {:g} mbar, no MC for setup {}".format(
                key[1], mcSetupName(*key)))
            continue
        with open(os.path.join(output, dataSet["run"]["run_id"] + ".json"), "w") as f:
            json.dump(dataSet, f, indent=2)
            f.write("\n")
        nWritten += 1
    print("Wrote {} MC setups to {}".format(nWritten, output))
################################################################
################################################################
def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--campaign", choices=sorted(CAMPAIGNS), default="2022")
    parser.add_argument("--logbook", help="logbook (2022: directory with the spreadsheets)")
    parser.add_argument("--output", help="directory for the run JSON files")
    parser.add_argument("--dataDir", action="append", help="glob of data directories (repeatable)")
    parser.add_argument("--mc", action="store_true", help="write the MC data sets (one per setup) instead of the runs")
    args = parser.parse_args()
    defaults = CAMPAIGNS[args.campaign]
    logbook = args.logbook or defaults["logbook"]
    outputName = "HIgS_{}_MC_runs".format(args.campaign) if args.mc else defaults["output"]
    args.output = args.output or os.path.join(os.path.dirname(os.path.abspath(__file__)), outputName)
    dataDirs = args.dataDir or defaults["dataDirs"]

    if args.campaign == "2022":
        runs = aprilRuns(os.path.join(logbook, "higs_april_exp_notes.ods"))
        runs += augustRuns(os.path.join(logbook, "higs_august_exp_notes.xlsx"))
    else:
        runs = august2026Runs(logbook)
    for run in runs:
        run.setdefault("temperature_C", None)

    problems = []
    for run in runs:
        if run["run_type"] == "Physics" and not run["beam_on"]:
            problems.append(run["run_id"] + ": Physics run with beam OFF")
        if run["run_type"] == "Background":
            run["beam_energy_MeV"] = 0.0
        # working point: pressure to 10 mbar (logged e.g. 109.5 or 110.9 at 110 mbar), drift voltage, sampling rate
        key = (int(round((run["pressure_mbar"] or 0)/10.0))*10, int(run["drift_V"] or 0), run["sampling_MHz"])
        run["geometry_file"] = GEOMETRY_FILES.get(key, "")
        if not run["geometry_file"]:
            problems.append(run["run_id"] + ": no geometry file for (p, Udrift, f) = " + str(key))
        if args.mc:
            continue
        run["data_dir"], run["n_chunks_found"] = findData(run, dataDirs)
        if run["n_chunks_logbook"] and run["n_chunks_found"] and run["n_chunks_logbook"] != run["n_chunks_found"]:
            problems.append("{}: logbook lists {} chunks, found {} complete".format(
                run["run_id"], run["n_chunks_logbook"], run["n_chunks_found"]))

    if args.mc:
        writeMCDataSets(runs, args.campaign, args.output)
        return

    os.makedirs(args.output, exist_ok=True)
    for oldFile in glob.glob(os.path.join(args.output, "*.json")):  # runs removed from the logbook
        os.remove(oldFile)
    for run in runs:
        fileName = os.path.join(args.output, run["run_id"].replace(":", "-") + ".json")
        with open(fileName, "w") as f:
            json.dump(toDataSet(run), f, indent=2)
            f.write("\n")

    nPhysics = sum(r["run_type"] == "Physics" for r in runs)
    nMissing = sum(r["run_type"] == "Physics" and not r["data_dir"] for r in runs)
    print("Wrote {} runs ({} Physics, {} of them without data on disk) to {}".format(
        len(runs), nPhysics, nMissing, args.output))
    for problem in problems:
        print("WARNING:", problem)
################################################################
################################################################
if __name__ == "__main__":
    main()
