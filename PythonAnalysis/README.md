## Python based analysis

### Create ROOT file with PEventTPC data

Create ROOT file with PEventTPC data, and a plain TTree for quick analysis 
with 1000 events generated from the MC simulation. Enable saving the PEventTPC data.

```Bash
cd resources
../bin/makeMCTrackTree ../config/config_GUI_MC.json \
--input.controllerConfigPath=../config/montecarlo_2prong_gun_PEventTPC.json \
--input.readNEvents=1000
```

### Convert ROOT to TFRecord