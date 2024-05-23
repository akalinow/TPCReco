# Application configuration

 The application is configured using one or more JSON files, in which parameters (nodes) are organised in blocks (groups). Nested sub-blocks are not allowed at this point.

## Parameters definition
 All valid parameter names and their default values have to be registered in a special JSON file [allowedOptions.json](config/allowedOptions.json).
 This file is installed in the user's `$HOME/.tpcreco/config/` directory upon successful `make install` command. Here is an example definition of `"meta.configJson"` parameter inside such JSON file:
```json
{
    "configJson" : {
        "group" : "meta",
        "type" : "vector<string>",
        "defaultValue" : "",
        "description" : "JSON file(s) with application configuration.\nParameters values from JSON file are overwritten by values provided at command line.\nType: vector<string>"
    },
...
}

```
There are 2 categories of parameters:
  * scalars - having single value of type **T**,
  * vectors - having multiple values of identical type **T** and equivalent to C++ `std::vector<`**T**`>`,

where **T** denotes one of 6 implemented types: `int`, `unsigned int`, `float`, `double`, `bool` or `std::string`.

### Important syntax notes for JSON files:
  * the last value in each block must not end with comma (compare `"defaultValue"` and `"description"` lines in the example above)
  * the curly bracket ending the very last block in the JSON file must not end with comma
  * allowed parameter names are restricted to ASCII alphanumeric characters only
  * JSON file names are restricted to ASCII alphanumeric characters plus: dot (`.`), dash (`-`), underline (`_`) and slash (`/`) characters.
  * values of vector parameters have to be enclosed in square brackets and separated by commas with optional blank spaces (e.g. `[ val1, val2 ]`)
  * empty vectors can be specified either as an empty quote `""` or as a pair of square brackets `[ ]`
  * short forms such as `vector`, `string` and `uint` are also recognized.


## Parameters modification

 The default values of available parameters can be modified by providing input JSON file(s) and/or command line arguments.

 When multiple JSON files are provided to the application then the next JSON file(s) override(s) changes made by the preceding one(s).
 On top of that the command line arguments always override values provided in JSON files.
 Below is an example that combines 2 input JSON files and 7 command line options:
```bash
cd TPCReco/build/resources

../bin/exampleApp --meta.configJson          myConfig1.json myConfig2.json \
                  --someGroup.someTextVar    "Text A B C"                  \
                  --someGroup.someVar        VAL                           \
		  --someGroup.someBoolVar    true                          \
		  --someGroup.someEnableFlag                               \
		  --someGroup.someVec1       [ VAL1 VAL2 VAL3 ]            \
		  --someGroup.someVec2       VAL1 VAL2 VAL3                \
		  --someGroup.someEmptyVec   [ ]
```

 Overriding default values of scalar- or vector parameters inside a JSON file can be done as follows:
```json
{
...
    "someGroup" : {
       "someBoolVar":   true,
       "someIntVar":    1000,
       "someDoubleVar": "M_PI*2",
       "someFloatVec":  [ 1.0, "sin(M_PI/2)", "atan(1.)*4" ],
       "someBoolVec":   [ false, true, 0, 1, "false && TRUE" ],
       "someTextVec":   [ "Text A", "Text B", "Text C" ],
       "someEmptyVec":  ""
    },
...
}
```
 In case of both JSON file and command line methods:
   * non-string values can be provided as simple mathematical expressions using standard **C** and **ROOT::TMath** functions, which will be converted to  numerical values at run-time (see `"someDoubleVar"`, `"someFloatVec"` and `"someBoolVec"` in the example above)
   * bool values can be denoted as `0`/`1` or case-insensitive `true` / `false` (see `"someBoolVec"` in the example above)

 ### Important syntax notes for command line options:
  * only long dash convention `--groupName.paramName` of command line options is supported <!-- to allow parsing of negative numbers -->
  * same option cannot be specified more than once
  * when option `--help` is present the application exits after displaying relevant HELP message
  * values of vector parameters have to be separated by blank spaces and can be optionally enclosed within square brackets (e.g. `[ val1 val2 ]`)
  * vector parameter without any values implies an empty vector (alternatively one can use a pair of square brackets `[ ]`)
  * bool parameter without any value acts as a switching flag that implies `true` value.


