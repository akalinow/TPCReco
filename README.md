# TPCReco

## Dependences

```Shell
sudo apt-get update
sudo apt-get install $(cat requirements_apt.txt)
pip3 install -H -r requirements_pip3.txt
```

## Installation instructions:

```Shell
source /opt/soft/GetSoftware_bin/env_settings.sh
git clone git@github.com:WarsawTPC/TPCReco.git
cd TPCReco
git submodule update --init --recursive
mkdir build; cd build
cmake -DBUILD_TEST=ON ../
make install -j 4
```

Run tests to check if everything is fine:

```Shell
export LC_ALL=$LANG
ctest
```

## Update instructions

To synchronize the version of software in your working directory with some never tag please do following:

```Shell
cd TPCReco
git fetch
git checkout newer_tag
cd build
cmake ../
make install -j 4
```

You can check the tag version for your working directory with

```Shell
cd TPCReco
git branch
```

the output should look like this:

```Shell
akalinow@daqula2:~/1/TPCReco$ git branch
* (HEAD detached at v0.02_28.04.2021)
  master
```

## Run instructions:

Use one of pre-defined configuration files (`config/config_GUI__OFFLINE[*].json`) corresponding to the known TPC working conditions with appropriate command line modificators (`--input.dataFile`) pointing to the correct location of the input data file(s). Data file argument should refer to:
  * single ROOT file (offline mode),
  * single GRAW file (offline mode),
  * list of comma-separated GRAW files for the same run (offline mode),
  * directory name to be minitored for new GRAW files (online mode).

When reading GRAW files setup GET and ROOT software environments beforehand. At **daqula2** node use the following commands:
```Shell
source /opt/soft/root_v6.08.06/bin/thisroot.sh
source /opt/soft/GetSoftware_bin/env_settings.sh
```
Also when reading GRAW files run the application from the **resources** directory since GET software requires some additional XCFG files located therein. Alternatively modify `input.resourcePath` accordingly.

When running from a container, enter the following command inside the container:
```Shell
export LC_ALL=$LANG
```

After successful compilation try to run the GUI from the **resources** directory:

```Shell 
cd resources
../bin/tpcGUI --meta.configJson ~/.tpcreco/config/test.json
../bin/tpcGUI ~/.tpcreco/config/test.json
```

Multiple JSON files can be specified at the same time.
For example, to add event filter for event browsing use this syntax at **daqula2** node:

```Shell
cd resources
../bin/tpcGUI \
../config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json \
../config/config_GUI_filter.json \
--input.dataFile $( ../bin/grawls --input 20220828111932 --chunk 0 --separator "," --directory /data/edaq/2022/HIgS_2022/20220828_extTrg_CO2_130mbar --ms 1000 )
```
Any application parameter can be set using a JSON file or a command line argument. Command line arguments **overwrite** settings from the JSON file(s).
List of all parameters is provided by

```Shell
../bin/tpcGUI --help
```

Check config file [structure and examples](Utilities/README.md) for more details.

