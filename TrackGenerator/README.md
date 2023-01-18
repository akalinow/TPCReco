# TrackGenerator
generating Monte Carlo events for ELI-TPC detector
## Contents
This project includes parent abstract class `AbstractGenerator`, two child classes `LineGenerator` (produces simple line shapes) and `FromTransportGenerator` (converts output of Geant4 simulations to event format by) and two corresponding executables. Names are subjects to change.

## Building

```
git submodule update --init --recursive
cmake -DBUILD_MC=ON ..
make
```

## Usage
### LineGenerator
```
bin/SimpleGenerator config/LineGenerator.json
```
LineGenerator creates events of predefined shape without greater physical meaning. The generated tracks aren't related to any physical particles (the charge is 'put by hand' and not transported). 

#### Config
LineGenerator.json:
```
{   
    "events":4,                                                                     # number of events to generate
    "geometryFile": "/home/mfila/TPCReco/resources/new_geometry_mini_eTPC.dat",
    "outputFile": "lineEvents.root",
    "sigma":"3",                                                                    # gaussian broadening equal in every direction
    "MCcounts":"10000",                                                             # number of mc points in every line
    "NbinsX":100, "xmin":-100, "xmax":100,                                          # size of histogram where hits are stored
    "NbinsY":100, "ymin":-100, "ymax":100,                                          # Nbins is critical for time/resolution try experimenting
    "NbinsZ":100, "zmin":-100, "zmax":100,
    "lines":[                                                                       # lines or segments generated in very event
        {
            "x0":"0",
            "y0":"0",
            "z0":"0",
            "length":"60",
            "theta":"0",
            "phi":"0",
            "thetaRandom":false,                                                    #if true theta will be random (spherical distribution) in every event
            "phiRandom":false,                                                      #if true phi will be random in every event
            "lengthRandom": false                                                   #if true length will be random in every event
        },
        {
            "x0":"0",
            "y0":"0",
            "z0":"0",
            "length":"90",
            "theta":"1.7",
            "phi":"1.7",
            "thetaRandom":true,
            "phiRandom":true,
            "lengthRandom": false
        }
    ]
}
```
This config will generate 4 events, 2 lines in each, both lines starting at (0,0,0). First line will have fixed orientation and length, while second line will be oriented randomly in 4 Pi. 
### FromTransportGenerator
```
bin/EventGenerator config/FromTransportGenerator.json 
```
FromTransport creates events by projecting output from Geant4 simulation of particles transport in ELI-TPC detector to UVW readout strips.
To run simulations go to `MonteCarloSimulations` and run:

```
./GELI; # produces ntuple_t0.root 
./TransportSimulator  ntuple_t0.root; #produces ntuple_t0_ntuple_t0.root
```
`GELI` simulates prim ionization of detector medium due to passage of charged particles. `TransportSimulator` takes `GELI`'s output and simulates secondary ionization and charge collection.

#### Config
FromTransportGenerator.json:
```
{   
    "events": 4,                                                                    # number of events to generate. If 0 then all events from dataFile will be read
    "dataFile": "/home/mfila/data/neutrons_data/ntuple_t0_transport.root",          # output from MonteCarloSimulation's TransportSimulator
    "geometryFile": "/home/mfila/data/TPCReco/resources/geometry_mini_eTPC.dat",
    "outputFile": "MCevent.root"
 }
```
Size and bining of the hits histograms is defined by `TranportSimulator`. To change it, edit these lines in `MonteCarloSimulations/TransportSimulator/config.xml`:
```
<!-- Parameters of energy deposit histogram used by SimEvent objects -->
<energy_deposit_histogram>
	<nBinsX>350</nBinsX>
	<nBinsY>200</nBinsY>
	<nBinsZ>196</nBinsZ>
	<xLow>-175</xLow>
	<xUp>175</xUp>
	<yLow>-100</yLow>
	<yUp>100</yUp>
	<zLow>-98</zLow>
	<zUp>98</zUp>
</energy_deposit_histogram>
```
and re-run TransportSimulator. To change particles parameters edit `MonteCarloSimulations/Geant4/*.xml`. As of writing MonteCarloSimulations supports GammaBeam background events, ParticleGun (single particles running through detector) and Photodesintegration.
## Processing Generator's output
### tpcGUI
Output from `Generators` can be examined in tpcGUI in ROOT mode.
```
cd resources
../bin/tpcGUI ../config/mcConfig.json 
```
mcConfig.json :
```
{
    "dataFile": "/home/mfila/TPCReco/build/lineEvents.root",                # this should match "outputFile" in *Generator.json
    "geometryFile": "/home/mfila/TPCReco/resources/geometry_ELITPC.dat",    # this should match "geometryFile" in *Generator.json
    "resourcesPath":"/home/mfila/TPCReco/build/resources/"                  
 }
```
### TTree branches
Generator's output is saved as TTree in *.root file. The TTree structure is similar to that used in GrawToRoot with a few additional branches:

```
 Event            (EventTPC*)               #Generated event (same as GrawToRoot)
 tracksNo         (int*)                    #number of tracks in event
 A                (vector<unsigned int>*)   
 Z                (vector<unsigned int>*)   
 momentum         (vector<TVector3>*)       
 start            (vector<TVector3>*)
 stop             (vector<TVector3>*)
 energy           (vector<double>*)         
 length           (vector<double>*)
```
Size of each vector is equal tracksNo. A, Z, momentum, energy describe particle leaving corresponding track and are only meaningful in case of `FromTransportGenerator`.

##### glhf