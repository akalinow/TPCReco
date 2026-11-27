# Analysis module

Programs, batch drivers and ROOT macros for the analysis of reconstructed and raw TPC data,
mainly the HIgS 2022 (and 2026) O16(γ,α)C12 measurements.

All commands below are run inside the TPCReco container, from the `resources` directory
of an installed build (`bin/`, `python/`, `config/`, `examples/` are its siblings).
Run `make install` after building — the programs in `bin/` are the installed copies.

```Bash
cd <build>/resources
```

## Contents

| Directory   | Content |
|-------------|---------|
| `bin/`      | sources of the command line programs (table below) |
| `python/`   | batch drivers: `makeRunList.py`, `runBatchJobs.py` (data and MC) |
| `python/HIgS_2022_runs/`, `python/HIgS_2026_runs/` | per-run data sets (one JSON file per run), made by `makeRunList.py` |
| `python/HIgS_2022_MC_runs/` | MC setups (one JSON file per setup), made by `makeRunList.py --mc` |
| `config/`   | configs for `recoEventsAnalysis`, `rawPedestalAnalysis`, cuts |
| `examples/` | ROOT macros for plotting the program outputs |
| `src/`, `include/` | analysis library (`HIGGS_analysis`, `TrackDiffusion_tree_analysis`, `Comp_analysis`, cuts, ...) |

### Programs

| Program | Input | Output |
|---------|-------|--------|
| `makeTrackTree` | GRAW / ROOT raw data | `TrackTree_<run>_<chunk>.root`: `trackTree` ntuple; with `--output.recoTree true` also the `TPCRecoData` tree (Track3D + EventInfo) |
| `makeMCTrackTree` | MC generator (`EventSourceMC`) | `MCTrackTree_*.root`: generated and reconstructed quantities per event |
| `recoEventsAnalysis` | `TPCRecoData` tree | `Histos.root`, `Trees.root` (HIgS analysis: cuts, beam-frame kinematics) |
| `rawTrackDiffusionAnalysis` | raw data + reco file (`TPCRecoData` or plain `trackTree`) | tree with transverse hit MEAN/RMS per U/V/W projection; optional event display dump |
| `rawSignalAnalysis`, `rawPedestalAnalysis` | raw data | signal / pedestal histograms |
| `recoEventsClean`, `recoEventsDiff`, `recoEventsComparison` | reco files | event selection by id, comparison of two reconstructions |
| `recoEnergyScaleFitter` | reco events of calibration runs | length / energy scale and offset corrections (`--type length\|energy_cms\|energy_lab\|zenek`) |

Most programs take a JSON config as the first argument, and every config entry can be
overridden on the command line, e.g. `--input.readNEvents 1000`. The raw-data programs
(`rawTrackDiffusionAnalysis`, `rawSignalAnalysis`, `rawPedestalAnalysis`) and `recoEnergyScaleFitter`
have their own short options (`--dataFile`, `--geometryFile`, ...). Use `--help` for the options.

## Quick start

Reconstruct 1000 MC events and plot the result:

```Bash
../bin/makeMCTrackTree ../config/config_GUI_MC.json --input.readNEvents=1000
root -l '../examples/makeMCPlots.cpp("MCTrackTree_<...>.root")'
```

Write the full `PEventTPC` data and a track tree for 1000 MC events:

```Bash
../bin/tpcWriter ../config/config_Writer_MC.json --input.readNEvents=1000
```

The output file names contain the run timestamp and the processing date.

## HIgS batch processing

### 1. Run list

The batch driver does not scan directories: it reads per-run data sets made from the experiment
logbooks by [makeRunList.py](python/makeRunList.py):

```Bash
../python/makeRunList.py --campaign 2022        # -> python/HIgS_2022_runs/
../python/makeRunList.py --campaign 2026        # -> python/HIgS_2026_runs/
../python/makeRunList.py --campaign 2022 --mc   # -> python/HIgS_2022_MC_runs/
```

Each `<run_id>.json` file holds the logbook metadata (`run` section: campaign, run type, quality,
beam energy, pressure, drift voltage, sampling rate, matching geometry file, data directory,
number of complete chunks found) plus `input`/`conditions`/`beamParameters` sections, so the same
file is also a data set in the tpcWebGUI Open dialog. The logbook paths, data directories and the
(pressure, drift voltage, sampling) → geometry file map are set in `CAMPAIGNS` and `GEOMETRY_FILES`
at the top of the script. Warnings are printed for Physics runs with beam OFF, missing geometry
files and chunk counts that differ from the logbook.

Regenerate the run list after adding data on disk or editing a logbook.

**MC setups.** With `--mc` the script writes one MC data set per setup (beam energy, pressure,
drift voltage) of the good Physics runs, `MC_<E>MeV_<p>mbar_<U>V.json`, with the same sections
and `run_type: "MC"`:

```json
"run":        {"run_id": "MC_9.85MeV_130mbar_1764V", "campaign": "HIgS_2022_MC", "run_type": "MC",
               "beam_energy_MeV": 9.85, "pressure_mbar": 130.0, ..., "n_events": 16000, "n_jobs": 4},
"input":      {"geometryFile": "...", "controllerConfigPath": "montecarlo_2prong_gun_PEventTPC.json",
               "fromGenerator": true},
"controller": {"ModuleConfiguration": {
                 "Generator": {"EventGenerator": {"Beam": {"GammaEnergy": {"distribution": "EProviderGaus",
                                                   "parameters": {"meanE": 9.85, "sigmaE": 0.197}}}}},
                 "GeantSim": {"gas_mixture": {"co2": 0.13}},
                 "ToyIonizationSimulator": {"Pressure": 0.13},
                 "TPCDigitizerSRC": {"sigmaXY": 1.0, "sigmaZ": 1.2}}}
```

