# Configuration files

Currently `tpcGUI` takes [JSON](https://en.wikipedia.org/wiki/JSON#Syntax) formatted configuration file as input.

## Formatting requirements

The first [JSON](https://en.wikipedia.org/wiki/JSON#Syntax) object in a config file has to be called "Options" and then to include options as JSON objects within. Every option has to have four values specified: type, defaultValue, description and isRequired.


## Example

```json
{   "Options":
    {
        "pressure": {
            "type": "float",
            "defaultValue": 60,
            "description": "float - CO2 pressure [mbar]",
            "isRequired": true
        },
        "no-type": {
            "type": "bool",
            "defaultValue": false,
            "description": "Skip comparing event type",
            "isRequired": false
        },
        "files": {
            "type": "std::vector<std::string>",
            "defaultValue": "PLACEHOLDER_FOR_VALUE",
            "description": "strings - list of files to browse. Mutually exclusive with 'directory'",
            "isRequired": false
        }
    }
}
```