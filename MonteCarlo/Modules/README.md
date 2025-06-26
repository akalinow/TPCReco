# Concept

The key concept of the framework is to encapsulate each functional block of the simulation into `Module`. Modules are
run sequentially and can pass information to the ones after them in a sequence (for example result of the simulation can
be written to a file by the exporter module).

# Modules

All modules inherit from an abstract class [fwk::VModule](../UtilsMC/include/TPCReco/VModule.h). Each module has to
implement `VModule`'s pure virtual methods:
* `EResultFlag Init(boost::property_tree::ptree config)` - initialize module, called at the beginning of the simulation,
  takes BOOST propterty tree configuration as an argument
* `EResultFlag Process(ModuleExchangeSpace &event)` - main *player*, called for every event, does the module's work, can read from-
  and write to the `ModuleExchangeSpace`, which is used for inter-module communication
* `EResultFlag Finish()` - finish the run, called for each module at the end of the simulation, used for cleanup.

## Available modules

* [Generator](Generator) - Wrapper for [EventGenerator](../EventGenerator/README.md) for generating `SimEvent`s.
* [EventFileExporter](EventFileExporter) - Writes simulation results into ROOT files
* [GeantSim](GeantSim) - Handles Geant4 simulation of detector response - it takes `SimEvent` and tracks primary
  particles through the detector
* [ToyIonizationSimulator](ToyIonizationSimulator) - Simple ionization simulator based on `IonRangeCalculator`
* [TriggerSimulator](TriggerSimulator) - Simulates self-triggering of the TPC by finding `z` position of first energy
  deposit that reaches the readout plane and shifts the whole event appropriately
* [TrackTruncator](TrackTruncator) - Truncates `SimTrack`s in a `SimEvent` to active volume of the detector and possibly
  to GET electronics range
* [Track3DBuilder](Track3DBuilder) - Builds `Track3D` objects from `SimEvent` objects - they are required by the
  existing legacy code used for comparison between pure and reconstructed Monte Carlo
* [TPCDigitizerRandom](TPCDigitizerRandom) - TPC digitizer based on Artur's approach for UVW projection. Each deposit is
  smeared with a 3D Gaussian function by sampling with configurable number of points
* [TPCDigitizerSRC](TPCDigitizerSRC) - TPC digitizer based on Mikolaj's `StripResponseCalculator`. It reads generated
  strip response histograms from a ROOT file that corresponds to the chosen TPC working conditions (including:
  drift velocity, electronics sampling rate and peaking time, "effective" Gaussian diffusion). More strip response
  ROOT files can be generated using ROOT macro [testStripResponseCalculator](../../Reconstruction/examples/testStripResponseCalculator.cxx).

`RunController` class creates all the modules, initializes them (Init method), runs Process method in the right order, and then cleans up with Finish method.

## ModuleExchangeSpace

The modules communicate with each-other through [ModuleExchangeSpace](../UtilsMC/include/TPCReco/ModuleExchangeSpace.h).
It contains:
* `SimEvent`
* `PEventTPC`
* `Track3D`
* `eventraw::EventInfo`.

`RunController` class keeps one instance of `ModuleExchangeSpace` and passes it by reference to the modules' `Process`
methods, that way the modules have read/write access.

# Configuration

Configuration template:

```json
{
  "EnableTiming": {},
  "ModuleSequence": [
    "ModuleA",
    "ModuleB",
    "ModuleC"
  ],
  "GeometryConfig": {},
  "ModuleConfiguration": {
    "ModuleA": {},
    "ModuleB": {},
    "ModuleC": {}
  }
}
```

where:

* `"EnableTiming"` - `bool`, flag enabling timing benchmark of the sequence
* `"ModuleSequence"` - vector of `string`, sequence of modules to be run in the same order,
  here `"ModuleA"`, `"ModuleB"` and "`"ModuleC"`"
* `"GeometryConfig"` - `string`, path to `geometry_ELITPC.dat` configuration
* `"ModuleConfiguration"` - configuration of modules, each module receives a `JSON` object parsed
  into `boost::property_tree::ptree` object as an argument to `ModuleName::Init` method

**Note**: When module is enabled in `"ModuleSequnce"` list but does not have any configuration parameters an empty JSON object for that module has to be provided anyway in `"ModuleSequenceConfiguration"` list.

