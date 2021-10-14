# dumpClusterHistos

## Description

The "dumpClusterHistos" executable extracts single event from the GRAW file and performs clusterization of hits.
The resulting histograms are written to the ROOT file of choice.
Use correct geometry_XXX.dat file for a given TPC detector (ELITPC, mini-TPC).

## Run instructions

```
cd build/resources
../bin/dumpClusterHistos <input_file.graw> <file_entry_number> <geometry_file.dat> <result_file.root>
```

Example to process frame no.100 (event id 25) for ELITPC raw-data file:
```
../bin/dumpClusterHistos /data/edaq/2021/20210622_extTrg_CO2_250mbar_DT1470ET/CoBo_ALL_AsAd_ALL_2021-06-22T12\:01\:56.568_0000.graw 100 ./geometry_ELITPC_250mbar_12.5MHz.dat /tmp/test_output.root
```

Example to process frame no.100 (vent id 100) for mini-TPC raw-data file:
```
../bin/dumpClusterHistos /data/edaq/2018/CoBo_2018-06-20T13\:12\:47.629_0001.graw 100 ./new_geometry_mini_eTPC.dat /tmp/test_output.root
```