The `controller` section is merged onto the controller template from `config/`
(dicts recursively, lists element by element), so any generator or simulation setting can be
changed per setup, e.g. the angular distributions in `Reactions`. The defaults (events, jobs,
beam energy spread, strip response σ per pressure) are in `MC_DEFAULTS` of `makeRunList.py`.
`--mc` overwrites the files of the generated setups and keeps other files in the directory,
so hand-made MC data sets can live there too.

### 2. Processing: `runBatchJobs.py`

Edit the settings at the end of [runBatchJobs.py](python/runBatchJobs.py)
(the comment there lists all good Physics setups with their run and chunk counts):

```python
dataPathPrefix    = "/scratch_hdd/akalinow/ELITPC/data"   # GRAW files in <prefix>/<dataDirName>/<day dir>
dataDirName       = "HIgs_2022"
mcOutputDir       = "/scratch_hdd/akalinow/ELITPC/WarsawTPC_Claude/data/HIgS_2022_MC"
stripResponsePath = ".../MonteCarlo/Modules/StripResponseModel/"   # StripResponseModel_*.root for the MC sigmas
selectedRunTypes  = ["Physics"]   # "Physics" (data), "MC"
selectedEnergies  = []        # [MeV]
selectedCampaigns = []        # "HIgS_April_2022", "HIgS_August_2022", "HIgS_2022_MC"
selectedPressures = []        # [mbar]
selectedRunIds    = ["2022-08-22T22:03:11"]   # run_id prefixes, e.g. "MC_9.85MeV"
firstRunPerSetup  = False     # data: only the first run of each (E, campaign, p, Udrift) setup
maxChunksPerRun   = -1        # data: -1 = all
maxChunks         = -1        # data: total limit; a chunk takes ~40 min
mcEventsPerSetup  = -1        # MC: -1 = n_events of the data set
procCount         = 14        # parallel jobs (makeTrackTree + makeMCTrackTree)
```

An empty list means no selection on that field. Only runs of `good` quality are taken.
Energies 8.86, 9.85 and 13.1 MeV were measured in more than one setup — select the campaign or pressure.
With `selectedRunTypes = ["Physics", "MC"]` data and the matching MC are processed in one go,
e.g. `selectedEnergies = [9.85]`.

```Bash
../python/runBatchJobs.py
```

All jobs share one queue of `procCount` slots. The script returns when all jobs have finished
and their outputs are collected.

**Data.** One `makeTrackTree` job is started per chunk (4 AsAd GRAW files),
with geometry, pressure, sampling rate and beam energy taken from the run list.
The run list gives the day directory of each run (e.g. `20220822_extTrg_CO2_130mbar`);
the data are read from that directory under `dataPathPrefix/dataDirName`, wherever the run list says they are.
The outputs are collected in `HIgS_2022/<E>MeV/`:

```
HIgS_2022/9.85MeV/
  2022-08-22T22-03-11/          TrackTree_*.root, <chunk>.out logs, configDump.json
  2022-08-22T22-03-11.root      hadd of the run's TrackTree files
  plots/2022-08-22T22-03-11/    Histos.root, Trees.root, *.pdf
  git_version.dat
```

An existing run directory in the destination is overwritten.

The finalisation runs `recoEventsAnalysis` and `makePlots_HIGS.cpp` on the merged file.
`recoEventsAnalysis` needs the `TPCRecoData` tree, so the batch jobs run `makeTrackTree`
with `--output.recoTree true` (set in `Utilities/python/fileLoop.py`; off by default when
`makeTrackTree` is run by hand).

**MC.** `n_jobs` `makeMCTrackTree` jobs per setup (independent random seeds, highest pressure first)
generate and reconstruct O16(γ,α)C12 events in one pass (`input.fromGenerator`, no raw event files).
Each job gets its controller config: the template with the data set's `controller` section, the
absolute geometry path and `stripResponsePath`. Outputs:

```
<mcOutputDir>/9.85MeV_130mbar_1764V/job_<n>/   MC_*.root, job.out, controller.json, configDump.json
<mcOutputDir>/9.85MeV_130mbar_1764V.root       hadd of the jobs
<mcOutputDir>/git_version.dat
```

## Plotting macros (`examples/`)

| Macro | Input |
|-------|-------|
| `makePlots.cpp`, `makeCalibrationPlots.cpp` | `trackTree` (data, calibration) |
| `makeMCPlots.cpp` | `makeMCTrackTree` output |
| `makePlots_HIGS.cpp` | `Histos.root` from `recoEventsAnalysis`: `makePlots_HIGS("Histos.root", energyMeV)` |
| `makePlots_trackDiffusion.cxx` | `rawTrackDiffusionAnalysis` output: `makePlotsDiffusion(file)` |
| `dumpRawHistograms.cxx` | raw ROOT / GRAW file(s) |

```Bash
root -l -b -q '../examples/makePlots.cpp("HIgS_2022/9.85MeV/2022-08-22T22-03-11.root")'
```

## Track diffusion

```Bash
../bin/rawTrackDiffusionAnalysis config.json \
    --geometryFile <geometry.dat> --dataFile <GRAW files> --recoFile <TrackTree file> \
    --trackDistanceMM 10 --hitThreshold 0 --outputFile diffusion.root
```

`--recoFile` may be a `TPCRecoData` file or a plain `trackTree` ntuple (tracks rebuilt from the vertex
and the α/C end points). Event display dump and per-hit dump: JSON blocks `eventDump`
and `trackDiffusion.dumpHits`; see `--help`.