**Note**: JSON configuration files for Monte Carlo modules allow to use simple math expressions for scalar and vector parameters of type: `int`, `unsigned int`, `float`, `double` and `bool`, provided that math expressions are enclosed in quotes (e.g. `"TMath::Pi()"`, `"M_PI"`, `"!true"`).


## Correct sequence of modules

The correct order of modules in the `"ModuleSequence"` vector is important, because
some of the modules are mandatory, while others are optional or mutually exclusive:
- MANDATORY: [Generator](Generator) - Should be the very first module.
- Optional: [GeantSim](GeantSim) - Required for `Track3D`s and raw signals.
Place after `Generator`. Mutually exclusive with `ToyIonizationSilmulator`.
- Optional: [ToyIonizationSimulator](ToyIonizationSimulator) - Required for `Track3D`s and raw signals. Place after `Generator`. Mutually exclusive with `GeantSim`.
- Optional: [TriggerSimulator](TriggerSimulator) - Required for truncated `Track3D`s/`SimEvent`s and delayed raw signals. Place after `GeantSim`/`ToyIonizationSimulator`.
- Optional: [TrackTruncator](TrackTruncator) - Required for truncated `Track3D`s/`SimEvent`s. Place after `TriggerSimulator` (if present) or after `TPCDigitizerRandom`/`TPCDigitizerSRC` (otherwise).
- Optional: [TPCDigitizerRandom](TPCDigitizerRandom) - Required for raw signals. Place after `TriggerSimulator` (if present) or after `GeantSim`/`ToyIonizationSimulator` (otherwise). Mutually exclusive with `TPCDigitizerSRC`.
- Optional: [TPCDigitizerSRC](TPCDigitizerSRC) - Required for raw signals. Place after `TriggerSimulator` (if present) or after `GeantSim`/`ToyIonizationSimulator` (otherwise). Mutually exclusive with `TPCDigitizerRandom`.
- Optional: [Track3DBuilder](Track3DBuilder) - Required for `Track3D`s. Place after `TPCDigitizer[*]` (if present), else after `TrackTruncator`/`TriggerSimulator` (if present), else after `GeantSim`/`ToyIonizationSimulator`. One of `TPCDigitizer[*]` modules must be present in order to populate `RecHit2D` deposits per track in `Track3D` collections.
- MANDATORY: [EventFileExporter](EventFileExporter) - Should be the very last module.

Typical module sequences are shown below:

**Example 1** - Generate only `SimEvent`s and store them to ROOT file.
```json
  "ModuleSequence": [
    "Generator",
    "EventFileExporter"
  ]
```

**Example 2** - Get ionization losses from SRIM, do not truncate tracks due to fiducial volume and electronics, store `SimEvent`s and `Track3D`s to ROOT file:
```json
  "ModuleSequence": [
    "Generator",
    "ToyIonizationSimulator",
    "Track3DBuilder",
    "EventFileExporter"
  ]
```
Since TPC signal digitizer module is absent, the charge deposits (after diffusion & electronics effects) in both `Track3D`s and `PEventTPC` branches will not be populated. Only `SimHits` deposits will be available (without diffusion & electronics effects).

**Example 3** - Get ionization losses from Geant4, truncate tracks due to fiducial volume and electronics, store `SimEvent`s, `Track3D`s as well as simulated raw data for `PEventTPC` in a single ROOT file:
```json
  "ModuleSequence": [
    "Generator",
    "GeantSim",
    "TriggerSimulator",
    "TrackTruncator",
    "TPCDigitizerSRC",
    "Track3DBuilder",
    "EventFileExporter"
  ]
```
For this full-chain configuration, provided that no branches are suppressed in `EventFileExporter` module, the typical performance of generating 2-prong (alpha+<sup>12</sup>C) event samples is:
* generation speed: 0.95 s / evt / CPU core
* disk storage: 213 kB / evt.

**Note**: In order to avoid creating a very large ROOT output file it is advised to **split** generation of Monte Carlo data samples into several chunks of no more than **10k events** each.


# Configuration of individual modules

## EventFileExporter

Configuration template:

