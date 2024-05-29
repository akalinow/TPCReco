# Application configuration

 The application is configured using one or more JSON files, in which parameters are organised in blocks (groups).
 Nested sub-blocks are also possible.

## Definition of parameters
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
There are 4 categories of parameters:
  * **scalar** - having single value of type **T**,
  * **vector of scalars** - having multiple values of identical type **T** and equivalent to C++ `std::vector<`**T**`>`,
  * **ptree** object - BOOST property tree with its content enclosed in curly brackets `{ }` and equivalent to `boost::property_tree::ptree`,
  * **vector of ptree** objects - having multiple elements of **ptree** type and being just another variant of `boost::property_tree::ptree`,

where **T** denotes one of 6 implemented types: `int`, `unsigned int`, `float`, `double`, `bool` or `std::string`.

### Important syntax notes for JSON files:
  * the last value in each block must not end with comma (compare `"defaultValue"` and `"description"` lines in the example above)
  * the curly bracket ending the very last block in the JSON file must not end with comma (`,`)
  * allowed parameter and group names are restricted to ASCII alphanumeric plus dash (`-`) characters
  * JSON file names are restricted to ASCII alphanumeric plus: dot (`.`), dash (`-`), underline (`_`) and slash (`/`) characters
  * values of vector parameters have to be enclosed in square brackets and separated by commas with optional blank spaces (e.g. `[ elem1, elem2 ]`)
  * empty vectors can be specified either as an empty quote `""` or as a pair of square brackets `[ ]`
  * short forms such as `vector`, `string` and `uint` are also recognized
  * parameter `meta.configJson` is transient and setting it inside JSON file will not affect the actual list of input JSON files specified in the command line.


## Modification of parameters

 The default values of available parameters can be modified by providing optional input JSON file(s) and/or optional command line arguments.

 First N consequtive command line arguments are treated as input JSON files <!--# internally assigned to 'meta.configJson' option #-->
 which are followd by regular options (`--groupName.parameterName`) and their respective values.
 When multiple JSON files are provided to the application then the next JSON file(s) override(s) changes made by the preceding one(s).
 On top of that the command line arguments always override values provided by JSON files.

 Below is an example that combines 2 input JSON files and 9 command line options:
```bash
cd TPCReco/build/resources

../bin/exampleApp myConfig1.json myConfig2.json                                                                           \
                  --someGroup.someTextVar          "Text A B C"                                                           \
                  --someGroup.someVar              VAL                                                                    \
                  --someGroup.someBoolVar          true                                                                   \
                  --someGroup.someEnableFlag                                                                              \
                  --someGroup.someVec1             [ VAL1 VAL2 VAL3 ]                                                     \
                  --someGroup.someVec2             VAL1 VAL2 VAL3                                                         \
                  --someGroup.someEmptyVec         [ ]                                                                    \
                  --someGroup.somePtree            "{ \"Par1\" : 1, \"Par2\": \"Text\" }"                                 \
                  --someGroup.somePtreeVec         [ "{ \"Tree1\" : "A" }" "{ \"Tree2\" : { \"Par1\": 1, \"Par2\": 2}}" ]
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
       "someEmptyVec":  "",
       "somePtree":     { "Par1": 1, "Par2": "Text" },
       "somePtreeVec":  [
                          { "Tree1": "A" },
                          { "Tree2": { "Par1": 1, "Par2": 2 }}
                        ]
    },
...
}
```
 In case of regular types (i.e. not **ptree**) in, both, JSON file- and command line methods one can represent:
   * numeric values as simple mathematical expressions using standard **C** and **ROOT::TMath** functions, which will be converted to  numerical values at run-time (see `"someDoubleVar"`, `"someFloatVec"` and `"someBoolVec"` in the example above)
   * boolean values as `0`/`1` or case-insensitive `true` / `false` (see `"someBoolVec"` in the example above)

 ### Important syntax notes for command line options:
  * only long dash convention `--groupName.paramName` of command line options is supported <!-- to allow parsing of negative numbers -->
  * whenever option `--help` is present the application exits after displaying relevant HELP message
  * specifying input JSON files by means of `--meta.configJson` option is also allowed <!-- but cannot be mixed with positional cmd line options -->
  * same option cannot be specified more than once
  * values of vector parameters have to be separated by blank spaces and can be optionally enclosed within square brackets (e.g. `[ val1 val2 ]`)
  * vector parameter without any values implies an empty vector (alternatively one can use a pair of square brackets `[ ]`)
  * boolean parameter without any value acts as a switching flag that implies `true` value.


## Saving current parameters to JSON file

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
../bin/dumpConfig myConfig.json  --beamParameters.energy 10.99 --meta.configDumpJson currentConfig.json
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


## Accessing parameters inside application

 In general configuration parameters inside the application can be accessed by their `"groupName.parameterName"` labels.

 Nested parameter values of non-vector type can be easily accessed via their respective `"group1.group2.parameterName"` labels.

 Below are four examples how to access configuration parameters using either native BOOST or ConfigManager methods:
```c++

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  // Method #1
  // via BOOST property tree getters for scalar parameters of known type (here T=float)
  auto energy = myConfig.get<float>("beamParameters.energy"));
  // via looping over BOOST property tree child node for vector parameters of known type (here T=int)
  std::vector<int> vectorI;
  BOOST_FOREACH(auto item, myConfig.get_child("groupName.paramVectorInt") {
    vectorI.push_back(item.second.get_value<int>());
  }

  // Method #2
  // via looping over all nested children of BOOST property tree of unknown type,
  // for details see: ConfigManager::printTree() and ConfigManager::listTree() methods using recurrent approach
  BOOST_FOREACH(auto item, myConfig.get_child("groupName.paramPtree") { // scan 1st level
    std::cout << "1st-level child:  key: '" << item.first << "',  size: " << item.second.size()
              << ",  value: " << item.second.get_value<std::string>() << std::endl;
    if(item.second.size()) {
       BOOST_FOREACH(auto item2, item.second) { // scan 2nd level
          std::cout << "2nd-level child:  key: '" << item2.first << "',  size: " << item2.second.size()
                    << ",  value: " << item2.second.get_value<std::string>() << std::endl;
          if(item2.second.size()) {
             BOOST_FOREACH(auto item3, item2.second) { // scan 3rd level
                std::cout << "3rd-level child:  key: '" << item3.first << "',  size: " << item3.second.size()
                          << ",  value: " << item3.second.get_value<std::string>() << std::endl;
             }
          }
       }
    }
  }

  // Method #3
  // via static getter methods for parameters of known type provided by ConfigManager that work with any BOOST property tree
  auto valueD = ConfigManager::getScalar<double>(myConfig, "groupName.paramDouble");
  auto vectorF = ConfigManager::getVector<std::vector<float>>(myConfig, "groupName.paramVectorFloat");

  // Method #4
  // via getter methods for parameters of known type for particular ConfigManager instance (cm)
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