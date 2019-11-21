## Installation instructions:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
git checkout relevant_tag
mkdir build; cd build
cmake ../
make install -j 4
```
## Run instructions:

```
./bin/tpcGUI config/config_GUI.json
```
