# TPCReco

## Dependences

```Shell
sudo apt-get update
sudo apt-get install $(cat requirements_apt.txt)
pip3 install -H -r requirements_pip3.txt
```

## Installation instructions:

```Shell
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

Run a specific test, here `GrawToROOT_tst` with output enabled for failed tests:
```Shell
ctest -R GrawToROOT_tst --output-on-failure
```

GRAW files are read by TPCReco's own reader (see [GrawToROOT README](GrawToROOT/README.md)),
the GET software is not needed. If the GET software is found by CMake
(e.g. after `source /opt/soft/GetSoftware_bin/env_settings.sh`), an additional test,
`GrawFileVsGET_tst`, compares the TPCReco reader with the GET reader.


## Update instructions

To synchronize the version of software in your working directory with a newer tag do the following:

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

## Run instructions

The applications are installed in `<build>/bin` and the configuration files in `<build>/config`,
where `<build>` is the installation directory (`CMAKE_INSTALL_PREFIX`, by default the build directory).
They can be run from any directory; output files are written to the current directory.

Relative paths in the configuration files and on the command line (configuration files, geometry,
data files, MC controller configuration, ML model, GUI images) are looked up in the current directory
first, then in the TPCReco installation (`$TPCRECO_RESOURCES`, if set, is searched before it).
The resolved paths are printed at start-up and stored in the configuration dump.

When running from a container, enter the following command inside the container:
```Shell
export LC_ALL=$LANG
```

Reading GRAW files does not require the GET software, nor the XCFG frame format files.

### tpcGUI

Use one of the pre-defined configuration files (`<build>/config/config_GUI__OFFLINE_*.json`) corresponding
to the known TPC working conditions, with `--input.dataFile` pointing to the input data. The data file
argument can be:
  * a single ROOT file (offline mode),
  * a single GRAW file (offline mode),
  * a list of comma-separated GRAW files of the same run (offline mode),
  * a directory to be monitored for new GRAW files (online mode).

```Shell
<build>/bin/tpcGUI <build>/config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json \
   --input.dataFile <comma separated GRAW files of a run chunk, or a ROOT file>
```

Examples:

* test data (included in the TPCReco repository and installed with it):

```Shell
<build>/bin/tpcGUI ~/.tpcreco/config/test.json
```

* generated $\alpha$ + $C$ events distributed according to the E1+E2 transitions mixture. This requires
  the Geant4 environment and compilation with the `-DBUILD_GEANT_MODULE=ON` option
  (`cmake -DBUILD_TEST=ON -DBUILD_GEANT_MODULE=ON ../`):

```Shell
<build>/bin/tpcGUI <build>/config/config_GUI_MC.json
```

* single generated $\alpha$ events with the track direction controlled by the user:

```Shell
<build>/bin/tpcGUI <build>/config/config_GUI_MC.json \
   --input.controllerConfigPath=<build>/config/montecarlo_2prong_gun.json
```

* several JSON files at once, e.g. a working point plus an event filter, with the GRAW files of a run
  chunk listed by `grawls` (at the **daqula2** node):

```Shell
<build>/bin/tpcGUI \
   <build>/config/config_GUI__OFFLINE_130mbar_1372Vdrift_25MHz_NGRAW.json \
   <build>/config/config_GUI_filter.json \
   --input.dataFile $( <build>/bin/grawls --input 20220828111932 --chunk 0 --separator "," --directory /data/edaq/2022/HIgS_2022/20220828_extTrg_CO2_130mbar --ms 1000 )
```

At the **daqula2** node set up the ROOT environment beforehand:
```Shell
source /opt/soft/root_v6.08.06/bin/thisroot.sh
```

Any application parameter can be set in a JSON file or with a command line argument. Command line
arguments **overwrite** the settings from the JSON file(s); later JSON files overwrite earlier ones.
The list of all parameters is printed by

```Shell
<build>/bin/tpcGUI --help
```

More details on the configuration files are in the [Utilities README](Utilities/README.md).

### Web GUI (tpcWebGUI)

`tpcWebGUI` has the functionality of `tpcGUI`, displayed in a web browser, also from a remote computer.
It takes the same configuration files and options:

```Shell
<build>/bin/tpcWebGUI <build>/config/config_GUI__OFFLINE_190mbar_3332Vdrift_25MHz_NGRAW.json \
   --input.dataFile <comma separated GRAW or ROOT files>
```

At start-up it prints the URL to open (with an access token). See the
[Web GUI README](WebServerClient/README.md) for remote access, the Open dialog with run data sets
and the `web.*` options.

### Converting data to ROOT (tpcGRAW2ROOT)

`tpcGRAW2ROOT` writes the events (charge maps, reconstructed and, for MC, generated tracks) of GRAW,
ROOT or MC input to a ROOT file, e.g.:

```Shell
<build>/bin/tpcGRAW2ROOT <build>/config/config_GRAW2ROOT_Data.json --input.readNEvents 3
```

## Analysis instructions
The analysis is done using the ROOT framework. The analysis can be done in two ways: command line or batch mode. More details are provided in the Analysis package [README](Analysis/README.md).