```json
{
  "FileName": {},
  "EnabledBranches": [
    {},
    {}
  ],
  "DisabledBranches": [
    {},
    {}
  ]
}
```

where:

* `"FileName"` - `string`, name of the output file
* `"EnabledBranches"` - vector of `string` describing branches to be saved into file, available branches:
  * `"SimEvent"` - pure Monte Carlo data (`SimEvent`)
  * `"PEventTPC"` - *raw* data (`PEventTPC`), requires one of `TPCDigitizer[*]` modules. 
    **Note**: Saving digitized `PEventTPC` charge maps to the output ROOT file is disabled to optimize disk space.
    If needed, it can be re-enabled by editing `"EventFileExporter"` part of JSON file.
  * `"Track3D"` - *'reconstructed'* data (`Track3D`), requires `Track3DBuilder` module
* `"DisabledBranches"` - vector of `string` describing branches to be excluded from saving into file.
  Branches can belong to two ROOT `TTree`'s stored into the output file (`"TPCData"` and `"TPCRecoData"`).
  List of all possibilities can be found by running `TTree::Print()` on the given tree. Branches occupying most space:
  * `"TPCData.tracks.hits"` - `std::vector` with all energy deposits (`SimHits`) generated by Geant4/ToyMC
  * `"TPCData.myChargeMap"` - `std::map` with all digitized charges needed for creating `PEventTPC`
  * `"TPCData.myChargeArray[3][3][256][512]"` - C-style array for ML purposes that holds the *same* information as `std::map` from the previous point (`"TPCData.myChargeArray*"` also works).
  * `"TPCRecoData.mySegments.myRecHits"` - `std::vector` with digitized deposits (`RecHit2D`) associated with individual generator level tracks (`TrackSegment3D`)

## GeantSim

Configuration template:

```json
{
  "EnableAlphaStraggling": true,
  "Temperature": 293.15,
  "gas_mixture": {
    "co2": 0.25,
    "he": 0.0
  },
  "magnetic_field": {
    "magnetic_field_ON": false,
    "magnetic_field_map": "/scratch/MonteCarloSimulations/Geant4/PurgMag3D.TABLE",
    "magnetic_field_offset": 200
  },
  "GeometryConfig": {
    "UseMaterials": false,
    "ModelPath": "/scratch/MonteCarloSimulations/Geant4/GEANT_elitpc_model_20180302/STL/",
    "MaterialColors": {
      "aluminium": {
        "r": 132,
        "g": 135,
        "b": 137,
        "alpha": 1
      },
      "peek": {
        "r": 222,
        "g": 184,
        "b": 135,
        "alpha": 1
      },
      "kapton": {
        "r": 236,
        "g": 206,
        "b": 106,
        "alpha": 1
      },
      "copper": {
        "r": 184,
        "g": 115,
        "b": 51,
        "alpha": 1
      },
      "stainless": {
        "r": 132,
        "g": 135,
        "b": 137,
        "alpha": 0.2
      },
      "FR4": {
        "r": 4,
        "g": 179,
        "b": 109,
        "alpha": 1
      }
    },
    "Solids": {
      "kapton": [
        "*gem_kapton*",
        "*ISO_KF40_window_kapton*"
      ],
      "stainless": [
        "*drift_conn*",
        "*M2*",
        "*pcb_compression_plate*",
        "*vessel_collar*",
        "*vessel_barrel_wall*",
        "*ISO_KF40_weld_flange*",
        "*vessel_endcap1*",
        "*vessel_endcap2*",
        "*ISO_KF40_full_nipple_130mm*",
        "*ISO_KF40_window_flange_35mm*",
        "ISO_KF40_centering_ring_modified*",
        "*PFEIFFER_170SFK040-06"
      ],
      "copper": [
        "*gem_cu_layer1*",
        "*gem_cu_layer2*",
        "*pcb_gnd_top*",
        "*pcb_gnd_bottom*"
      ],
      "aluminium": [
        "*drift_strip1*",
        "*drift_strip2*",
        "*cathode_plate*",
        "*ISO_KF40_clamp*"
      ],
      "peek": [
        "*gem_frame*",
        "*drift_rod*",
        "*drift_screw*",
        "*drift_spacer*",
        "*drift_spacer2*",
        "*drift_frame1*",
        "*drift_frame2*"
      ],
      "FR4": [
        "*pcb_readout_core*"
      ]
    }
  }
}
```

