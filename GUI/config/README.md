# Configuration files

Currently `tpcGUI` takes [JSON](https://en.wikipedia.org/wiki/JSON#Syntax) formatted configuration file as input.

Known tags are defined in the [allowedOptions.json](Utilities/config/allowedOptions.json) file. List of allowed options is printed with:

```Bash
../bin/tpcGUI --help
```

## Example

```json
{
"input":{
    "dataFile": "../testData/CoBo0_AsAd0_2022-04-12T08_03_44.531_0000.graw,../testData/CoBo0_AsAd1_2022-04-12T08_03_44.533_0000.graw,../testData/CoBo0_AsAd2_2022-04-12T08_03_44.536_0000.graw,../testData/CoBo0_AsAd3_2022-04-12T08_03_44.540_0000.graw",
    "geometryFile": "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat",    
    "resourcesPath":"./"
},
"conditions":{
    "samplingRate": 25.0,
    "pressure": 250,
    "driftV": 0.405
},
"pedestal":{
    "minPedestalCell" : 10,
    "maxPedestalCell" : 50,
    "minSignalCell": 2,
    "maxSignalCell": 500
},
    "beamParameters":{
        "energy" : 99.9
    }
}
```
