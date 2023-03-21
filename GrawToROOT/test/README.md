## Installation instructions:

```
source /opt/soft/GetSoftware_bin/env_settings.sh
git clone ssh://git@dracula.hep.fuw.edu.pl:8822/elitpcSoftware/TPCReco.git
cd TPCReco
git checkout relevant_tag
git submodule update --init --recursive
mkdir build; cd build
cmake ../
make install -j 4
```

## Update instructions

To synchronize the version of software in your working directory with some never tag please do following:

```
cd TPCReco
git fetch
git checkout newer_tag
cd build
cmake ../
make install -j 4
```

## Run instructions:

To run test file EventTPC_tst do following:

``` 
cd resources
../GrawToROOT/EventTPC_tst
```

## Structure of EventTPC_tst.cpp:

In file there are define void function that takes arguments:

 - std::shared_ptr<EventTPC> EventPtr =  myEventSource->loadDataFile(dataFileName)->getCurrentEvent()
 - std::map<std::string, double> Test_Reference - defined in dataEventTPC.h
 - std::map<std::string, std::string> Test_Reference_Titles - defined in dataEventTPC.h
  
Those functions are:
 - get1DProjection_Titles_Test
 - get2DProjection_Titles_Test
 - get1DProjection_Test
 - get2DProjection_Test
 - GetTotalCharge_Test
 - GetMaxCharge_Test
 - GetMaxChargePos_Test
 - GetSignalRange_Test
 - GetMultiplicity_Test
 
 Each one comperes returns of class EventTPC methods with example data, which are sturcture into maps in dataEventTPC.h.
 
