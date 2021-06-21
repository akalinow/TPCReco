# Configuration files

Currently `tpcGUI` takes [JSON](https://en.wikipedia.org/wiki/JSON#Syntax) formatted configuration file as input.

## Known tags

- `dataFile` - a path to a `.graw`, `.root` file (offline mode) or a directory (online mode).
- `geometryFile` - a path to geometry `.dat` file. Geometries for **mini-eTPC** and full scale prototype **ELITPC** are available in _resources_,
- `resourcesPath` - a path to _resources_ directory,
- `updateInterval` - a refresh rate in ms for online mode,
- `frameLoadRange` - an amount of frames when collecting event from GRAW input files,
- `removePedestal` - flag to control pedestal removal. Has effect only for GRAW input files,
- `display` - a configuration of toggleable modes:
  - `zLogScale` - displays charge on logarithmic scale on the projection histograms ,
  - `autoZoom` - zooms the projection histograms to maximum charge deposits,
  - `recoMode` - enables the reconstruction mode,
  - `rate` - display rate instead of projection plot,
- `pedestal` - a pedestal calculator configuration:
  - `minPedestalCell` - an index of a first time cell used to calculate pedestals,
  - `maxPedestalCell` - an index of a last time cell used to calculate pedestals,
  - `minSignalCell` - an index of a first time cell used to calculate signals,
  - `maxSignallCell` - an index of a last time cell used to calculate signals,
- `eventFilter` - a configuration of filter:
  - `enabled` - toggles filtering,
  - `events` - an array of indices of eligible events,
  - `maxChargeLowerBound` - low cutoff value on maximum charge in event,
  - `maxChargeUpperBound` - high cutoff value on maximum charge in event,
  - `totalChargeLowerBound` - low cutoff value on total charge in event,
  - `totalChargeUpperBound` - high cutoff value on total charge in event

## Example

```json
{
    "dataFile": "/data/edaq/2018/CoBo_2018-06-20T10:35:30.853_0000.graw",
    "geometryFile": "/home/mfila/ELITPC/data/neutrons/geometry_mini_eTPC_2018-06-19T10:35:30.853.dat",
    "resourcesPath": "/home/mfila/TPCReco/build/resources/",
    "updateInterval": 3000,
    "removePedestal": true,
    "display": {
        "autoZoom": true,
    }
    "eventFilter": {
        "enabled": true,
        "events": [
            4,
            5,
            9,
            12
        ],
        "totalChargeLowerBound": 1200.5
    }
}
```
