# Reconstruction

Automatic reconstruction of single-vertex events (α + recoil, single α, "dot") from the raw UVW
strip data: from GRAW samples to a fitted `Track3D` with α and carbon segments, the charge in MeV
and the dE/dx fit result. The main class is `TrackBuilder`. It is run by every application that
reconstructs, for example `makeTrackTree` (ntuples), `tpcGUI` (reconstruction mode) and `tpcWriter`.

The steps below are in processing order. For each step the table gives the code, the
configuration options, and the files it reads. Options are written as `group.name`.

## Configuration

* **Defaults and allowed options** are defined in
  [`Utilities/config/allowedOptions.json`](../Utilities/config/allowedOptions.json). `make install`
  copies this file to `~/.tpcreco/config/allowedOptions.json`, and the applications read that
  installed copy. An option that is missing from the installed copy is rejected on the command line.
* **Per-setup JSON files** set the working point (geometry, pressure, sampling, pedestal windows):
  `GUI/config/config_GUI__OFFLINE_<p>mbar_<U>Vdrift_<f>MHz[_NGRAW].json`, e.g.
  `config_GUI__OFFLINE_190mbar_3332Vdrift_25MHz_NGRAW.json`. `_NGRAW` = one GRAW file per AsAd.
* **Command line** arguments override the JSON files; several JSON files can be given. The full
  configuration used by a job is written to `configDump.json`.
* **File paths** (`input.geometryFile`, `input.gainMapFile`, `reconstruction.gainTableFile`,
  `reconstruction.tfModelPath`, ...) may be relative to the `resources` directory. They are looked up
  in the current directory, `$TPCRECO_RESOURCES`, the installed `resources` and the source-tree
  `resources` (`ConfigManager::resolveResourcePaths`).
* **HIgS 2022 batch processing**: `Analysis/python/runBatchJobs.py` takes the runs from
  `Analysis/python/HIgS_2022_runs/` (one data-set JSON per run, made from the logbooks by
  `makeRunList.py`; the same files are tpcWebGUI data sets) and starts one job
  per chunk (`Utilities/python/fileLoop.py`). Each job gets `--input.geometryFile`, `--input.dataFile`,
  `--conditions.pressure`, `--conditions.samplingRate`, `--beamParameters.energy`,
  `--eventFilter.enabled true` and `--eventFilter.maxChargeLowerBound 1000`.

## Processing steps

```
GRAW samples ──► 1 geometry, run conditions
             ──► 2 raw data: (strip, time cell) samples
             ──► 3 pedestal + FPN subtraction, signal time window
             ──► 4 event pre-filter (charge bounds, event list)
             ──► 5 hit filter (threshold + envelope)        ┐
             ──► 6 clean cluster per projection (U, V, W)   ┘ TrackBuilder::makeRecHits
             ──► 7 3D seed (ML model, or projection lengths and strip crossings)
             ──► 8 3D line fit (Minuit2) + shrink to hits
             ──► 9 gain map g(X_DET), per hit                 ─ only with input.gainMapFile
             ──► 10 dE/dx hypothesis fit (α / C+α, both orientations)  ─ tracks > 30 mm
             ──► 11 optional: other α–C orientation by R = Q/(G_raw E_dep)
             ──► 12 charge → MeV (gain table)
             ──► Track3D (makeTrackTree: trackTree ntuple)
```

