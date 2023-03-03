# Concept 
The key concept of the framework is to encapsulate each functional block of the simulation into `Module`. Modules are run sequentially and can pass information to the ones after them in a sequence (for example result of the simulation can be written to a file by the exporter module).


# Modules
All modules inherit from an abstract class [fwk::VModule](../UtilsMC/include/VModule.h). Each module has to implement `VModule`'s pure virtual methods:
* `EResultFlag Init(boost::property_tree::ptree config)` - initialize module, called at the beginning of the simulation, takes `boost` configuration as an argument 
* `EResultFlag Process(ModuleExchangeSpace &event)` - main *player*, do what module's work, module is able to read from and write to `ModuleExchangeSpace`, which is used for inter-module communication
* `EResultFlag Finish()` - finish the run, called for each module at the end of the simulation, used for cleanup


## Available modules
* [Generator](Generator) - Wrapper for [EventGenerator](../EventGenerator/README.md) for generating `SimEvent`s
* [EventFileExporter](EventFileExporter) - Writes simulation results into ROOT files
* [GeantSim](GeantSim) - Handles GEANT simulation of detector response - it takes `SimEvent` and tracks primary particles through the detector
* [TPCDigitizerRandom](TPCDigitizerRandom) - TPC digitizer based on Artur's approach for UVW projection. Each deposit is smeared with a 3D gaussian function by sampling with configurable number of points
* [TPCDigitizerSRC](TPCDigitizerSRC) - TPC digitizer based on Mikolaj's `StripResponseCalculator`. It can either read a generated strip response histograms from a ROOT file or generate new ones with default parameters, if the response does not exist.
* [Track3DBuilder](Track3DBuilder) - Builds `Track3D` objects from `SimEvent` objects - they are required by the existing legacy code used for comparison between pure and reconstructed MonteCarlo
* [TrackTruncator](TrackTruncator) - Truncates `SimTrack`s in a `SimEvent` to active volume of the detector and possibly to GET electronics range
* [TriggerSimulator](TriggerSimulator) - Simulates self-triggering of the TPC by finding `z` position of first energy deposit that reaches the readout plane and shifts the whole event appropriately

`RunController` creates all the modules, initializes them (`Init` method), runs `Process` method in the right order, and then cleans up with `Finish` method.

## ModuleExchangeSpace
The modules communicate with each-other through [ModuleExchangeSpace](../UtilsMC/include/ModuleExchangeSpace.h). It contains:
* `SimEvent`
* `PEventTPC`
* `Track3D`
* `eventraw::EventInfo`

`RunController` keeps one instance of `ModuleExchangeSpace` and passes it by reference to the modules' `Process` methods, that way the modules have read/write access.

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
* `"ModuleSequence"` - vector of `string`, sequence of modules to be run in the same order, here `"ModuleA"`, `"ModuleB"` and "`"ModuleC"`"
* `"GeometryConfig"` - `string`, path to `geometry_ELITPC` configuration
* `"ModuleConfiguration"` - configuration of modules, each module receives a `JSON` object parsed into `boost::property_tree::ptree` object as an argument to `ModuleName::Init` method

# Configuration of modules
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

## EventFileExporter
Configuration template:
```json
{
  "FileName": {},
  "EnabledBranches": [
    {},
    {}
  ]
}
```
where:
* `"FileName"` - `string`, name of the output file
* `"EnabledBranches"` - vector of `string` describing branches to be saved into file, available branches:
  * `"SimEvent"` - pure Monnte Carlo data (`SimEvent`)
  * `"PEventTPC"` - *raw* data (`PEventTPC`)
  * `"Track3D"` - *'reconstructed'* data (`Track3D`)

## GeantSim
Configuration template:
```json
{
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
* `"gas_mixture"` - describes partial pressures of mixture components in bar
* `"magnetic_field"` - configuration of the magnetic field of the purging magnet
  * `"magnetic_field_ON"` - `bool`, magnetic field ON(`true`) or OFF(`false`)
  * `"magnetic_field_map"` - `"string"`, path to magnetic field table
  * `"magnetic_field_offset"` - `float`, offset along beam axis in mm
* `"GeometryConfig"` - configuration of the geometry:
  * `"ModelPath"` - `string`, path to the directory with STL files
  * `"MaterialColors"` - definition of material colors, as in the config above
  * `"Solids"` - definition of different solids for each material, wildcards can be used

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
* `"GeometryConfig"` - `string`, path to `geometry_ELITPC` configuration
* `"sigmaXY"` - `float`, sigma for diffusion in plane perpendicular to drift direction
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction
* `"NSamplesPerHit"` - `int`, number of random samples per simulated hit
* `"MeVToChargeScale"` - `float`, number of ADC samples per MeV

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
* `"GeometryConfig"` - `string`, path to `geometry_ELITPC` configuration
* `"StripResponsePath"` - `string`, path to directory where strip responses are stored
* `"sigmaXY"` - `float`, sigma for diffusion in plane perpendicular to drift direction
* `"sigmaZ"` - `float`, sigma for diffusion along drift direction
* `"MeVToChargeScale"` - `float`, number of ADC samples per MeV
* `"th2PolyPartitionX"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, `x` direction
* `"th2PolyPartitionY"` - `int`, repartition parameter for `TH2Poly` in `GeometryTPC`, `y` direction
* `"peakingTime"` - `float`, peaking time of AGET electronics
* `"nStrips"` - `int`, number of neighbouring strips considered during UVW projection
* `"nCells"` - `int`, number of neighbouring cells considered during UVW projection
* `"nPads"` - `int`, number of neighbouring pads considered during UVW projection

## Track3DBuilder
Configuration template:
```json
{
}
```
**Note**: When module does not have any configuration parameters an empty JSON object has to be provided anyway.
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


# Example configuration
Example of the full configuration can be found in [ModuleConfig.json](../config/ModuleConfig.json). It can be run with `build/bin/examples/RunControllerExample /path/to/config/file`