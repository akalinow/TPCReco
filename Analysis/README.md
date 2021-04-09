## Run instructions:

Update the ROOT macro with correct path to data and geometry files in the script analyzeRecoEvent.cxx
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
