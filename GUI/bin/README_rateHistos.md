# dumpRateHistos

## Description

The "dumpRateHistos" executable processes a single GRAW file and finds dot-like events according to specified cuts.
It prepares several histograms that are useful to estimate event rate, dead time and distribution of dot-like events on the readout plane:  
- time difference between events (all events, dot eventts)
- distribution of dots on XY plane.

## Run instructions - single file

```
cd build/resources
../bin/dumpRateHistos <input_file.graw> <hit_threshold> <total_charge_threshold> <matching_R_in_mm> <geometry_file.dat> <result_file.root>
```

For example:
```
../bin/dumpRateHistos /data/edaq/2021/20210622_extTrg_CO2_250mbar_DT1470ET/CoBo_ALL_AsAd_ALL_2021-06-22T12\:01\:56.568_0000.graw 50 200 25.0 ./geometry_ELITPC_250mbar_12.5MHz.dat /tmp/test_output.root
```


## Run instructions - list of files

In roder to process multiple files GRAW files correspoinding to same TPC geometry the following BASH script can be used:
```
cd build/resources
../tools/batch_job/job_dotrate.sh --optionName value [--optionName value [--optionName value]]
```
where available options are:
```
--dataFile     [SINGLE_GRAW_FILE]
--fileList     [FILE_WITH_LIST_OF_GRAW_FILES]
--hitThr       [number>0]                   default=50 ADC units
--minCharge    [number>0]                   default=200 ADC units
--maxRadius    [number>0]                   default=25.0 mm
--geometryFile [ALTERNATIVE_GEOMETRY_FILE]  default=./geometry_ELITPC_80mbar_12.5MHz.dat
--binDir       [ALTERNATIVE_EXEC_DIRECTORY] default=../bin
--ncpu         [number>0]                   default=1
--outputDir    [NEW_OUTPUT_DIRECTORY]       default=./

```
A new ROOT file with merged jobs will be called "results_YYYYDDMM_hhmmss.root" and the list of all ROOT files used to produce this merged file will be stored in "results_YYYYMMDD_hhmmss.list".

In this example, the script will process all GRAW files listed in "/tmp/listfile" using default hit criteria and then merge all resulting ROOT files using up to parallel 8 proceses at the same time.
A nice-looking PDF plot from combined ROOT files can be created using "plot_results_dotrate.C" macro. 
```
cd build/resources
time ../tools/batch_job/job_dotrate.sh --fileList /tmp/listoffiles --ncpu 8 --geometryFile ./geometry_ELITPC_80mbar_12.5MHz.dat --outputDir /tmp/newResultDir
root -q -b '../tools/batch_job/plot_results_dotrate.C('/tmp/newResultDir/results_YYYYMMDD_HHMMSS.root")'

```