where:

* `"EnableAlphaStraggling"` - `bool`, switch to enable multiple scattering for <sup>4</sup>He/<sup>3</sup>He particles
* `"Temperature"` - `float`, gas temperature in K
* `"gas_mixture"` - describes partial pressures of mixture components:
  * `"p_co2"` - `float`, CO<sub>2</sub> pressure in bar
  * `"p_he"` - `float`, He pressure in bar
* `"magnetic_field"` - configuration of the magnetic field of the purging magnet (optional):
  * `"magnetic_field_ON"` - `bool`, magnetic field ON(`true`) or OFF(`false`)
  * `"magnetic_field_map"` - `"string"`, path to magnetic field table
  * `"magnetic_field_offset"` - `float`, offset along beam axis in mm
* `"GeometryConfig"` - configuration of the geometry:
  * `"UseMaterials"` - `bool`, flag to use STL files for geometry, if `false` then a simple cube of **1m x 1m x 1m** volume filled with the specified gas mixture is created. The initialization will be much shorter, and the code will run a little bit faster (about 10%), but the tracks originated outside of the active TPC volume will
not be obscured by the realistic drift cage and vacuum vessel geometry!
  * `"ModelPath"` - `string`, path to the directory with STL files
  * `"MaterialColors"` - definition of material colors, as in the config above
  * `"Solids"` - definition of different solids for each material, wildcards can be used.


The code will run a little bit faster (about 10%), but the tracks originated outside of the active TPC volume will
not be obscured by the realistic drift cage and vacuum vessel geometry!

## Generator

Configuration template:

```json
{
  "EventGenerator": {}
}
```

where:

* `"EventGenerator"` - `JSON`, configuration of `EventGenerator`, as described [here](../EventGenerator/README.md)

## ToyIonizationSimulator

Configuration template:

```json
{
  "Temperature": {},
  "Pressure": {},
  "PointsPerMm": {}
}
```

where:

* `"Temperature"` - `float`, gas temperature in K
* `"Pressure"` - `float`, gas pressure in bar
* `"PointsPerMm"` - `float`, number of `SimHit` energy deposits per milimeter to be simulated

## TPCDigitizerRandom

Configuration template:

```json
{
  "GeometryConfig": {},
  "sigmaXY": {},
  "sigmaZ": {},
  "NSamplesPerHit": {},
  "MeVToChargeScale": {}
}
```

where:

* `"GeometryConfig"` - `string`, path to `geometry_ELITPC.dat` configuration
* `"sigmaXY"` - `float`, sigma for diffusion in XY_DET plane perpendicular to drift direction in millimiters
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction Z_DET in millimiters
* `"NSamplesPerHit"` - `int`, number of random samples around each `SimHit` location
* `"MeVToChargeScale"` - `float`, number of ADC counts per MeV

## TPCDigitizerSRC

Configuration template:

```json
{
  "GeometryConfig": {},
  "StripResponsePath": {},
  "sigmaXY": {},
  "sigmaZ": {},
  "MeVToChargeScale": {},
  "th2PolyPartitionX": {},
  "th2PolyPartitionY": {},
  "peakingTime": {},
  "nStrips": {},
  "nCells": {},
  "nPads": {}
}
```

where:

* `"GeometryConfig"` - `string`, path to `geometry_ELITPC.dat` configuration
* `"StripResponsePath"` - `string`, path to directory where strip responses are stored
* `"sigmaXY"` - `float`, sigma for diffusion in XY_DET plane perpendicular to drift direction in millimiters
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction Z_DET in millimiters
* `"MeVToChargeScale"` - `float`, number of ADC counts per MeV
* `"th2PolyPartitionX"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, X_DET direction
* `"th2PolyPartitionY"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, Y_DET direction
* `"peakingTime"` - `float`, peaking time of AGET electronics in nanoseconds (0 or 232 ns)
* `"nStrips"` - `int`, number of neighbouring strips considered during UVW projection
* `"nCells"` - `int`, number of neighbouring cells considered during UVW projection
* `"nPads"` - `int`, number of neighbouring pads considered during UVW projection

