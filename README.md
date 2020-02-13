## Installation instructions:

```
git clone https://github.com/akalinow/TPCReco.git
cd TPCReco
git checkout relevant_tag
mkdir build; cd build
cmake ../
make install -j 4
```

## Run instructions:

### For *GUI* interface
```
./bin/tpcGUI config/config_GUI.json
```

### For *Batch* interface
```
./bin/tpcBatch
```

### To generate dummy EventTPC data
```
./bin/dummyEventTPCMaker [TIMESTAMP]
```
Where `[TIMESTAMP]` acts as an unique id of the generated file.

## Testing:
We use the googletest library, for unit testing. See `TPCReco/DataFormatsTests` for an example of a test case and the CMake config.
