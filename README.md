## Installation instructions:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8888/elitpcSoftware/TPCReco.git
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
When reading a GRAW file one has to run the application from the resources directory, as GET software requires a lots of additional files.
When reading GRAW files setup the GRAW environment. At the daqula2 node use a following command:
For reading the GRAW files in online mode set  "dataFile": "directory_path" WITH trailing "/" character. 
```
source /opt/soft/GetSoftware_bin/env_settings.sh
```

```
cd resources
../bin/tpcGUI ../config/config_GUI.json
```