## Track3DBuilder

Configuration template:

```json
{
  "simRecoHitFilter": {
    "recoClusterType": "fraction",
    "recoClusterThreshold": 35.0,
    "recoClusterConstantFractionThreshold": 0.1,
    "recoClusterDeltaStrips": 2,
    "recoClusterDeltaTimeCells": 5
}
```

where:

* `"simRecoHitFilter"` - describes clustering method for creating true `RecHit2D` deposits per individual generator level `TrackSegment3D`:
  * `"recoClusterType"` - `string`, name of clustering method (`"none"`, `"threshold"`, `"fraction"`, `"island"`)
  * `"recoClusterThreshold"` - `float`, "seed" hit threshold in ADC units (same for all 2D projections)
  * `"recoClusterConstantFractionThreshold"` - `float`, "seed" hit fractional threshold [0..1] (wrt max. ADC value in each 2D projection)
  * `"recoClusterDeltaStrips"` - `int`, envelope size in strips around "seed" hit
  * `"recoClusterDeltaTimeCells"` - `int`, envelope size in time cells around "seed" hit

## TrackTruncator

Configuration template:

```json
{
  "IncludeElectronicsRange": true
}
```

where:

* `"IncludeElectronicsRange"` - `bool`, flag to select if electronics range should be used during track truncation

## TriggerSimulator

Configuration template:

```json
{
  "TriggerArrival": 0.1
}
```

where:

* `"TriggerArrival"` - `double`, fraction of the electronics time range at which the self trigger arrives


# Example configurations:

## Single $\alpha$ 
[montecarlo_1prong_gun.json](../config/montecarlo_1prong_gun.json) - mono-energetic alpha-particles of 3 MeV are emitted along X_DET axis from a fixed point in the center of TPC's active volume and
stopped in CO<sub>2</sub> gas kept at 190 mbar pressure.
The ROOT output file can be analyzed with [DrawBragg_example](../examples/DrawBragg_example.cpp) program, which creates a PDF file (`"bragg.pdf"`)
with dE/dx plots per event (first 10 events in this case) as well as several summary plots from entire sample (10k events in this case):
```Shell
cd resources
../bin/mcRunController ../config/config_GUI_MC.json --input.controllerConfigPath=../config/montecarlo_1prong_gun.json 
../bin/examples/DrawBragg_example SimEvent_Track3D_SingleAlpha_MC.root 10
```

## <sup>16</sup>O photodisintegration reaction
[montecarlo_O16_E1E2.json](../config/montecarlo_O16_E1E2.json) - the photodisintegration reactions of <sup>16</sup>O are induced by mono-energetic gamma photons of 11 MeV in the LAB reference frame.
In the centre-of-mass reference frame the reaction products follow a mixed E1+E2 polar angle distribution and uniform azimuthal angle distribution.
The vertices are generated uniformly along X_DET coordinate in the range [-100, 100] mm along nominal beam axis.
The resulting pseudo-reconstructed `Track3D` collections can be visualized in 3D by [PlotEvents_example](../examples/PlotEvents_example.cpp) program,
which creates, both, a PDF file and a ROOT C-macro (`"Generated_wirePlotTrack3D.[*]"`).
The visualisation accounts for apparent track shift along Z_DET coordinate due to simulated self-triggering mode of the DAQ electronics.
```Shell
cd resources
../bin/mcRunController ../config/config_GUI_MC.json
../bin/examples/PlotEvents_example \
       --geometryFile geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat \
       --dataFile SimEvent_Track3D_TwoProngE1E2_MC.root
root -l -x Generated_wirePlotTrack3D.C
```

## <sup>16</sup>O photodisintegration reaction (transient mode)
[config_GUI_MC.json](../../GUI/config/config_GUI_MC.json) - example [ConfigManager](../../Utilities/README.md)
JSON file for `tpcGUI`/`makeTrackTree` raw data processors that internally refers to [ModuleConfig.json](../config/ModuleConfig.json)
from the previous example and allows one to generate random events on-the-fly
in memory without the need of creating a large ROOT file with `PEventTPC` digitized raw data beforehand.
```Shell
cd resources
../bin/tpcGUI ../config/config_GUI_MC.json
```

