# dumpRateHistos

## Description

The "dumpRateHistos" executable processes a single GRAW file chunk (_NNNN.graw) and finds dot-like events according to specified cuts.
It prepares several histograms that are useful to estimate event rate, dead time and distribution of dot-like events on the readout plane:  
- time difference between events (all events, dot eventts)
- distribution of dots on XY plane.

## Run instructions - single file chunk (_NNNN.graw)

The executable expects a JSON config file name as the first command line argument.
Additional options and/or flags can be specified afterwards to override config file settings:
```
cd build/resources
../bin/dumpRateHistos <config.json> [--optionName value, ...] [--optionFlag]
```
where available options are:
```
  --geometryFile string    - path to TPC geometry file
  --dataFile string        - path to raw data file in single-GRAW mode
                             (or list of comma-separated raw data files in multi-GRAW mode)
  --frameLoadRange int     - maximal number of frames to be read by event builder in single-GRAW mode
  --singleAsadGrawFile     - when present, indicates multi-GRAW mode
  --hitThr int             - minimal hit charge after pedestal subtraction [ADC units]
  --totalChargeThr int     - minimal event total charge after pedestal subtraction [ADC units]
  --matchRadiusInMM float  - matching radius for strips and time cells from different U/V/W directions [mm]
  --outputFile string      - path to the output ROOT file
```

Example without overriding default parameters:
```
../bin/dumpRateHistos ../config/config_DotFinder.json
../bin/dumpRateHistos ../config/config_DotFinder_NGRAW.json
```

Example with overriding default parameters:
```
../bin/dumpRateHistos ../config/config_DotFinder.json \
  --dataFile /data/edaq/2021/20210622_extTrg_CO2_250mbar_DT1470ET/CoBo_ALL_AsAd_ALL_2021-06-22T12\:01\:56.568_0000.graw \
  --hitThr 50 --totalChargeThr 200 --matchRadiusInMM 25.0 \
  --geometryFile ./geometry_ELITPC_250mbar_12.5MHz.dat \
  --outputFile /tmp/test_output.root
```

In multi-GRAW mode the 'singleAsadGrawFile' flag must be set (JSON or CLI) and the 'dataFile' field (JSON or CLI) must be initialized with comma-separated list of GRAW files corresponding to individual ASAD boards of a given _NNNN file chunk.
In single-GRAW mode the 'frameLoadRange' parameter should be set to the maximal distance in GRAW frames for same event number. This parameter has no effect in multi-GRAW mode.


## Run instructions - list of files (multiple _NNNN.graw file chunks)

In order to process multiple GRAW file chunks (many _NNNN.graw) correspoinding to same TPC geometry the following BASH script can be used:
```
cd build/resources
chmod +x ../tools/batch_job/job_dotrate.sh
../tools/batch_job/job_dotrate.sh config.json [--optionName value, ...]
```
where available options are:
```
--dataFile     [SINGLE_GRAW_FILE]
--fileList     [FILE_WITH_LIST_OF_GRAW_FILES]
--geometryFile [ALTERNATIVE_GEOMETRY_FILE]  default=./geometry_ELITPC_80mbar_12.5MHz.dat
--binDir       [ALTERNATIVE_EXEC_DIRECTORY] default=../bin
--ncpu         [number>0]                   default=8
--outputDir    [NEW_OUTPUT_DIRECTORY]       default=./

```
A new ROOT file with merged jobs will be called "DotFinder_YYYYDDMM_hhmmss.root" and the list of all ROOT files used to produce this merged file will be stored in "DotFinder_YYYYMMDD_hhmmss.list".

In the example below, the script will process all GRAW files listed in "/tmp/listfile" using default hit criteria and then merge all resulting ROOT files using up to parallel 8 proceses at the same time.
```
cd build/resources
time ../tools/batch_job/job_dotrate.sh --fileList /tmp/listoffiles --ncpu 8 --geometryFile ./geometry_ELITPC_80mbar_12.5MHz.dat --outputDir /tmp/newResultDir
```

A nice-looking PDF plot from the combined ROOT files can be created using "plot_results_dotrate.C" macro.
```
root -q -b '../tools/batch_job/plot_results_dotrate.C('/tmp/newResultDir/DotFinder_YYYYMMDD_HHMMSS.root")'

```