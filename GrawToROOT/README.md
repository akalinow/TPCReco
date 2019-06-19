## Installation instructions:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
mkdir build; cd build
cmake ../
make install -j 4
```

Convert all graw files in the directory hardcoded in the convertGrawToEventTPC.py file
to ROOT EventTPC format. Since the GET software nees a lots of config files the command has to
issued from a directory containing the config files. (To be fixed in some distant future.)
Each .graw file is converted into a .root file with a run number assigned.

```
cd resources
../python/convertGrawToEventTPC.py
```

Read an event (resources/EventTPC_1.root ) in EventTPC format, and plot the three projections.
Requires geometry definition in resources/geometry_mini_eTPC.dat.

```
./bin/testEventTPCread
```