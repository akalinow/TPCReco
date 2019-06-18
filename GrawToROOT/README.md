## Installation instructions:

```
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/akalinowski/TPCReco.git
cd TPCReco
git checkout relevant_tag
mkdir build; cd build
cmake ../
make install -j 4
```

Convert all graw files in the directory hardcoded in the convertGrawToEventTPC.py file
to ROOT EventTPC format as GET software nees a lots of config files the command has to
issued from a directory containing the config files. (To be fixed in some distant future.)
Each .graw file is converted into a .root file with a run number assigned.

```
cd resources
python ../python/convertGrawToEventTPC.py
```