## Saving the current JSON

The final configuration settings resulting from multiple JSON files and/or multiple command line arguments
can be saved to a new JSON file using [dumpConfig](bin/dumpConfig.cpp) application.

In order to produce a JSON file with all default values (its name is given by default value of `"meta.configDumpJson"` parameter inside [allowedOptions.json](config/allowedOptions.json)):
```bash
cd TPCReco/build/resources
../bin/dumpConfig
```
```
ConfigManager: using config file $HOME/.tpcreco/config/allowedOptions.json
for a list of allowed command line arguments and default parameters values
...
Configuration tree saved to: currentConfig.json

```

In order to produce a JSON file (its name is given by default value of `"meta.configDumpJson"` parameter inside [allowedOptions.json](config/allowedOptions.json)) with one parameter overwritten in the command line:
```bash
../bin/dumpConfig --beamParameters.energy 12.5
```
```
ConfigManager: using config file $HOME/.tpcreco/config/allowedOptions.json
for a list of allowed command line arguments and default parameters values
ConfigManager: updating parameters with command line arguments:
beamParameters.energy
List of final configuration tree to be used:
...
|
|___beamParameters:
|   |___energy: 12.5
...
Configuration tree saved to: configDump.json
```

In order to produce a JSON file of specific name that contains entire configuration after merging values from one JSON file and one parameter overwritten in the command line:
```bash
../bin/dumpConfig --meta.configDumpJson currentConfig.json --meta.configJson myConfig.json --beamParameters.energy 10.99
```
```
ConfigManager: using config file $HOME/.tpcreco/config/allowedOptions.json
for a list of allowed command line arguments and default parameters values
ConfigManager: updating parameters with config file: myConfig.json
ConfigManager: updating parameters with command line arguments:
meta.configJson
beamParameters.energy
List of final configuration tree to be used:
...
|
|___beamParameters:
|   |___energy: 10.99
...
Configuration tree saved to: currentConfig.json

```


## Accessing parameters within application

 Configuration parameters inside the application can be accessed by their `"groupName.parameterName"` labels in the following ways:
```c++

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  // Method #1
  // via BOOST property tree getters for scalar parameters
  auto energy = myConfig.get<float>("beamParameters.energy"));
  // via loop over BOOST property tree child node for vector parameters
  std::vector<int> vectorI;
  BOOST_FOREACH(auto item, myConfig.get_child("groupName.paramVectorInt") {
    vectorI.push_back(item.second.get_value<int>());
  }

  // Method #2
  // via static getter methods provided by ConfigManager that work with any BOOST property tree
  auto valueD = ConfigManager::getScalar<double>(myConfig, "groupName.paramDouble");
  auto vectorF = ConfigManager::getVector<std::vector<float>>(myConfig, "groupName.paramVectorFloat");

  // Method #3
  // via getter methods of particular ConfigManager instance
  auto valueF = cm.getScalar<float>("groupName.paramFloat");
  auto vectorD = cm.getVector<std::vector<double>>("groupName.paramVectorDouble");

```
 The resulting `boost::property_tree::ptree myConfig` object should be passed to all relevant classes that needs access to configuration parameters.


 The contructor of `ConfigManager` class can be overloaded to use only a subset of all allowed parameters inside the application and/or to provide an alternative JSON file with parameter definitions. In such cases the HELP message displayed by the application will be adjusted automatically. Here are two examples of use:
```c++
  // use a subset of available parameters defined by default parameter definition
  ConfigManager cm( { "groupA.param1", "groupA.param2", "groupB.param1" } );

  // use all allowed parameters from alternative JSON file with parameter definition
  ConfigManager cm( {}, "/my/location/myAllowedOptions.json" );

```

 One can also call a static method `ConfigManager::printTree` to display the entire content of any BOOST property tree:
```c++

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  ConfigManager::printTree(myConfig);

```
 which will produce a nice-looking output similar to this one:
```
|
|___conditions:
|   |___pressure: 190
|   |___samplingRate: 25
|   |___temperature: 293.15
|   |___driftV: 0.646
|
|___beamParameters:
|   |___energy: 12.5
|
```