| # | Step | Code | Options | Files |
|---|---|---|---|---|
| 1 | Geometry and run conditions | `GeometryTPC`, `EventSourceBase::loadGeometry` | `input.geometryFile`, `conditions.pressure`, `conditions.temperature`, `conditions.samplingRate`, `beamParameters.energy` | `resources/geometry_ELITPC_<p>mbar_<U>Vdrift_<f>MHz.dat` |
| 2 | Raw data | `EventSourceGRAW`, `EventSourceMultiGRAW`, `EventSourceROOT` | `input.dataFile`, `input.singleAsadGrawFile`, `input.frameLoadRange`, `input.readNEvents` | GRAW files, GET format files in `resources` |
| 3 | Pedestal and FPN | `PedestalCalculator`, `EventSourceGRAW::fillEventFromFrame` | `pedestal.remove`, `pedestal.minPedestalCell`, `pedestal.maxPedestalCell`, `pedestal.minSignalCell`, `pedestal.maxSignalCell` | – |
| 4 | Event pre-filter | `EventFilter`, `Filters` | `eventFilter.enabled`, `eventFilter.maxChargeLowerBound`, `eventFilter.maxChargeUpperBound`, `eventFilter.totalChargeLowerBound`, `eventFilter.totalChargeUpperBound`, `eventFilter.events`, `eventFilter.recoProngs` | – |
| 5 | Hit filter | `EventTPC::filterHits` (`filter_type::threshold`) | see the step notes | – |
| 6 | Clean cluster | `RecHitBuilder::makeCleanCluster` | `reconstruction.cleanClusterWindowTimeMM`, `reconstruction.cleanClusterWindowStripMM`, `reconstruction.cleanClusterThreshold`, `reconstruction.cleanClusterEmptyBinThreshold` | – |
| 7 | 3D seed | `TrackBuilder::buildSegment3D`, `MLTrackBuilder` | `reconstruction.tfModelPath`, `reconstruction.inputDim`, `reconstruction.outputDim` | TensorFlow model (optional) |
| 8 | 3D line fit | `TrackBuilder::fitTrack3D`, `Track3D`, `TrackSegment3D`, `TrackSegment2D` | hard-coded | – |
| 9 | Gain map | `GainMap`, `TrackSegment3D::getGainCorrection` | `input.gainMapFile` (default empty = off) | `resources/gainMap_HIgS2022_xDET.dat` |
| 10 | dE/dx fit | `TrackBuilder::fitEventHypothesis`, `dEdxFitter` | `reconstruction.dEdxFitAttempts`, `reconstruction.dEdxFitMaxLossRatio`, `reconstruction.dEdxPhysicsConstraint`, `reconstruction.dEdxCarbonRangeMargin` | `resources/dEdx_corr_alpha_10MeV_CO2_250mbar.dat`, `resources/dEdx_corr_12C_5MeV_CO2_250mbar.dat` |
| 11 | Orientation by R | `TrackBuilder::fitEventHypothesis`, `dEdxFitter::setOrientation` | `reconstruction.dEdxOrientationByR` (default off), `reconstruction.dEdxOrientationVertexWindow` | gain table |
| 12 | Charge → MeV | `GainTable`, `Track3D::getChargeEnergy` | `reconstruction.gainTableFile` (default `gainTable_HIgS2022.dat`) | `resources/gainTable_HIgS2022.dat` |

### 1. Geometry and run conditions

The geometry file defines:
* the strip map (electronics channel ↔ strip direction, section and number);
* the strip angles and the reference point;
* the drift velocity (Magboltz, per drift voltage and pressure);
* the sampling rate;
* the trigger delay;
* the drift cage acceptance.

The drift velocity and sampling rate in the geometry file set the time-cell → z conversion; the
`conditions.driftV` and `conditions.samplingRate` entries of the JSON files are informational.
`conditions.pressure` and `conditions.temperature` set the gas for the range–energy tables
(`IonRangeCalculator`, SRIM, CO2) and for the scaling of the dE/dx templates. There is one
geometry file per working point:

| Campaign | Geometry files |
|---|---|
| HIgS 2022, 130 mbar | `geometry_ELITPC_130mbar_{1372,1568,1764}Vdrift_25MHz.dat` |
| HIgS 2022, 190 mbar | `geometry_ELITPC_190mbar_{3332,3626,3920}Vdrift_25MHz.dat` |
| HIgS 2022, 250 mbar | `geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat` |

No drift-velocity correction is applied in the reconstruction. The correction factors measured in
the analysis are all compatible with 1.

### 2. Raw data

GRAW files are read by TPCReco's own reader (`GrawToROOT`); the GET software is not needed.
`input.dataFile` is a comma-separated list of the files of one chunk: 4 files, one per AsAd, with
`input.singleAsadGrawFile = true`. Each sample is stored by (strip direction, section, strip number,
time cell).

