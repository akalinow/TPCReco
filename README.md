## Installation instructions:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
git checkout relevant_tag
git submodule update --init --recursive
mkdir build; cd build
cmake ../
make install -j 4
```
## Run instructions:
Update the  config/config_GUI.json with correct values for input date file (ROOT or GRAW), corresponding geometry file,
and location of the resources directory.
When reading a GRAW file one has to enter the resources direcotry, as GET software requires a lots of additional files.

```
cd resources
../bin/tpcGUI ../config/config_GUI.json
```