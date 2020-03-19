## Installation instructions:

Before compilig setup the GET environment.
At the daqula2 node use afollowing command:

```
source /opt/soft/GetSoftware_20170928/thisGET.sh
```

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
mkdir build; cd build
cmake ../
make install -j 4
```

Convert all graw files in the directory hardcoded in the convertGrawToEventCharges.py file
to ROOT EventCharges format. Since the GET software nees a lots of config files the command has to
issued from a directory containing the config files. (To be fixed in some distant future.)
Each .graw file is converted into a .root file with the same timestamp and file fragment number.
Conversion requires a geometry file witg the same time stamp as data files (without file fragment number).
IMPORTANT: Update the path to data location
```
cd resources
../python/convertGrawToEventCharges.py
```

Read an event in EventCharges format, and fill an example histogram.
Requires geometry definition in resources/geometry_mini_eTPC.dat.
The plot will be saved to current directory.
IMPORTANT: Update the data file name and geometry location.
```
cd ../
./bin/testEventChargesread
```

Read all events in may all files from a single run (single timestamp), and fill an example histogram.
Requires geometry definition in resources/geometry_mini_eTPC.dat.
The plot will be saved to current directory.
IMPORTANT: Update the data file name and geometry location.
```
cd ../
./bin/testEventChargesreadTChain
```