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
| `python/`   | batch drivers: `makeRunList.py`, `makeTrackTree.py`, `makeMCTrackTree.py` |
| `python/HIgS_2022_runs/`, `python/HIgS_2026_runs/` | per-run data sets (one JSON file per run), made by `makeRunList.py` |
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

The batch drivers do not scan directories: they read per-run data sets made from the experiment
logbooks by [makeRunList.py](python/makeRunList.py):

```Bash
../python/makeRunList.py --campaign 2022     # -> python/HIgS_2022_runs/
../python/makeRunList.py --campaign 2026     # -> python/HIgS_2026_runs/
```

Each `<run_id>.json` file holds the logbook metadata (`run` section: campaign, run type, quality,
beam energy, pressure, drift voltage, sampling rate, matching geometry file, data directory,
number of complete chunks found) plus `input`/`conditions`/`beamParameters` sections, so the same
file is also a data set in the tpcWebGUI Open dialog. The logbook paths, data directories and the
(pressure, drift voltage, sampling) → geometry file map are set in `CAMPAIGNS` and `GEOMETRY_FILES`
at the top of the script. Warnings are printed for Physics runs with beam OFF, missing geometry
files and chunk counts that differ from the logbook.

Regenerate the run list after adding data on disk or editing a logbook.

### 2. Data: `makeTrackTree.py`

Edit the selection at the end of [makeTrackTree.py](python/makeTrackTree.py)
(the comment there lists all good Physics setups with their run and chunk counts):

```python
selectedEnergies  = []        # [MeV]
selectedCampaigns = []        # "HIgS_April_2022", "HIgS_August_2022"
selectedPressures = []        # [mbar]
selectedRunIds    = ["2022-08-22T22:03:11"]   # run_id prefixes
firstRunPerSetup  = False     # only the first run of each (E, campaign, p, Udrift) setup
maxChunksPerRun   = -1        # -1 = all
maxChunks         = -1        # total limit; a chunk takes ~40 min
procCount         = 14        # parallel jobs
```

An empty list means no selection on that field. Only `Physics` runs of `good` quality are taken.
Energies 8.86, 9.85 and 13.1 MeV were measured in more than one setup — select the campaign or pressure.

```Bash
../python/makeTrackTree.py
```

One `makeTrackTree` job is started per chunk (4 AsAd GRAW files, or one CoBo file),
with geometry, pressure, sampling rate and beam energy taken from the run list.
When all jobs finish, the outputs are collected in `HIgS_2022/<E>MeV/`:

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

### 3. Monte Carlo: `makeMCTrackTree.py`

[makeMCTrackTree.py](python/makeMCTrackTree.py) generates and reconstructs O16(γ,α)C12 events
for every good Physics setup (beam energy, pressure, drift voltage) of the run list, in one pass
(`input.fromGenerator`, no raw event files). For each setup the MC controller config
`montecarlo_2prong_gun_PEventTPC.json` is adapted: Gaussian gamma energy
(σ = `beamEnergySpread` × E), gas pressure, geometry and the strip-response model
(`TPCDigitizerSRC`, `sigmaXY`, `sigmaZByPressure`; the matching `StripResponseModel_*.root`
files must exist in `stripResponsePath`).

Settings at the top of the script: `selectedEnergies`, `selectedSetups`
(e.g. `"12.7MeV_190mbar_3626V"`), `nEventsPerSetup`, `nJobsPerSetup`, `maxProcs`
(shared with running `makeTrackTree` jobs) and `topDirName` (output location).

```Bash
../python/makeMCTrackTree.py
```

Output: `<topDirName>/<E>MeV_<p>mbar_<U>V/job_<n>/` and the merged `<setup>.root`.

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
