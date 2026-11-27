# GUI – ROOT based event display (`tpcGUI`)

`tpcGUI` displays the raw events (charge maps of the U, V, W strips) and the reconstructed tracks,
from GRAW, ROOT or MC generator input, offline or online. A web browser version with the same
functionality is `tpcWebGUI`, see the [Web GUI README](../WebServerClient/README.md).

The general run instructions (installation paths, path lookup, container setup) are in the
[main README](../README.md#run-instructions).

## Running tpcGUI

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

## Examples

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

## Options

Any application parameter can be set in a JSON file or with a command line argument. Command line
arguments **overwrite** the settings from the JSON file(s); later JSON files overwrite earlier ones.
The list of all parameters is printed by

```Shell
<build>/bin/tpcGUI --help
```

More details on the configuration files are in the [Utilities README](../Utilities/README.md).
