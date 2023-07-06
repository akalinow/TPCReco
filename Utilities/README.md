# Application configuration

The application is configured using a JSON file. Parameters are organised into blocks, with no sub-block are allowed. All parameters have to be defined in the [allowedOptions.json](config/allowedOptions.json) file in a following format:

```json
{
    "configJson":{
        "group":"meta",
        "type" : "string",
        "defaultValue" : "",
        "description" : "JSON file with application configuration.\nParameters values from json file are overwritten by values provided at command line.\nType: string"            
    },
}
```

**Important**: 
 * the last block of a value can not have coma after the value - here is it ```description```
 * the very last value in the whole JSON file also can not have coma after ending curly bracket
 
 Inside the application the parameters are accessed in following way:
 ```C++
 
ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double energy = myConfig.get<std::string>("beamParameters.energy"));
 
 ```
 The `boost::property_tree::ptree myConfig` object should be passed to every place where parameters are accessed.
 
 ## Parameters modification
 
 The default parameters can be modified at command line and/or by providing a input JSON file.
 The command line value overwrite values provided in JSON files:
 
 ```bash
 cd TPCReco/build/resources
 Apptainer> ../bin/dumpConfig 
ConfigManager: using config file /home/akalinow/.tpcreco/config/allowedOptions.json
 for a list of allowed command line arguments
 and default parameters values
Using default config file: defaultConfig.json
Beam energy is: 0
Apptainer> ../bin/dumpConfig --beamParameters.energy 12.5
ConfigManager: using config file /home/akalinow/.tpcreco/config/allowedOptions.json
 for a list of allowed command line arguments
 and default parameters values
Updating parameters with command line arguments: 
Beam energy is: 12.5
Apptainer> ../bin/dumpConfig --meta.configJson test_energy.json 
ConfigManager: using config file /home/akalinow/.tpcreco/config/allowedOptions.json
 for a list of allowed command line arguments
 and default parameters values
Updating parameters with config file: test_energy.json
Updating parameters with command line arguments.
Beam energy is: 99.99
```

## Saving the current JSON

The JSON file with whole configuration can be saved to a new file with [dumpConfig](bin/dumpConfig.cpp) application:

```bash
Apptainer> ../bin/dumpConfig --meta.configJson test_energy.json --meta.configDumpJson current_config.json --beamParameters.energy 120.99
ConfigManager: using config file /home/akalinow/.tpcreco/config/allowedOptions.json
 for a list of allowed command line arguments
 and default parameters values
Updating parameters with config file: test_energy.json
Updating parameters with command line arguments.
Beam energy is: 120.99
```