### 3. Pedestal and fixed-pattern noise

With `pedestal.remove = true`, each event is corrected from its own data:
* **Pedestal:** for each channel, the mean of the time cells
  [`minPedestalCell`, `maxPedestalCell`] (HIgS: 5–25).
* **FPN:** for each AGET chip and time cell, the average of its FPN channels, subtracted from every
  channel of that chip.

Only cells in [`minSignalCell`, `maxSignalCell`] (HIgS: 5–506) are kept. Cells 0–1 and 510–511
are always dropped. There is no shaping-time deconvolution and no channel gain calibration.

### 4. Event pre-filter

The filter is applied while reading, before any reconstruction (`EventSourceBase`). The HIgS batch
jobs use `maxChargeLowerBound = 1000`: the event is skipped if its largest sample is below 1000 ADC.
`events` and `recoProngs` select by event ID or by the number of prongs of a previous
reconstruction.

The charge bounds use the whole-event maximum, total charge and number of hits (`EventInfo`
properties). `EventTPC::SetEventInfo` computes them in one pass over the samples, summing the strip
sections, without building the 3D histograms; the histograms are built only when the reconstruction
or the GUI needs them. Events rejected by the filter are not reconstructed. `getRecoEvent()`
reconstructs on demand when it is called for such an event (e.g. by the `recoProngs` filter).

### 5. Hit filter

`TrackBuilder` uses the `threshold` filter. A sample is kept if it is above the threshold, and so
are its neighbours within ±`DeltaStrips` strips and ±`DeltaTimeCells` time cells ("envelope").

The settings are the `hitFilter.recoClusterThreshold`, `hitFilter.recoClusterDeltaStrips` and
`hitFilter.recoClusterDeltaTimeCells` options. `makeTrackTree` passes the configuration to the event at
the start of its event loop (`EventTPC::setHitFilterConfig`), as do the GUI (`HistoManager`) and
`MLTree`. A program without that call uses the defaults of `EventTPC`: 35 ADC, ±2 strips, ±5 time
cells (the same values as the HIgS JSON files). `hitFilter.recoClusterEnable` only switches the GUI
display; the reconstruction always uses the threshold filter.

### 6. Clean cluster

For each projection (U, V, W; time × strip, in mm), a cluster grows from the maximum bin. From
every bin of the cluster whose kernel window sum is at least `cleanClusterThreshold` (default 0.2)
times the maximum bin, all window bins with charge above `cleanClusterEmptyBinThreshold` (default
1 ADC) are added. This removes isolated noise and other, separate deposits. The cluster is used as
the hits of the track fits.

The window has half-widths `cleanClusterWindowTimeMM` (time axis) and `cleanClusterWindowStripMM`
(strip axis) in mm, converted to bins with the bin widths of the projection (n = round(half-width /
bin width)). The default, negative, is one bin on each axis: a 3 × 3 bin window, the historical
setting. Time and strip bins differ in size (e.g. 0.26 mm and 1.5 mm at 190 mbar and 25 MHz), so
the 3 × 3 bin window spans about 0.8 mm along the time axis and 4.5 mm along the strips; equal
half-widths in mm make the cut independent of the track orientation.

### 7. 3D seed

* **With a valid TensorFlow model** (`reconstruction.tfModelPath`), the seed is the bias and tangent
  predicted by the model. The model also gives a vertex constraint for step 10 (±2 mm).
* **Without a model** (the HIgS 2022 default; the log says "Error loading TensorFlow model from"):
  * Projection lengths and charge maxima are computed for each strip direction and for the time
    projection.
  * **Bias:** the mean of the strip crossing points of all pairs of directions at the charge
    maxima, weighted by the product of the projection lengths.
  * **Tangent:** from the signed projection lengths.
  * **Direction:** of the two tangents that give the same projections, the one with the larger
    integrated charge is kept.
  * **Dot:** a zero bias means a dot (all projections point-like). The track is then a
    zero-length `DOT` segment.

