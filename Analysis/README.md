## Batch analysis

The batch analysis is a simple script that analyses a selected number of files (GRAW or ROOT) on a single PC.


1) edit the [makeTrackTree.py](python/makeTrackTree.py) file to set the input directories, and relevant geometry files:

```
runs = [
    ("/mnt/NAS_STORAGE_BIG/IFJ_VdG_20210630/20210621_extTrg_CO2_250mbar_DT1470ET/",
     "../geometry_ELITPC_250mbar_12.5MHz.dat"),
]
```

1.1) Optional: edit the [makeTrackTree.py](python/makeTrackTree.py) file to add automatic output file merging:
Modify the `finalizeFunc()` by adding the list of runs to be merged, and the location of the final ouput files:

```
samples_HIGGS = [
    "2022-04-12T15-28-17.188"
]

command = "mkdir HIgS_2022"
os.system(command)

for item in samples_HIGGS:
        command = "mv "+item+" HIgS_2022"
        os.system(command)
        command = "hadd -f HIgS_2022/HIgS_2022.root HIgS_2022/*/*.root"
        os.system(command)
```		

2) run the script from the resources directory:

```
cd resources
../python/makeTrackTree.py
```
3) merge the ROOT files in selected directories (optional if step 1.1 was executed):

```
cd 2022-04-12T15-28-17.188
hadD -f HIgS_2022.root TrackTree_*.root
```

4) make plots
```
root
.L ../../test/makePlots.C
makePlots(" HIgS_2022.root")
```

The [makeTrackTree.py](python/makeTrackTree.py) script will create a directory for each run timestamp:

```
drwxrwxr-x  2 user1 user1   61440 Feb 28 10:45 2021-11-25T15-00-32.273
drwxrwxr-x  2 user1 user1   61440 Feb 25 17:11 2021-11-25T15-21-05.094
drwxrwxr-x  2 user1 user1   20480 Feb 25 17:37 2021-11-25T16-12-14.995
drwxrwxr-x  2 user1 user1   69632 Feb 28 12:58 2021-11-25T16-29-22.081
```

Each directory will contain a set of ROOT files created for a single input file:

```
-rw-r--r-- 1 user1 user1   63807 Feb 25 16:47 TrackTree_2021-11-25T16:29:22.081_0001.root
-rw-r--r-- 1 user1 user1   63823 Feb 25 17:07 TrackTree_2021-11-25T16:29:22.081_0002.root

-rw-r--r-- 1 user1 user1   63807 Feb 25 16:47 Reco_EventTPC_2021-11-25T16:29:22.081_0001.root
-rw-r--r-- 1 user1 user1   63823 Feb 25 17:07 Reco_EventTPC_2021-11-25T16:29:22.081_0002.root

```

The `TrackTree` ROOT files contain a simple TTree with basic track properties:
```
typedef struct {Float_t eventId, frameId, length,
    horizontalLostLength, verticalLostLength,
    energy, charge, cosTheta, phi, chi2,
    x0, y0, z0, x1, y1, z1;} TrackData;
```

The `Reco_EventTPC` ROOT files contain full reconstruction information:
```

```

The `TrackTree` TTree can be analysed with a example scripts:
* [makeCalibrationPlots.cpp](test/makeCalibrationPlots.cpp) - a script for plotting track length for calibration data
* [makePlots.cpp](test/makePlots.cpp) - a script for plotting track length for physics data


## Analysis of the full `Reco_EventTPC` file:

Update the ROOT macro with correct path to data and geometry files in the script
[analyzeRecoEvent.cxx](test/analyzeRecoEvent.cxx):
```
std::string fileName = "Reco_EventTPC_2018-06-19T15:13:33.941_0008.root";
Track3D *aTrack = loadRecoEvent(fileName);
fileName = "/home/akalinow/scratch/data/neutrons/geometry_mini_eTPC_2018-06-19T10:35:30.853.dat";
```

The script is assumed to be run from resources directory:

```
cd resources
root
root [0] .L ../test/analyzeRecoEvent.cxx
root [1] plotTrack()
```

After script modifications the ROOT has to reloaded from scratch:
```
root [2] .q
root
root [0] .L ../test/analyzeRecoEvent.cxx
root [1] plotTrack()
```
