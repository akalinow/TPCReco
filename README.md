# TPCReco

## Dependences

The recommended way is to build and run TPCReco in the container (Docker or Singularity image
`elitpc/tpcreco-with_tf`), which provides all dependencies (ROOT, Boost, Geant4, TensorFlow, ...):
see [docker/USER_README.md](docker/USER_README.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

To install the dependencies on the host system instead:

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

### GUIs

* `tpcGUI` – ROOT based event display: [GUI README](GUI/README.md)
* `tpcWebGUI` – the same in a web browser, also from a remote computer: [Web GUI README](WebServerClient/README.md)

### Options

Application parameters are set with JSON files and command line arguments: [Utilities README](Utilities/README.md).

### Converting data to ROOT (tpcGRAW2ROOT)

GRAW files are read by TPCReco's own reader (see [GrawToROOT README](GrawToROOT/README.md)):
neither the GET software nor its frame format (`.xcfg`) files are needed. The GET software is used
only by an optional test, `GrawFileVsGET_tst`, which compares the TPCReco reader with the GET reader.
It is built when CMake finds the GET software. `tpcGRAW2ROOT` writes the events (charge maps, 
reconstructed and, for MC, generated tracks) of GRAW, ROOT or MC input to a ROOT file, e.g.:

```Shell
source /opt/soft/GetSoftware_bin/env_settings.sh
cmake -DBUILD_TEST=ON ../
<build>/bin/tpcGRAW2ROOT <build>/config/config_GRAW2ROOT_Data.json --input.readNEvents 3
```

## Analysis instructions
The analysis is done using the ROOT framework. The analysis can be done in two ways: command line or batch mode. More details are provided in the Analysis package [README](Analysis/README.md).

## Contributing

Updating to a newer version, the development environment and building the container images: [CONTRIBUTING.md](CONTRIBUTING.md).