### 8. 3D line fit

Minuit2 fits the straight segment to the hits of the three projections in four passes:
1. tangent;
2. bias in the strip plane;
3. bias along z;
4. two small rotations about the fitted direction (`TANGENT_ROTATION`, no singularity along z).

The tangent fits use the parallel-line loss and the bias fits the hit-distance loss, with hits up
to 20 mm from the line. A pass is kept only if it lowers the loss by more than 1 %. Before each
pass the segment is extended to a 300 mm sphere. After the fit it is shrunk to the hits: the ends
are where the charge profile drops below 5 % of its maximum (`Track3D::shrinkToHits`).

### 9. Gain map (position-dependent gain)

The relative gain of the readout plane depends on X_DET: +2.5 % in the centre, −5 % at
|x| ≈ 150 mm (HIgS 2022, same in all setups).
* **When:** after the 3D fit, so the geometric fit is not corrected.
* **What:** each hit's charge is divided by g(x) at its 3D position on the fitted segment. This
  applies to the charge profile used by the dE/dx fit and to the integrated charge of the track.
* **Why not on the raw (strip × time) data:** the U strips run along x, so a sample's x is only
  known once the track is found.
* **Defaults:** off unless `input.gainMapFile` is set; ignored for Monte Carlo input.

File format: `x_lo x_hi g` per line, in mm, interpolated linearly between bin centres.
Measurement: `WarsawTPC_Claude/code/gain_x_map.py`. The table in Step 12 was measured with the map
on, so use both together.

### 10. dE/dx hypothesis fit

The fit runs for tracks longer than 30 mm (`minTkLenghtWithHypothesis`).
* **Charge profile:** the hits within 4 mm of each projection are projected onto the 3D segment.
  Projections shorter than 30 mm are skipped.
* **Models:**
  * α only;
  * recoil + α: the vertex position, α range and recoil range are free.
  * Both use the Bragg curves smeared with a Gaussian (effective diffusion). The curves are SRIM
    tables at 250 mbar, scaled to `conditions.pressure`.
  * The diffusion range is 0.5–1.5 × (projection width / 0.76).
* **Orientations:** each model is fitted in both orientations of the profile, and the hypothesis
  and orientation with the lowest loss are kept.
* **Result:** an α segment from the vertex and, for the two-prong hypothesis, a recoil segment
  (`CARBON_12`, or `CARBON_14` with O18 kinematics).

Options:
* `dEdxFitAttempts` (default 3): restarts with different initial step sizes.
* `dEdxFitMaxLossRatio` (default 1·10⁻⁵): the restarts stop at the first valid fit with
  loss / Q² ≤ this value, where Q is the integral of the profile normalised to its maximum.
  * The old value, 5·10⁻⁴, accepted the wrong minimum (loss ≈ 0.7 for an 80 mm track) on the
    first attempt.
  * On 150 events at 11.5 MeV: 19.5 % bad fits with 3 attempts and the old threshold; 18.0 % with
    3 attempts and the new one; 12.9 % with 10 attempts (+30 % CPU).
* `dEdxPhysicsConstraint` (default off): the recoil range is also fitted with the O16 → C12 + α
  and O18 → C14 + α kinematics, within ±`dEdxCarbonRangeMargin`, and the best of the three is kept.

Known issue: in a bad fit (`dEdxFitLoss` ≈ 0.6–0.8 instead of ≈ 0.01), the vertex sits about 10 mm
from the α end and the α Bragg peak is taken as the carbon. The restarts all begin from the same
initial parameters, so they do not escape this minimum. Select good fits with `dEdxFitLoss < 0.1`.

### 11. Other α–C orientation by R (optional, default off)

With `reconstruction.dEdxOrientationByR = true` and a gain-table entry for the run:
1. The profile is refitted in the other orientation, with the vertex seeded one carbon range from
   the current α end (window ±`dEdxOrientationVertexWindow`, default 6 mm).
2. The fit with |R − 1| smaller is kept, where R = Q / (G_raw E_dep) and
   E_dep = T_α(L_α) + T_C(L_C) from the fitted ranges.

