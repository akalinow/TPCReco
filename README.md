# TPCReco

## Dependences

```
sudo apt-get update
sudo apt-get install $(cat requirements_apt.txt)
pip3 install -H -r requirements_pip3.txt
```

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

You can check the tag version fo your working directory with

```
cd TPCReco
git branch
```

the output should looke like this:

```
akalinow@daqula2:~/1/TPCReco$ git branch
* (HEAD detached at v0.02_28.04.2021)
  master

```


## Run instructions:

Update the  config/config_GUI.json with correct values for input date file (ROOT or GRAW), corresponding geometry file,
and location of the resources directory.  
When reading a GRAW file one has to run the application from the resources directory, as GET software requires a lots of additional files.  
When reading GRAW files setup the GET environment. At the daqula2 node use a following command:
For reading the GRAW files in online mode set  "dataFile": "directory_path". 

```
source /opt/soft/GetSoftware_bin/env_settings.sh
```

``` 
cd resources
../bin/tpcGUI ../config/config_GUI.json
```

If you want to override the "dataFile" and/or "removePedestal" setting from the config file, 
please use a command line option:

```
../bin/tpcGUI ../config/config_GUI.json --dataFile path --removePedestal true
```

Check config file [structure and examples](GUI/config/README.md). 
