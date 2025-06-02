# Concept

The key concept of the framework is to encapsulate each functional block of the simulation into `Module`. Modules are
run sequentially and can pass information to the ones after them in a sequence (for example result of the simulation can
be written to a file by the exporter module).

# Modules

All modules inherit from an abstract class [fwk::VModule](../UtilsMC/include/TPCReco/VModule.h). Each module has to
implement `VModule`'s pure virtual methods:

* `EResultFlag Init(boost::property_tree::ptree config)` - initialize module, called at the beginning of the simulation,
  takes `boost` configuration as an argument
* `EResultFlag Process(ModuleExchangeSpace &event)` - main *player*, do what module's work, module is able to read from
  and write to `ModuleExchangeSpace`, which is used for inter-module communication
* `EResultFlag Finish()` - finish the run, called for each module at the end of the simulation, used for cleanup

## Available modules

* [Generator](Generator) - Wrapper for [EventGenerator](../EventGenerator/README.md) for generating `SimEvent`s.
* [EventFileExporter](EventFileExporter) - Writes simulation results into ROOT files
* [GeantSim](GeantSim) - Handles GEANT simulation of detector response - it takes `SimEvent` and tracks primary
  particles through the detector
* [ToyIonizationSimulator](ToyIonizationSimulator) - Simple ionization simulator based on `IonRangeCalculator`
* [TriggerSimulator](TriggerSimulator) - Simulates self-triggering of the TPC by finding `z` position of first energy
  deposit that reaches the readout plane and shifts the whole event appropriately
* [TrackTruncator](TrackTruncator) - Truncates `SimTrack`s in a `SimEvent` to active volume of the detector and possibly
  to GET electronics range
* [Track3DBuilder](Track3DBuilder) - Builds `Track3D` objects from `SimEvent` objects - they are required by the
  existing legacy code used for comparison between pure and reconstructed MonteCarlo
* [TPCDigitizerRandom](TPCDigitizerRandom) - TPC digitizer based on Artur's approach for UVW projection. Each deposit is
  smeared with a 3D gaussian function by sampling with configurable number of points
* [TPCDigitizerSRC](TPCDigitizerSRC) - TPC digitizer based on Mikolaj's `StripResponseCalculator`. It reads generated
  strip response histograms from a ROOT file that corresponds to the chosen TPC working conditions (including:
  drift velocity, electronics sampling rate and peaking time, "effective" gaussian diffusion). More strip response
  ROOT files can be generated using ROOT macro [testStripResponseCalculator](../../Reconstruction/examples/testStripResponseCalculator.cxx).


`RunController` creates all the modules, initializes them (`Init` method), runs `Process` method in the right order, and
then cleans up with `Finish` method.

## ModuleExchangeSpace

The modules communicate with each-other through [ModuleExchangeSpace](../UtilsMC/include/TPCReco/ModuleExchangeSpace.h).
It contains:

* `SimEvent`
* `PEventTPC`
* `Track3D`
* `eventraw::EventInfo`

`RunController` keeps one instance of `ModuleExchangeSpace` and passes it by reference to the modules' `Process`
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

**Example 3** - Get ionization losses from GEANT, truncate tracks due to fiducial volume and electronics, store `SimEvent`s and `Track3D`s to ROOT file, store raw signals in another ROOT file:
```json
  "ModuleSequence": [
    "Generator",
    "GeantSim",
    "TriggerSimulator",
    "TrackTruncator",
    "Track3DBuilder",
    "TPCDigitizerSRC",
    "EventFileExporter"
  ]
```


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
  * `"PEventTPC"` - *raw* data (`PEventTPC`), requires one of `TPCDigitizer[*]` modules
  * `"Track3D"` - *'reconstructed'* data (`Track3D`), requires `Track3DBuilder` module
* `"DisabledBranches"` - vector of `string` describing branches to be excluded from saving into file.
  Branches can belong to two ROOT `TTree`'s stored into the output file (`"TPCData"` and `"TPCRecoData"`).
  List of all possibilities can be found by running `TTree::Print()` on the given tree. Branches occupying most space:
  * `"TPCData.tracks.hits"` - `std::vector` with all energy deposits (`SimHits`) generated by Geant/ToyMC
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
    "ModelPath": "/scratch/MonteCarloSimulations/Geant4/GEANT_elitpc_model_20180302/STwwL/",
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

* `"EnableAlphaStraggling"` - `bool`, switch to enable multiple scattering for 4He/3He particles
* `"Temperature"` - `float`, gas temperature in K
* `"gas_mixture"` - describes partial pressures of mixture components:
  * `"p_co2"` - `float`, CO2 pressure in bar
  * `"p_he"` - `float`, He pressure in bar
* `"magnetic_field"` - configuration of the magnetic field of the purging magnet:
  * `"magnetic_field_ON"` - `bool`, magnetic field ON(`true`) or OFF(`false`)
  * `"magnetic_field_map"` - `"string"`, path to magnetic field table
  * `"magnetic_field_offset"` - `float`, offset along beam axis in mm
* `"GeometryConfig"` - configuration of the geometry:
  * `"ModelPath"` - `string`, path to the directory with STL files
  * `"MaterialColors"` - definition of material colors, as in the config above
  * `"Solids"` - definition of different solids for each material, wildcards can be used

## Generator

Configuration template:

```json
{
  "NumberOfEvents": {},
  "EventGenerator": {}
}
```

where:

* `"NumberOfEvents"` - `int`, number of events to be generated
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
* `"sigmaXY"` - `float`, sigma for diffusion in plane perpendicular to drift direction in millimiters
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction in millimiters
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
* `"sigmaXY"` - `float`, sigma for diffusion in plane perpendicular to drift direction in millimiters
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction in millimiters
* `"MeVToChargeScale"` - `float`, number of ADC counts per MeV
* `"th2PolyPartitionX"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, `x` direction
* `"th2PolyPartitionY"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, `y` direction
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


# Example configurations

Example of the light configuration that stores only generator level information plus `SimHits` from GEANT4 can be found in [ModuleConfigGun.json](../config/ModuleConfigGun.json).

Example of the full configuration can be found in [ModuleConfig.json](../config/ModuleConfig.json).

After compilation they can be run with:

```Shell
cd resources
../bin/mcRunController ../config/montecarlo_ModuleConfig.json
../bin/mcRunController ../config/montecarlo_ModuleConfigGun.json
```