Both fits are written to the log ("dE/dx orientation by R").

**Off by default.** On 150 events at 11.5 MeV it raised the bad fits from 12.8 % to 30.8 %. The
refit adjusts its ranges until E_dep matches the charge, so R does not separate the orientations.
R is still computed when a gain-table entry exists (`chargeRatio` leaf).

### 12. Charge → MeV (gain table)

The gain table gives, per setup:
* `G_cal`: ADC per MeV of calibrated total kinetic energy. It gives
  `Track3D::getChargeEnergy() = Q / G_cal` (leaf `chargeEnergy`).
* `G_raw`: ADC per MeV of the SRIM energy of the reconstructed lengths. It is used for R.

Rows are `run_first run_last pressure_mbar G_cal G_raw`, matched by run ID (YYYYMMDDhhmmss, from
the GRAW file name) and pressure (±5 mbar). With no matching row (Monte Carlo, other campaigns)
there is no conversion: `chargeEnergy = 0`.

The HIgS 2022 table (`resources/gainTable_HIgS2022.dat`) has one row per campaign × pressure; the
drift voltage does not change the gain. It is produced by `WarsawTPC_Claude/code/4_length_charge.ipynb`
(section "Gain table for TPCReco") with the gain map on. At 11.5 MeV the median `chargeEnergy` is
4.36 MeV, against E_γ + Q = 4.34 MeV.

## Output (`makeTrackTree`)

One entry per event with a track; the `trackTree` leaves are listed in
`WarsawTPC_Claude/analysis/08_ntuples.md`.
* **Charges:** `charge` is the integrated track charge in ADC, gain-map corrected if the map is on.
  `chargeEnergy` is in MeV and `chargeRatio` is R.
* **Energies:** `alphaEnergy` and `carbonEnergy` come from the fitted ranges (`IonRangeCalculator`).
* **Angles:** `cosTheta` and `phi` are in the nominal BEAM frame (beam along −X_DET).
  `cosThetaDET` and `phiDET` are in the DET frame.

## Not part of the reconstruction

These corrections are applied in the analysis, not here:
* beam tilt and offset (`CoordinateConverter`, `recoEventsAnalysis`: `recoAnalysis.beamOffset`,
  `beamSlope`, `beamDir`);
* drift-velocity factors;
* energy calibration of T_α;
* the LAB → CM boost;
* acceptance.

## Hard-coded parameters

These are candidates for configuration options:

| Parameter | Value | Where |
|---|---|---|
| hit-filter defaults (when the `hitFilter.*` options are not passed to the event) | 35 ADC, ±2 strips, ±5 time cells | `EventTPC` constructor |
| max. hit distance in the line fit | 20 mm | `TrackSegment2D` losses |
| fit sphere radius | 300 mm | `TrackBuilder::fitTrack3DInSelectedDir` |
| shrink-to-hits threshold | 5 % of the profile maximum | `Track3D::shrinkToHits` |
| min. track length for the dE/dx fit | 30 mm | `TrackBuilder::minTkLenghtWithHypothesis` |
| profile hit radius, min. projection length | 4 mm, 30 mm | `TrackSegment3D::getChargeProfile` |
| integrated-charge hit radius | 10 mm | `TrackSegment2D::getIntegratedCharge` |
| diffusion range | 0.5–1.5 × width / 0.76 | `TrackBuilder::fitEventHypothesis` |
| ML vertex constraint | ±2 mm | `dEdxFitter::setVertexConstraint` |

## Tests

* `Reconstruction/test/dEdxFitter_tst`: dE/dx templates and models, fit determinism, both
  orientations, physics constraint.
* `DataFormats/test/Track3D_tst`: line fit parametrisations, charge profile of a uniform track.
* `DataFormats/test/GainMap_tst`: gain map file, per-hit correction on a simulated track.
* `DataFormats/test/GainTable_tst`: gain table file and lookup.

Run them from the build directory: `export LC_ALL=$LANG; ctest`.
